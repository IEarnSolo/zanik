[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$GameRoot,
    [string]$OutputDirectory,
    [string]$UsmapPath,
    [switch]$KeepWork,
    [switch]$CatalogsOnly,
    [switch]$SkipGameLaunch,
    [switch]$PreserveExistingLoader,
    [string]$RecoveryDirectory,
    [string]$WorkDirectory,
    [switch]$Resume,
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
$Pipeline = Join-Path $Root 'tools\windows_asset_pipeline.py'
$Recipe = Join-Path $Root 'asset-tools\extraction-recipes.json'
$ToolLock = Join-Path $Root 'asset-tools\tools.lock.json'
$RecipeVersion = (Get-Content -LiteralPath $Recipe -Raw | ConvertFrom-Json).schemaVersion
$Work = if ($WorkDirectory) { [IO.Path]::GetFullPath($WorkDirectory) } else { Join-Path ([IO.Path]::GetTempPath()) ("zw-" + [Guid]::NewGuid().ToString('N').Substring(0, 8)) }
$JournalPath = Join-Path $Work 'extraction-journal.json'
$MarkerPath = Join-Path $Work '.zanik-asset-work.json'
$StateOrder = @{ created = 0; mappings = 1; exports = 2; generating = 3; generated = 4; bundled = 5; parts = 6; complete = 7 }
function Write-JsonAtomic([string]$Path, [object]$Value) {
    $Temporary = $Path + '.new'
    $Json = ($Value | ConvertTo-Json -Depth 8) + [Environment]::NewLine
    $Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [IO.File]::WriteAllText($Temporary, $Json, $Utf8NoBom)
    Move-Item -LiteralPath $Temporary -Destination $Path -Force
}
function Set-ExtractionStage([string]$Stage) {
    $script:Journal.stage = $Stage
    $script:Journal.updatedAt = [DateTime]::UtcNow.ToString('o')
    Write-JsonAtomic $JournalPath $script:Journal
}
$Executable = Join-Path $GameRoot 'RSDragonwilds\Binaries\Win64\RSDragonwilds-Win64-Shipping.exe'
$Fingerprint = @{
    gameExecutableSha256 = (Get-FileHash -LiteralPath $Executable -Algorithm SHA256).Hash.ToLowerInvariant()
    recipeSha256 = (Get-FileHash -LiteralPath $Recipe -Algorithm SHA256).Hash.ToLowerInvariant()
    toolLockSha256 = (Get-FileHash -LiteralPath $ToolLock -Algorithm SHA256).Hash.ToLowerInvariant()
}
if ($Resume) {
    if (-not (Test-Path -LiteralPath $JournalPath)) { throw "No resumable extraction exists at '$Work'." }
    $Journal = Get-Content -LiteralPath $JournalPath -Raw | ConvertFrom-Json
    foreach ($Name in $Fingerprint.Keys) {
        if ($Journal.fingerprints.$Name -ne $Fingerprint[$Name]) { throw "The resumable extraction no longer matches the game or packaged tools ($Name changed). Discard it and start again." }
    }
} else {
    if (Test-Path -LiteralPath $JournalPath) { throw "A resumable extraction already exists at '$Work'. Resume or discard it before starting another extraction." }
    New-Item -ItemType Directory -Path $Work -Force | Out-Null
    Write-JsonAtomic $MarkerPath @{ schemaVersion = 1; owner = 'Zanik asset extractor' }
    $Journal = [pscustomobject]@{ schemaVersion = 1; stage = 'created'; createdAt = [DateTime]::UtcNow.ToString('o'); updatedAt = [DateTime]::UtcNow.ToString('o'); fingerprints = $Fingerprint }
    Write-JsonAtomic $JournalPath $Journal
}
$Generated = Join-Path $Work 'generated-web'
$MappingWasGenerated = $false
$Succeeded = $false
try {
if ($StateOrder[$Journal.stage] -lt $StateOrder['mappings'] -and -not $UsmapPath) {
    if ($SkipGameLaunch) { throw '-SkipGameLaunch requires -UsmapPath because cooked catalog assets need locally generated mappings.' }
    $Ue4ss = Join-Path $Root 'third_party\ue4ss'
    $UsmapPath = Join-Path $Work 'Mappings.usmap'
    Write-ZanikEvent 'mappings' 'Preparing local Unreal mappings'
    $MappingArguments = @($AssetTool, 'prepare-mappings', '--game-root', $GameRoot, '--ue4ss-root', $Ue4ss, '--output', $UsmapPath, '--work-root', (Join-Path $Work 'mapping'))
    if ($PreserveExistingLoader) {
        Write-ZanikEvent 'preserve' 'Backing up and verifying the existing UE4SS installation'
        $MappingArguments += '--preserve-existing-loader'
    }
    if ($RecoveryDirectory) { $MappingArguments += @('--recovery-root', ([IO.Path]::GetFullPath($RecoveryDirectory))) }
    & $Python @MappingArguments
    if ($LASTEXITCODE -ne 0) { throw "Mapping generation failed with exit code $LASTEXITCODE" }
    if ($PreserveExistingLoader) { Write-ZanikEvent 'restore' 'Existing UE4SS installation restored and verified' }
    $MappingWasGenerated = $true
    Set-ExtractionStage 'mappings'
} else {
    $UsmapPath = if ($UsmapPath) { [IO.Path]::GetFullPath($UsmapPath) } else { Join-Path $Work 'Mappings.usmap' }
}
if ($StateOrder[$Journal.stage] -lt $StateOrder['generated']) {
$PipelineWorkspace = Join-Path $Work 'pipeline'
$PipelineArguments = @($Pipeline, '--game-root', $GameRoot, '--output', $Generated, '--tool-root', $Root, '--usmap-path', $UsmapPath, '--recipe', $Recipe, '--work-root', $Work, '--stage-journal', $JournalPath)
if (($Resume -and (Test-Path -LiteralPath $PipelineWorkspace -PathType Container)) -or $Journal.stage -in @('exports', 'generating')) { $PipelineArguments += @('--resume-work', $PipelineWorkspace) }
else { $PipelineArguments += @('--workspace', $PipelineWorkspace) }
if ($KeepWork) { $PipelineArguments += '--keep-work' }
if ($CatalogsOnly) { Write-Warning '-CatalogsOnly retains artwork referenced by runtime catalogs and omits only unreachable exports.' }
Write-ZanikEvent 'extract' 'Exporting targeted catalogs and artwork'
& $Python @PipelineArguments
if ($LASTEXITCODE -ne 0) { throw "Asset extraction failed with exit code $LASTEXITCODE" }
Set-ExtractionStage 'generated'
} elseif (-not (Test-Path -LiteralPath $Generated)) {
    throw "The extraction journal reports generated assets, but the checkpoint is missing: $Generated"
}
$Build = (& $Python $AssetTool show-build --game-root $GameRoot).Trim()
if (-not $Build) { $Build = 'unknown' }
$Bundle = Join-Path $OutputDirectory "zanik-assets-$Build.zip"
if ($StateOrder[$Journal.stage] -lt $StateOrder['bundled']) {
Write-ZanikEvent 'bundle' 'Validating and packaging the complete asset bundle'
& $Python $AssetTool bundle --game-root $GameRoot --generated-root $Generated --output $Bundle --mapping $UsmapPath --tool-versions-file $ToolLock --recipe-version $RecipeVersion --profile $Recipe
if ($LASTEXITCODE -ne 0) { throw "Asset bundling failed with exit code $LASTEXITCODE" }
Set-ExtractionStage 'bundled'
} elseif (-not (Test-Path -LiteralPath $Bundle)) {
    throw "The extraction journal reports a complete bundle, but it is missing: $Bundle"
}
$Parts = Join-Path $OutputDirectory "zanik-assets-$Build-parts"
$ResolvedOutput = [IO.Path]::GetFullPath($OutputDirectory).TrimEnd([IO.Path]::DirectorySeparatorChar)
$ResolvedParts = [IO.Path]::GetFullPath($Parts)
if (-not $ResolvedParts.StartsWith($ResolvedOutput + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Refusing to replace a bundle-parts directory outside the selected output directory.'
}
if ($StateOrder[$Journal.stage] -lt $StateOrder['parts']) {
& $Python $AssetTool bundle-set --bundle $Bundle --output-dir $Parts --replace
if ($LASTEXITCODE -ne 0) { throw "Multipart bundling failed with exit code $LASTEXITCODE" }
Set-ExtractionStage 'parts'
}
& $Python $AssetTool validate --bundle $Bundle
if ($LASTEXITCODE -ne 0) { throw "Bundle validation failed with exit code $LASTEXITCODE" }
Set-ExtractionStage 'complete'
$Succeeded = $true
Write-Host "Created $Bundle"
Write-Host "Created Pterodactyl bundle set at $Parts"
Write-ZanikEvent 'complete' "Created validated bundle $Bundle"
Write-Host "Upload the part directory to /home/container/zanik-assets-incoming/$Build/ and install bundle-index.json."
} finally {
    if ($KeepWork) {
        Write-Warning "Intermediate game exports remain at '$Work'. They are game-derived and must not be redistributed."
    } elseif ($Succeeded -and (Test-Path -LiteralPath $Work)) {
        $PendingCleanup = Join-Path (Split-Path -Parent $Work) ("pending-cleanup-" + [IO.Path]::GetFileName($Work) + '.json')
        & $Python $AssetTool cleanup-work --work-root $Work --pending-journal $PendingCleanup
        if ($LASTEXITCODE -eq 3) {
            Write-ZanikEvent 'cleanup-warning' "The validated bundle is ready, but temporary files are pending cleanup. Zanik Manager will retry safely."
        } elseif ($LASTEXITCODE -ne 0) {
            Write-Warning "Validated bundles are ready, but cleanup could not be completed."
        }
    } elseif (-not $Succeeded) {
        Write-ZanikEvent 'resume' "Extraction work was preserved at '$Work' and can be resumed from stage $($Journal.stage)."
    }
}
