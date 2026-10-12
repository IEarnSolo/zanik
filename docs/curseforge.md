# CurseForge distribution

Create the project as **Zanik** under **UE4SS Mods > Utilities**, the closest class currently available. State prominently that Zanik is a native Linux dedicated-server mod using process-scoped `LD_PRELOAD`, does not use UE4SS, and is categorized there only because CurseForge has no native Linux/server-mod class. If moderation rejects the category or binary, keep GitLab as the canonical release source and ask CurseForge for the correct classification; do not disguise the architecture.

Publish the validated `zanik-server-linux-x86_64-VERSION.zip` as the primary manual server download. Publish Linux and Windows asset-tool bundles as optional additional files. Do not upload a locally generated `zanik-assets-*` bundle.

Describe Zanik as a server-side native Linux mod using process-scoped `LD_PRELOAD`. The recommended wrapper does not modify game files. Clearly label the fixed-path Pterodactyl compatibility mode as non-recommended because it temporarily replaces a core executable path while preserving the original.

The release description must link installation, security, assets, and hosting documents and note that mutation/QoL behavior is opt-in. A Windows server loader is deferred; Windows support in this release is only for local asset building.

After creating the project, generate a token from the CurseForge API Tokens page. Put it only in GitLab's protected, masked-and-hidden `CURSEFORGE_API_TOKEN` variable. Put the numeric project ID in protected `CURSEFORGE_PROJECT_ID`. See [releasing](releasing.md) for the manual job and partial-upload recovery procedure.

The `publish-curseforge` job submits the server ZIP as the primary release for game version `1.0`. It attaches both asset-tool archives to that primary file, uses Markdown release notes, and allows publication automatically after moderation. The server tarball stays on GitLab.
