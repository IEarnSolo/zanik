# Zanik Manager

Zanik Manager is the recommended Windows interface for installing and maintaining Zanik on a Pterodactyl-hosted Dragonwilds server. It is portable: extract the complete archive and open `ZanikManager.exe`.

API keys are stored in Windows Credential Manager, not in the application directory or profile file. When connecting to a remote HTTP panel, the Manager displays a confirmation before sending the key and keeps a compact warning on that profile because its traffic is not encrypted. HTTPS and loopback connections do not display this warning.

The first release supports Pterodactyl. Wrapper and acknowledged executable-replacement installations remain available, while the standalone shell scripts and Windows asset extractor are packaged separately for advanced use.

Install, Doctor, configuration, extraction, upload, and activation operations display modal stage progress and block conflicting actions. Asset extraction also checks that Dragonwilds is closed before the mapping workflow begins.
