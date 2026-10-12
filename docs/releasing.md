# Releasing Zanik

`VERSION` is the release version source of truth. Stable releases use an annotated tag named exactly `v<VERSION>`, such as `v0.1.0`. Release notes come from the matching `CHANGELOG.md` section.

## One-time GitLab configuration

Protect the tag wildcard `v*` under **Settings > Repository > Protected tags** and allow only Maintainers to create it. If protected environments are available, protect `curseforge-production` for Maintainers as well.

The release pipeline uses `CI_JOB_TOKEN` for GitLab packages and releases. Do not add a personal GitLab release token.

For CurseForge, create these project CI/CD variables:

- `CURSEFORGE_API_TOKEN`: protected, masked and hidden, variable expansion disabled.
- `CURSEFORGE_PROJECT_ID`: protected. This identifier is not secret.

Never enable CI debug tracing on a job with the CurseForge token.

Optional Windows signing uses protected, masked variables `WINDOWS_SIGNING_CERT_BASE64` and `WINDOWS_SIGNING_CERT_PASSWORD`. Omit both for the documented unsigned portable release. Never store a certificate or password in the repository.

## Release procedure

1. Update `VERSION` and the matching `CHANGELOG.md` section.
2. Run `python3 tools/release_contract.py --tag vVERSION` and the clean test/package validation suite.
3. Commit and push to `main`; wait for its pipeline to pass.
4. Create and push an annotated tag from that exact commit:

   ```sh
   git tag -a v0.1.0 -m "Zanik 0.1.0"
   git push origin v0.1.0
   ```

5. Wait for the tag pipeline. It publishes Generic Package `zanik/0.1.0`, creates the GitLab Release, and retains the validated tag artifact indefinitely.
6. Download the release files afresh and verify `sha256sum --check SHA256SUMS`.
7. Run the optional `publish-curseforge` manual job when the approved project and protected variables are ready.

Branch artifacts expire after 30 days. Tagged packages and GitLab Releases are the authoritative durable downloads.

## CurseForge recovery

The job uploads the server ZIP first, then records its CurseForge file ID before uploading the two asset-tool children. Its `curseforge-upload.json` artifact is retained even on failure.

Do not blindly retry an upload after a timeout or partial failure. Inspect the report and the CurseForge project. For recovery, run the manual job with `CURSEFORGE_PRIMARY_FILE_ID` set to the recorded primary ID. Set `CURSEFORGE_SKIP_CHILDREN` to any comma-separated combination of `linux`, `windows`, and `manager` for child files already accepted. These recovery values are non-secret and apply only to that manual run.
