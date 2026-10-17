# Zanik Manager

Zanik Manager is the recommended Windows interface for installing and maintaining Zanik on a Pterodactyl-hosted Dragonwilds server. It is portable: extract the complete archive and open `ZanikManager.exe`.

The **Changelog** tab provides the bundled release history through the installed Manager version, with the newest version expanded. The **Activity** tab timestamps progress and diagnostic output in local time.

The bottom-right indicators separately track the Manager release and the active server's Linux Dragonwilds build every ten minutes. Manager checks use stable GitHub releases and offer a verified transactional **Update and relaunch** action. Game checks read the selected server's Steam manifest and public dedicated-server metadata, work without Zanik installed, and never install a game update. **Install Zanik** is available only on an unmodded server. The server summary compares an operational payload contract independently from the Manager version. **Update / repair Zanik** remains available for an intentional repair and reuses the existing installation mode, but a Manager-only release does not create a false server-update requirement.

Manager remembers the last selected server in `%LOCALAPPDATA%\Zanik Manager\preferences.json`. The file contains only a profile identifier and no panel credentials.

The external updater runs from LocalAppData rather than the portable installation directory, retries short-lived Windows file locks, and records the exact failed swap operation in its LocalAppData journal while preserving or restoring the prior installation.

API keys are stored in Windows Credential Manager, not in the application directory or profile file. When connecting to a remote HTTP panel, the Manager displays a confirmation before sending the key and keeps a compact warning on that profile because its traffic is not encrypted. HTTPS and loopback connections do not display this warning.

The first release supports Pterodactyl. Wrapper and acknowledged executable-replacement installations remain available, while the standalone shell scripts and Windows asset extractor are packaged separately for advanced use.

Install, Doctor, configuration, extraction, upload, and activation operations display modal stage progress and block conflicting actions. After installation, Manager first offers an optional website-settings step for Zanik's separate TCP port and public URL, then offers to complete website asset setup for that exact server. Skipped website settings remain available under the selected server's Configuration tab. Asset setup can reuse a validated local bundle or extract, upload, atomically activate, and verify a new one.

Configuration loads automatically when the selected server changes. An unmodded server is reported as not installed rather than as a failed file download. The guided editor includes `ZANIK_ENABLED`, a master switch that preserves files and data while starting the official server without the mod.

Asset extraction checks whether Dragonwilds is running and can request a graceful close; it never force-terminates the game. When UE4SS or another proxy loader is already installed, Manager asks permission, preserves and verifies it outside the game directory, uses Zanik's mapper temporarily, and restores the original files exactly. Manual uploads always show an explicit target-server selector and confirmation.

Interrupted catalog, packaging, or cleanup work is journaled under `%LOCALAPPDATA%\Zanik Manager\asset-work\`. The Assets tab offers **Resume extraction** and **Discard generated work**. Special building images are authored build-UI thumbnails extracted directly from the client; Zanik does not render models or require Blender.
