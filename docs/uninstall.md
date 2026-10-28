# Uninstall

Stop the server, then run:

```sh
/home/container/zanik/scripts/uninstall.sh --server-root /home/container
```

In compatibility mode this restores the preserved official binary only when the current executable is a verified Zanik launcher. The command removes installed Zanik files but leaves `/home/container/zanik-data` for recovery or reinstall. Add `--purge-data` only to deliberately remove all Zanik persistence and secrets.

Zanik Manager moves a verified removed installation into the `uninstall` recovery family and retains only its newest successful recovery point. If data purge is explicitly selected, the data is moved into that recovery point rather than immediately destroyed. Doctor can review superseded recognized uninstall backups; it never treats the active `zanik-data` directory as cleanup data.

For replacement installs, Manager verifies and restores the preserved official game executable and restores `AUTO_UPDATE` only if it still has Zanik's managed value. For automatic wrapper installs, current installation metadata retains the original editable executable variable across repairs and updates; uninstall restores it only if it still points to Zanik. Operator changes are never overwritten. Older or manually configured wrapper installs that lack safe restoration metadata are uninstalled with the server left stopped so the normal startup command can be restored explicitly.

Uninstall preserves the server's prior power state: an offline server remains offline, while a previously running server is restarted only after its executable/startup path and removal postconditions verify. The active `zanik` directory is removed, while `zanik-data`—including player history, activity, diagnostics, admin state, and other persistent stores—remains in place by default.
