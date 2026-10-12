# Uninstall

Stop the server, then run:

```sh
/home/container/zanik/scripts/uninstall.sh --server-root /home/container
```

In compatibility mode this restores the preserved official binary only when the current executable is a verified Zanik launcher. The command removes installed Zanik files but leaves `/home/container/zanik-data` for recovery or reinstall. Add `--purge-data` only to deliberately remove all Zanik persistence and secrets.
