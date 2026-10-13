# Zanik Manager

Zanik Manager is the recommended Windows interface for installing and maintaining Zanik on a Pterodactyl-hosted Dragonwilds server. It is portable: extract the complete archive and open `ZanikManager.exe`.

API keys are stored in Windows Credential Manager, not in the application directory or profile file. When connecting to a remote HTTP panel, the Manager displays a confirmation before sending the key and keeps a compact warning on that profile because its traffic is not encrypted. HTTPS and loopback connections do not display this warning.

The first release supports Pterodactyl. Wrapper and acknowledged executable-replacement installations remain available, while the standalone shell scripts and Windows asset extractor are packaged separately for advanced use.

Install, Doctor, configuration, extraction, upload, and activation operations display modal stage progress and block conflicting actions. After installation, Manager first offers an optional website-settings step for Zanik's separate TCP port and public URL, then offers to complete website asset setup for that exact server. Skipped website settings remain available under the selected server's Configuration tab. Asset setup can reuse a validated local bundle or extract, upload, atomically activate, and verify a new one.

Asset extraction checks whether Dragonwilds is running and can request a graceful close; it never force-terminates the game. When UE4SS or another proxy loader is already installed, Manager asks permission, preserves and verifies it outside the game directory, uses Zanik's mapper temporarily, and restores the original files exactly. Manual uploads always show an explicit target-server selector and confirmation.
