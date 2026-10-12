# Administration and HTTP security

Administrators are stable public character IDs in `/home/container/zanik/admins.txt`; the UI displays the linked character name. Zanik uses character-linked sessions and five-minute, single-use console claim URLs. There is no reusable web password and the game/server password is never reused. Removing an administrator immediately invalidates that character's sessions.

State-changing requests require a session, independent feature permission, CSRF value, and rate limit. Audit records omit raw claim, session, CSRF, and proxy-secret values. The console proxy secret is generated with mode `0600` and must never be printed or exposed by diagnostics.

This release intentionally supports HTTP. Cookies and all traffic can be intercepted on an untrusted network. Restrict the port to a trusted network or put a TLS reverse proxy in front of Zanik. HTTPS and trusted-proxy policy are a later phase; cookie/proxy configuration remains extensible for it.
