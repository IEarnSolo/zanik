param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$MonitorArguments
)

$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    & wsl.exe -e python3 tools/gitlab_pipeline.py watch @MonitorArguments
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
