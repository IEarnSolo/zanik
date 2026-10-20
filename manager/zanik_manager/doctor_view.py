from __future__ import annotations

import html
from datetime import datetime


STATUS_COLORS = {
    "healthy": "#22a06b",
    "attention": "#d97706",
    "error": "#dc2626",
    "neutral": "#a3a3a3",
}


def _truth(value) -> bool:
    return value is True or (isinstance(value, str) and value.lower() in {"healthy", "valid", "current", "complete"})


def _section(title: str, status: str, summary: str, action: str = "") -> dict:
    return {"title": title, "status": status, "summary": summary, "action": action}


def doctor_presentation(result: dict | None) -> dict:
    """Turn one raw Doctor result into stable, operator-facing sections."""
    result = result if isinstance(result, dict) else {}
    metadata = result.get("managerDoctor", {}) if isinstance(result.get("managerDoctor"), dict) else {}
    checks = result.get("checks", {}) if isinstance(result.get("checks"), dict) else {}
    installation = result.get("installation") if isinstance(result.get("installation"), dict) else None

    meaningful = any(key != "managerDoctor" for key in result)
    if not meaningful or (not installation and checks.get("installation")):
        overall = "Unavailable"
        overall_state = "error"
    elif result.get("healthy") is True:
        overall = "Healthy"
        overall_state = "healthy"
    else:
        overall = "Attention required"
        overall_state = "attention"

    if installation:
        version = installation.get("version", "unknown")
        mode = installation.get("mode", "unknown")
        sections = [_section("Installation and payload", "healthy",
                             f"Zanik {version} is installed in {mode} mode.",
                             "Run Update / repair Zanik if the installed payload is damaged or outdated.")]
    else:
        detail = str(checks.get("installation") or "No recognized Zanik installation was found.")
        sections = [_section("Installation and payload", "error", detail,
                             "Install Zanik, or retry Doctor after the server becomes reachable.")]

    launcher = result.get("launcher", {}) if isinstance(result.get("launcher"), dict) else {}
    automatic = result.get("autoUpdate", {}) if isinstance(result.get("autoUpdate"), dict) else {}
    if not installation:
        sections.append(_section("Launcher and automatic-update safety", "neutral",
                                 "Launcher safety is unavailable until an installation is recognized."))
    elif launcher.get("applicable") and not launcher.get("healthy"):
        sections.append(_section("Launcher and automatic-update safety", "error",
                                 "The replacement launcher did not pass verification.",
                                 "Use Update / repair Zanik before relying on replacement mode."))
    elif automatic and not automatic.get("healthy", True):
        sections.append(_section("Launcher and automatic-update safety", "attention",
                                 "The Pterodactyl automatic-update setting is unsafe for this installation mode.",
                                 "Use Update / repair Zanik to restore the required startup setting."))
    else:
        auto_text = (f"AUTO_UPDATE is {automatic.get('value')}." if automatic.get("available") else
                     "Automatic-update metadata is not required or is unavailable.")
        sections.append(_section("Launcher and automatic-update safety", "healthy",
                                 "The configured launcher mode is safe. " + auto_text))

    game = result.get("gameUpdate", {}) if isinstance(result.get("gameUpdate"), dict) else {}
    game_status = str(game.get("status", "unchecked"))
    if game.get("updateAvailable") is True or game_status == "update-available":
        sections.append(_section("Dragonwilds update state", "attention",
                                 "A newer Dragonwilds dedicated-server build is available.",
                                 "Use Update Dragonwilds server from the Servers tab."))
    elif game.get("updateAvailable") is False or game_status == "current":
        sections.append(_section("Dragonwilds update state", "healthy",
                                 "The installed Dragonwilds dedicated-server build is current."))
    else:
        sections.append(_section("Dragonwilds update state", "neutral",
                                 "No current Dragonwilds update result is available.",
                                 "Use Check game in the bottom status bar for a fresh result."))

    operational = [checks.get(name) for name in ("library", "analyzer", "launcher", "official") if name in checks]
    config_ok = bool(checks.get("configSha256"))
    if operational and all(value is True for value in operational) and config_ok:
        sections.append(_section("Configuration and operational files", "healthy",
                                 "The configuration is readable and the critical operational files match their recorded versions."))
    elif not installation:
        sections.append(_section("Configuration and operational files", "neutral",
                                 "Operational-file checks are unavailable without Zanik."))
    else:
        sections.append(_section("Configuration and operational files", "attention",
                                 "One or more configuration or operational-file checks did not pass.",
                                 "Review Raw diagnostics, then use Update / repair Zanik if needed."))

    assets = result.get("assets", {}) if isinstance(result.get("assets"), dict) else {}
    if not assets.get("installed"):
        sections.append(_section("Website assets", "attention", "No active website asset bundle was found.",
                                 "Extract and deploy assets from the Assets tab."))
    elif assets.get("compatible") is False or assets.get("mismatches"):
        sections.append(_section("Website assets", "error", "The active website asset bundle did not pass validation.",
                                 "Deploy a compatible validated bundle; the prior active bundle remains protected."))
    else:
        build = assets.get("gameBuild")
        suffix = f" for client build {build}" if build else ""
        sections.append(_section("Website assets", "healthy", "The active website asset bundle is compatible" + suffix + "."))

    storage = result.get("storageAndBackups", {}) if isinstance(result.get("storageAndBackups"), dict) else {}
    remote = storage.get("remote", result.get("backupRetention", {}))
    remote = remote if isinstance(remote, dict) else {}
    warnings = remote.get("warnings", []) if isinstance(remote.get("warnings", []), list) else []
    remote_status = str(remote.get("status", "available" if remote else "unavailable"))
    if warnings or remote_status in {"attention", "unhealthy", "error"}:
        sections.append(_section("Remote backups and recovery", "attention",
                                 "Recognized remote recovery data needs review.",
                                 "Use Review storage cleanup to inspect exact eligible paths."))
    elif remote:
        families = remote.get("families", {})
        count = sum(len(value) for value in families.values() if isinstance(value, list)) if isinstance(families, dict) else 0
        sections.append(_section("Remote backups and recovery", "healthy",
                                 f"Remote Zanik backup retention is available; {count} recognized recovery point(s) are reported."))
    else:
        sections.append(_section("Remote backups and recovery", "neutral",
                                 "Remote backup information is unavailable for this run."))

    local = storage.get("local", {}) if isinstance(storage.get("local"), dict) else {}
    if local:
        total = 0
        for value in local.values():
            if isinstance(value, dict) and isinstance(value.get("bytes"), int):
                total += value["bytes"]
        sections.append(_section("Local Manager storage", "healthy", f"Manager is tracking {human_size(total)} of local managed data."))
    else:
        sections.append(_section("Local Manager storage", "neutral", "Local Manager storage was not measured in this result."))

    return {
        "overall": overall,
        "overallState": overall_state,
        "mode": str(metadata.get("mode", "quick")).title(),
        "runAt": str(metadata.get("runAt", "Not run")),
        "sections": sections,
    }


def human_size(value: int) -> str:
    size = float(max(0, value))
    for unit in ("B", "KiB", "MiB", "GiB", "TiB"):
        if size < 1024 or unit == "TiB":
            return f"{int(size)} B" if unit == "B" else f"{size:.1f} {unit}"
        size /= 1024
    return "0 B"


def render_doctor_html(result: dict | None) -> str:
    presentation = doctor_presentation(result)
    color = STATUS_COLORS[presentation["overallState"]]
    parts = [
        f'<div style="padding:12px;border:1px solid {color};border-radius:6px;">',
        f'<div style="font-size:20px;font-weight:700;color:{color};">{html.escape(presentation["overall"])}</div>',
        f'<div style="color:#a3a3a3;">{html.escape(presentation["mode"])} Doctor · {html.escape(presentation["runAt"])}</div>',
        "</div>",
    ]
    for section in presentation["sections"]:
        section_color = STATUS_COLORS.get(section["status"], STATUS_COLORS["neutral"])
        parts.extend([
            f'<h3 style="margin-top:18px;margin-bottom:4px;color:{section_color};">{html.escape(section["title"])}</h3>',
            f'<p style="margin-top:0;">{html.escape(section["summary"])}</p>',
        ])
        if section.get("action"):
            parts.append(f'<p style="color:#a3a3a3;"><b>Suggested action:</b> {html.escape(section["action"])}</p>')
    return "".join(parts)
