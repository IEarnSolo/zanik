# Releasing Zanik

`VERSION` is the release version source of truth. Stable releases use an annotated tag named exactly `v<VERSION>`, such as `v0.1.0`. Release notes come from the matching `CHANGELOG.md` section.

Every server archive contains `payload-contract.json`, a deterministic contract over operational binaries, launchers, scripts, tools, configuration, and compatibility metadata. Manager code, documentation, changelogs, licenses, generated assets, and display-version files are excluded. Release validation must prove the Manager, nested server archive, and public release manifest report the same contract. A Manager-only release is expected to retain the prior contract.

`release-manifest.json` also records the Windows extraction asset contract: recipe version, reviewed compatibility-profile version, and exact recipe hash. Manager treats this independently from the server payload so it can explain whether a release requires only a Manager update, a Zanik server update, an asset refresh, or both. Never infer either requirement from the shared release number alone.

## One-time release configuration

Protect the tag wildcard `v*` under **Settings > Repository > Protected tags** and allow only Maintainers to create it. If protected environments are available, protect `curseforge-production` for Maintainers as well.

The release pipeline uses `CI_JOB_TOKEN` for GitLab packages and releases. Do not add a personal GitLab release token.

Create a fine-grained GitHub personal access token restricted to `IEarnSolo/zanik`, with only **Contents: Read and write** permission and an appropriate expiry. Add it to the GitLab project as `GITHUB_RELEASE_TOKEN`: protected, masked and hidden, variable expansion disabled, and environment scope `github-production`. Protect the `github-production` environment for Maintainers. This scope keeps the token out of ordinary branch and test jobs. Never enable CI debug tracing on a job that receives this token.

For CurseForge, create these project CI/CD variables:

- `CURSEFORGE_API_TOKEN`: protected, masked and hidden, variable expansion disabled.
- `CURSEFORGE_PROJECT_ID`: protected. This identifier is not secret.

Never enable CI debug tracing on a job with a release token.

Optional Windows signing uses protected, masked variables `WINDOWS_SIGNING_CERT_BASE64` and `WINDOWS_SIGNING_CERT_PASSWORD`. Omit both for the documented unsigned portable release. Never store a certificate or password in the repository.

## Release procedure

1. Update `VERSION` and the matching `CHANGELOG.md` section.
2. Run `python3 tools/release_contract.py --tag vVERSION` and the clean test/package validation suite.
3. Commit and push `main` to GitLab first. After that push succeeds, push the identical commit to GitHub, verify both remote branches resolve to the same SHA, and wait for the GitLab pipeline to pass:

   ```sh
   git push origin main
   git push github main
   git ls-remote origin refs/heads/main
   git ls-remote github refs/heads/main
   ```

4. Create an annotated tag from that exact commit. Push it to GitLab first and then immediately to GitHub:

   ```sh
   git tag -a v0.1.0 -m "Zanik 0.1.0"
   git push origin v0.1.0
   git push github v0.1.0
   ```

5. Wait for the tag pipeline. It publishes Generic Package `zanik/0.1.0`, creates the authoritative GitLab Release, retains the validated tag artifact indefinitely, verifies the matching GitHub tag commit, and mirrors the release notes and seven release files to GitHub.
6. Download the release files afresh and verify `sha256sum --check SHA256SUMS`.
7. Run the optional `publish-curseforge` manual job when the approved project and protected variables are ready.

Branch artifacts expire after 30 days. Tagged GitLab packages and GitLab Releases are authoritative; GitHub is the public source and release mirror. Routine pushes must always go to `origin` first and `github` second. Do not use force push, `--all`, or `--mirror` for synchronization.

The automatic `publish-github-release` job applies only to new stable tags after this automation was introduced; it does not backfill older releases. It waits up to five minutes for the GitHub tag. A missing or mismatched tag fails the job without changing the completed GitLab release. Retrying is safe: matching GitHub asset digests are retained and same-name content conflicts fail instead of being overwritten. The retained `github-release.json` report contains only public identifiers and file hashes.

## CurseForge recovery

The job uploads the server ZIP first, then records its CurseForge file ID before uploading the Manager and two asset-tool archives as independent normal files. Its `curseforge-upload.json` artifact is retained even on failure. CurseForge does not allow additional-file relationships for Zanik's project type, so none of these uploads uses `parentFileID`.

Do not blindly retry an upload after a timeout or partial failure. Inspect the report and the CurseForge project. For recovery, run the manual job with `CURSEFORGE_PRIMARY_FILE_ID` set to the recorded server-file ID. Set `CURSEFORGE_SKIP_CHILDREN` to any comma-separated combination of `linux`, `windows`, and `manager` for companion files already accepted. These recovery values are non-secret and apply only to that manual run.
