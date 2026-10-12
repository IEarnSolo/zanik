# Linux server installation

Linux x86-64 is the supported server runtime. Install the server's standard C++ runtime dependencies, extract the Zanik server archive, and use wrapper mode from [installation.md](installation.md). The launcher sets `LD_PRELOAD` only for the game process; do not add it globally or to the container environment.

Keep the official game content and `/home/container/zanik` separate from persistent `/home/container/zanik-data`. Run `doctor.sh --json` after installation and every update.
