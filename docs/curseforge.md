# CurseForge release notes

Publish the validated `zanik-server-linux-x86_64-VERSION.zip` as the primary manual server download. Publish Linux and Windows asset-tool bundles as optional additional files. Do not upload a locally generated `zanik-assets-*` bundle.

Describe Zanik as a server-side native Linux mod using process-scoped `LD_PRELOAD`. The recommended wrapper does not modify game files. Clearly label the fixed-path Pterodactyl compatibility mode as non-recommended because it temporarily replaces a core executable path while preserving the original.

The release description must link installation, security, assets, and hosting documents and note that mutation/QoL behavior is opt-in. A Windows server loader is deferred; Windows support in this release is only for local asset building.
