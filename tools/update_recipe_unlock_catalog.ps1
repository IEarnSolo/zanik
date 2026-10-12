param(
    [string]$GameRoot = 'E:\SteamLibrary\steamapps\common\RSDragonwilds',
    [string]$Python = 'C:\Users\Bill\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe',
    [switch]$AllowUnresolved
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$uas = Join-Path $repo '.tmp\uas\uas.exe'
$paks = Join-Path $GameRoot 'RSDragonwilds\Content\Paks'
$usmap = Join-Path $repo 'dist\RSDragonwilds-current.usmap'
$assets = Join-Path $repo 'dist\recipe-unlock-all-json-current'
$previous = Join-Path $repo 'reference\recipe-unlocks.previous.json'
$current = Join-Path $repo 'reference\recipe-unlocks.json'
$baseline = Join-Path $repo 'reference\recipe-unlock-testdude-baseline.json'

if (-not (Test-Path -LiteralPath $uas)) { throw "UnrealAssetScout is missing: $uas" }
if (-not (Test-Path -LiteralPath $paks)) { throw "Game Paks directory is missing: $paks" }
if (-not (Test-Path -LiteralPath $usmap)) { throw "Current .usmap is missing: $usmap" }

if (Test-Path -LiteralPath $current) { Copy-Item -LiteralPath $current -Destination $previous -Force }

& $uas export Json -p $paks -g GAME_UE5_6 -u $usmap -b -o $assets -c -r -l (Join-Path $repo '.tmp\recipe-unlock-all-export.log')
if ($LASTEXITCODE -ne 0) { throw "Asset extraction failed with exit code $LASTEXITCODE" }

$arguments = @((Join-Path $repo 'tools\generate_recipe_unlock_catalog.py'), '--assets', $assets,
    '--building-assets', $assets, '--fresh-baseline', $baseline,
    '--output', $current, '--docs-dir', (Join-Path $repo 'docs'),
    '--web-output', (Join-Path $repo 'web\recipe-unlocks.json'),
    '--runtime-web-output', (Join-Path $repo 'web\recipe-runtime-catalog.tsv'),
    '--runtime-history-output', (Join-Path $repo 'web\recipe-runtime-catalog-history.tsv'))
if ($AllowUnresolved) { $arguments += '--allow-unresolved' }
& $Python @arguments
if ($LASTEXITCODE -ne 0) { throw "Catalog validation failed; inspect reference/recipe-unlocks.json validation.unresolved" }

if (Test-Path -LiteralPath $previous) {
    & $Python (Join-Path $repo 'tools\diff_recipe_unlock_catalog.py') $previous $current
}
