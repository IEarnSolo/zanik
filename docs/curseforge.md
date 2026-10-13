# CurseForge distribution

Create the project as **Zanik** under **UE4SS Mods > Utilities**, the closest class currently available. State clearly that Zanik is a native Linux dedicated-server mod using process-scoped `LD_PRELOAD`, does not use UE4SS, and is categorized there only because CurseForge has no native Linux/server-mod class. If moderation rejects the category or binary, ask CurseForge for the correct classification; do not disguise the architecture.

Use the public GitHub mirror, <https://github.com/IEarnSolo/zanik>, for every source, issue, and documentation link shown on CurseForge. Do not reference private release infrastructure in CurseForge project text, file metadata, reviewer notes, or support links.

Publish the validated `zanik-server-linux-x86_64-VERSION.zip` as the primary manual server download. Publish Zanik Manager as the recommended Windows additional file, with Linux and Windows asset-tool bundles for advanced users. Do not upload a locally generated `zanik-assets-*` bundle.

The CurseForge project description and reviewer notes are edited manually in the CurseForge author dashboard. The CI job uploads release files, display names, and release changelogs; it does not replace the project description. Use the maintained copy in [curseforge-listing-copy.md](curseforge-listing-copy.md).

Describe Zanik as a server-side native Linux mod using process-scoped `LD_PRELOAD`. The recommended wrapper does not modify game files. Clearly label the fixed-path Pterodactyl compatibility mode as non-recommended because it temporarily replaces a core executable path while preserving the original.

Keep the public description concise and practical. Link manager, installation, security, asset, and hosting documentation through the GitHub mirror, note that mutation/QoL behavior is opt-in, and explain that the Windows app manages Linux dedicated servers and builds local assets; it is not a Windows server loader.

After creating the project, generate a token from the CurseForge API Tokens page. Store it only as the protected, masked-and-hidden CI variable `CURSEFORGE_API_TOKEN`. Store the numeric project ID as protected `CURSEFORGE_PROJECT_ID`. See [releasing](releasing.md) for the manual job and partial-upload recovery procedure.

The `publish-curseforge` job submits the server ZIP as the primary release for game version `1.0`. It attaches Zanik Manager and both asset-tool archives to that primary file, uses Markdown release notes, and allows publication automatically after moderation. The server tarball is not uploaded to CurseForge.
