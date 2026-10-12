# Upgrades

Stop the server, extract the new release to `/home/container/zanik-release/`, verify `SHA256SUMS`, then run:

```sh
/home/container/zanik-release/scripts/update.sh --server-root /home/container
```

The updater preserves configuration and creates a dated backup of authorization, sessions, audit logs, persistence, compatibility data, generated assets, installed files, and managed game paths. A failed update restores the prior installation. Run `doctor.sh --json` before restart and retain the backup until development-server validation passes.
