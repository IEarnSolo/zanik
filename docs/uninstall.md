# Uninstall

Stop the server, then run:

```sh
/home/container/zanik/scripts/uninstall.sh --server-root /home/container
```

In compatibility mode this restores the preserved official binary only when the current executable is a verified Zanik launcher. The command removes installed Zanik files but leaves `/home/container/zanik-data` for recovery or reinstall. Add `--purge-data` only to deliberately remove all Zanik persistence and secrets.

Zanik Manager moves a verified removed installation into the `uninstall` recovery family and retains only its newest successful recovery point. If data purge is explicitly selected, the data is moved into that recovery point rather than immediately destroyed. Doctor can review superseded recognized uninstall backups; it never treats the active `zanik-data` directory as cleanup data.
