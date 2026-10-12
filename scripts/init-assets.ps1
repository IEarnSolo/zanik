[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$GameRoot,
    [string]$ToolPath,
    [string]$InstallBundle,
    [string]$Destination
)
$ErrorActionPreference = 'Stop'
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$AssetTool = Join-Path $Here 'tools\zanik_assets.py'
if (-not (Test-Path -LiteralPath $AssetTool)) {
    $AssetTool = Join-Path (Split-Path -Parent $Here) 'tools\zanik_assets.py'
}
if (-not $Destination) { $Destination = Join-Path $Here 'assets' }
if ($InstallBundle) {
    & python $AssetTool install --bundle $InstallBundle --destination $Destination
    exit $LASTEXITCODE
}
$Generated = Join-Path $GameRoot 'zanik-generated-web'
$Inventory = Join-Path $Here 'asset-inventory.json'
& python $AssetTool inventory --game-root $GameRoot --generated-root $Generated --output $Inventory
if ($ToolPath) {
    if (-not (Test-Path -LiteralPath $ToolPath -PathType Leaf)) { throw "Tool not found: $ToolPath" }
    & $ToolPath --game-root $GameRoot --output $Generated
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if (-not (Test-Path -LiteralPath $Generated)) {
    $Hint = if ($ToolPath) { " Use the converter at '$ToolPath'." } else { '' }
    throw "Inventory is saved at $Inventory, but no generated web tree exists at $Generated.$Hint See docs/assets.md."
}
$Build = (& python $AssetTool show-build --game-root $GameRoot).Trim()
$Bundle = Join-Path $Here "zanik-assets-$Build.zip"
& python $AssetTool build --game-root $GameRoot --generated-root $Generated --output $Bundle
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Created $Bundle. Upload it and run init-assets.sh --game-root SERVER_ROOT --install-bundle FILE."
