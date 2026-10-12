# GitLab pipeline monitoring

The monitor uses GitLab's read-only pipeline, job, and trace APIs. It sends the
token only in the `PRIVATE-TOKEN` request header and never prints it.

## Configuration

Copy `.env.gitlab.example` to `.env.gitlab`, then replace only the placeholder
in this line:

```text
GITLAB_TOKEN=glpat-replace-with-a-read-api-token
```

`.env.gitlab` is ignored by Git. Do not add it to commits, paste it into logs,
or put the token in a remote URL.

Create a project access token in the Zanik GitLab project:

1. Open **Settings > Access tokens**.
2. Select **Add new token**.
3. Name it `zanik-pipeline-monitor` and choose an appropriate expiration date.
4. Select the **Reporter** role and only the **read_api** scope.
5. Create the token and immediately copy the one-time value into
   `.env.gitlab` as `GITLAB_TOKEN`.

If project access tokens are disabled on the instance, create a personal access
token with only `read_api` instead. The project token is preferred because its
access is limited to this project. Rotate or revoke the token from the same
Access tokens page if the file is ever exposed.

## Usage

From PowerShell, monitor the pipeline for the currently checked-out commit:

```powershell
.\tools\watch-gitlab-pipeline.ps1
```

From Linux or WSL:

```sh
./tools/watch-gitlab-pipeline.sh
```

The monitor waits for GitLab to create the pipeline, reports status changes,
lists every job, and prints the last 200 lines from failed job traces. It exits
nonzero when the pipeline fails or times out.

The underlying tool also supports targeted inspection:

```sh
python3 tools/gitlab_pipeline.py latest --ref main
python3 tools/gitlab_pipeline.py jobs --pipeline-id 123
python3 tools/gitlab_pipeline.py trace --job-id 456
python3 tools/gitlab_pipeline.py watch --sha COMMIT_SHA --tail 400
```
