# Docker and generic hosts

Extract the server archive in an image build or persistent management volume, then run `install.sh --server-root GAME_ROOT --mode wrapper`. Use `GAME_ROOT/zanik/bin/zanik-server --server-root GAME_ROOT --` as the container entrypoint.

Persist `GAME_ROOT/zanik-data` and the game's normal save paths. Expose the configured Zanik TCP port separately from Dragonwilds UDP ports. The game-content volume can remain read-only in wrapper mode if its normal updater and save layout permit it. Never set Zanik's `LD_PRELOAD` globally for unrelated container processes.
