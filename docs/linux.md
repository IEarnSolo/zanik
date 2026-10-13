# Self-hosted Linux server

Linux x86-64 is Zanik's supported server runtime. A self-hosted server is a first-class installation: it does not require Pterodactyl or Zanik Manager.

Install the standard C++ runtime dependencies and Python 3, then follow the complete [installation and lifecycle guide](installation.md). Use the `.tar.gz` server archive, verify it against `SHA256SUMS`, and install in wrapper mode. Run every lifecycle command as the same account that owns the server files and runs Dragonwilds.

The recommended example layout is:

```text
/srv/dragonwilds/                         game server root
/srv/dragonwilds/zanik/                   installed Zanik files and configuration
/srv/dragonwilds/zanik-data/              persistent Zanik data
/srv/dragonwilds/zanik-release/VERSION/   downloaded and extracted release
/srv/dragonwilds/zanik-assets-incoming/   transferred Windows-generated asset bundles
```

Do not set `LD_PRELOAD` globally. Start the service through `zanik/bin/zanik-server`, which applies it only to the game process.

Zanik also needs its own open TCP port for the website and API. Configure `STATUS_PORT` in `zanik/config/zanik.conf`, open that port in the firewall and any router or container mapping, and set `PUBLIC_BASE_URL` to the externally reachable website address. Never reuse the Dragonwilds game port.

The Linux dedicated-server containers cannot generate the complete website bundle. Build it from a licensed Windows Dragonwilds client, transfer the generated full ZIP with SCP or SFTP, and activate it with the installed initializer. The full procedure is in [Local game assets](assets.md#install-on-a-self-hosted-linux-server).

Run `zanik/scripts/doctor.sh --server-root SERVER_ROOT --json` after installation, asset activation, and every update.
