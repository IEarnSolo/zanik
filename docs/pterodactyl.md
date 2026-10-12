# Pterodactyl installation

Stop the server and upload the extracted archive to `/home/container/zanik-release/`. Copy no individual library into the game directory.

Preferred mode:

```sh
/home/container/zanik-release/scripts/install.sh --server-root /home/container --mode wrapper
```

Set the egg command to `/home/container/zanik/bin/zanik-server --server-root /home/container --` followed by its existing arguments.

If the egg command cannot be changed, use the explicitly acknowledged command in [installation.md](installation.md). That compatibility mode is non-recommended. Upload a client-built asset bundle to `/home/container/` and activate it only through `/home/container/zanik/scripts/init-assets.sh`.

For an allowlisted administrator, enter `zanik-admin <public-character-id>` in the Pterodactyl console. The canonical launcher intercepts only that command and prints a five-minute, single-use login link; every other console command is forwarded unchanged to the game server.
