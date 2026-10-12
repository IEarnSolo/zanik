[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$GameRoot,
    [string]$OutputDirectory,
    [string]$UsmapPath,
    [switch]$KeepWork,
    [switch]$CatalogsOnly,
    [switch]$SkipGameLaunch,
    [string]$InstallBundle,
    [string]$Destination
)
$ErrorActionPreference = 'Stop'
function Write-ZanikEvent([string]$Phase, [string]$Message) {
    $payload = @{ phase = $Phase; message = $Message } | ConvertTo-Json -Compress
    Write-Output "ZANIK_EVENT $payload"
}
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Root = $Here
if (-not (Test-Path -LiteralPath (Join-Path $Root 'tools\zanik_assets.py'))) {
    $Root = Split-Path -Parent $Here
}
$EmbeddedPython = Join-Path $Root 'runtime\python.exe'
$PythonCommand = Get-Command python -ErrorAction SilentlyContinue
$Python = if (Test-Path -LiteralPath $EmbeddedPython) { $EmbeddedPython } elseif ($PythonCommand) { $PythonCommand.Source } else { $null }
if (-not $Python) { throw 'The bundled Python runtime is missing. Re-download the Windows asset-tool archive.' }
$AssetTool = Join-Path $Root 'tools\zanik_assets.py'
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $Here 'output' }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
if (-not $Destination) { $Destination = Join-Path $Here 'assets' }
if ($InstallBundle) {
    & $Python $AssetTool install-bundle --bundle $InstallBundle --destination $Destination
    exit $LASTEXITCODE
}
$GameRoot = [IO.Path]::GetFullPath($GameRoot)
$Work = Join-Path ([IO.Path]::GetTempPath()) ("zw-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $Work -Force | Out-Null
$Generated = Join-Path $Work 'generated-web'
$MappingWasGenerated = $false
try {
if (-not $UsmapPath) {
    if ($SkipGameLaunch) { throw '-SkipGameLaunch requires -UsmapPath because cooked catalog assets need locally generated mappings.' }
    $Ue4ss = Join-Path $Root 'third_party\ue4ss'
    $UsmapPath = Join-Path $Work 'Mappings.usmap'
    Write-ZanikEvent 'mappings' 'Preparing local Unreal mappings'
    & $Python $AssetTool prepare-mappings --game-root $GameRoot --ue4ss-root $Ue4ss --output $UsmapPath --work-root (Join-Path $Work 'mapping')
    if ($LASTEXITCODE -ne 0) { throw "Mapping generation failed with exit code $LASTEXITCODE" }
    $MappingWasGenerated = $true
} else {
    $UsmapPath = [IO.Path]::GetFullPath($UsmapPath)
}
$Pipeline = Join-Path $Root 'tools\windows_asset_pipeline.py'
$Recipe = Join-Path $Root 'asset-tools\extraction-recipes.json'
$PipelineArguments = @($Pipeline, '--game-root', $GameRoot, '--output', $Generated, '--tool-root', $Root, '--usmap-path', $UsmapPath, '--recipe', $Recipe, '--work-root', $Work)
if ($KeepWork) { $PipelineArguments += '--keep-work' }
if ($CatalogsOnly) { Write-Warning '-CatalogsOnly retains artwork referenced by runtime catalogs and omits only unreachable exports.' }
Write-ZanikEvent 'extract' 'Exporting targeted catalogs and artwork'
& $Python @PipelineArguments
if ($LASTEXITCODE -ne 0) { throw "Asset extraction failed with exit code $LASTEXITCODE" }
$Build = (& $Python $AssetTool show-build --game-root $GameRoot).Trim()
if (-not $Build) { $Build = 'unknown' }
$Bundle = Join-Path $OutputDirectory "zanik-assets-$Build.zip"
$Versions = '{"python":"3.12.10-embed","unrealAssetScout":"0.5.0","ue4ss":"3.0.1-1135-gf6d5f942"}'
Write-ZanikEvent 'bundle' 'Validating and packaging the complete asset bundle'
& $Python $AssetTool bundle --game-root $GameRoot --generated-root $Generated --output $Bundle --mapping $UsmapPath --tool-versions $Versions --recipe-version 1 --profile $Recipe
if ($LASTEXITCODE -ne 0) { throw "Asset bundling failed with exit code $LASTEXITCODE" }
$Parts = Join-Path $OutputDirectory "zanik-assets-$Build-parts"
$ResolvedOutput = [IO.Path]::GetFullPath($OutputDirectory).TrimEnd([IO.Path]::DirectorySeparatorChar)
$ResolvedParts = [IO.Path]::GetFullPath($Parts)
if (-not $ResolvedParts.StartsWith($ResolvedOutput + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Refusing to replace a bundle-parts directory outside the selected output directory.'
}
if (Test-Path -LiteralPath $ResolvedParts) { Remove-Item -LiteralPath $ResolvedParts -Recurse -Force }
& $Python $AssetTool bundle-set --bundle $Bundle --output-dir $Parts
if ($LASTEXITCODE -ne 0) { throw "Multipart bundling failed with exit code $LASTEXITCODE" }
& $Python $AssetTool validate --bundle $Bundle
if ($LASTEXITCODE -ne 0) { throw "Bundle validation failed with exit code $LASTEXITCODE" }
Write-Host "Created $Bundle"
Write-Host "Created Pterodactyl bundle set at $Parts"
Write-ZanikEvent 'complete' "Created validated bundle $Bundle"
Write-Host "Upload the part directory to /home/container/zanik-assets-incoming/$Build/ and install bundle-index.json."
} finally {
    if ($KeepWork) {
        Write-Warning "Intermediate game exports remain at '$Work'. They are game-derived and must not be redistributed."
    } elseif (Test-Path -LiteralPath $Work) {
        Remove-Item -LiteralPath $Work -Recurse -Force
    }
}
