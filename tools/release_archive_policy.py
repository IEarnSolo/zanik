from pathlib import PurePosixPath


BANNED_NAMES = {
    ".env", ".env.gitlab", "admins.txt", "console-admin.key",
    "pterodactyl_api_key.txt", "agents.md", ".agents", ".codex",
}
BANNED_SUFFIXES = {".pak", ".utoc", ".ucas", ".usmap", ".sav", ".db", ".pem", ".p12", ".pfx"}


def prohibited_archive_path(name: str) -> bool:
    normalized = name.replace("\\", "/")
    if normalized.startswith("/"):
        return True
    while normalized.startswith("./"):
        normalized = normalized[2:]
    parts = PurePosixPath(normalized).parts
    return (".." in parts
            or any(part.lower() in BANNED_NAMES for part in parts)
            or PurePosixPath(normalized).suffix.lower() in BANNED_SUFFIXES)
