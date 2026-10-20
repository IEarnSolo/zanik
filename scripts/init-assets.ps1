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
    [string]$BaselineBundle,
    [switch]$Resume,
    [switch]$Incremental,
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
$Work = if ($WorkDirectory) {
    [IO.Path]::GetFullPath($WorkDirectory)
} else {
    $StateRoot = if ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA 'Zanik Manager\asset-work-standalone' } else { Join-Path ([IO.Path]::GetTempPath()) 'Zanik-asset-work-standalone' }
    $Hasher = [Security.Cryptography.SHA256]::Create()
    try {
        $RootHash = ([BitConverter]::ToString($Hasher.ComputeHash([Text.Encoding]::UTF8.GetBytes($GameRoot.ToLowerInvariant())))).Replace('-', '').Substring(0, 16).ToLowerInvariant()
    } finally { $Hasher.Dispose() }
    Join-Path $StateRoot $RootHash
}
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
function Get-ZanikSha256([string]$Path) {
    # Get-FileHash is supplied by Microsoft.PowerShell.Utility, which is not
    # guaranteed to be discoverable in a packaged application's environment.
    # Use the .NET implementation available in both Windows PowerShell 5.1 and
    # PowerShell 7 so fingerprinting does not depend on module auto-loading.
    $Resolved = [IO.Path]::GetFullPath($Path)
    $Stream = [IO.File]::Open($Resolved, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
    $Hasher = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString($Hasher.ComputeHash($Stream))).Replace('-', '').ToLowerInvariant()
    } finally {
        $Hasher.Dispose()
        $Stream.Dispose()
    }
}
function Set-ExtractionStage([string]$Stage) {
    $script:Journal.stage = $Stage
    $script:Journal.updatedAt = [DateTime]::UtcNow.ToString('o')
    Write-JsonAtomic $JournalPath $script:Journal
}
$Executable = Join-Path $GameRoot 'RSDragonwilds\Binaries\Win64\RSDragonwilds-Win64-Shipping.exe'
$Fingerprint = @{
    gameExecutableSha256 = Get-ZanikSha256 $Executable
    recipeSha256 = Get-ZanikSha256 $Recipe
    toolLockSha256 = Get-ZanikSha256 $ToolLock
}
$GameChanged = $false
if (-not $Resume -and -not $Incremental -and (Test-Path -LiteralPath $JournalPath)) {
    $ExistingJournal = Get-Content -LiteralPath $JournalPath -Raw | ConvertFrom-Json
    if ($ExistingJournal.stage -eq 'complete') { $Incremental = $true }
}
if ($Resume -and $Incremental) { throw '-Resume and -Incremental cannot be used together.' }
if ($Resume) {
    if (-not (Test-Path -LiteralPath $JournalPath)) { throw "No resumable extraction exists at '$Work'." }
    $Journal = Get-Content -LiteralPath $JournalPath -Raw | ConvertFrom-Json
    foreach ($Name in $Fingerprint.Keys) {
        if ($Journal.fingerprints.$Name -ne $Fingerprint[$Name]) { throw "The resumable extraction no longer matches the game or packaged tools ($Name changed). Discard it and start again." }
    }
    if ($Journal.PSObject.Properties.Name -contains 'preserveExistingLoader' -and $Journal.preserveExistingLoader) {
        $PreserveExistingLoader = $true
    }
} elseif ($Incremental) {
    if (-not (Test-Path -LiteralPath $JournalPath)) { throw "No incremental extraction cache exists at '$Work'." }
    $Journal = Get-Content -LiteralPath $JournalPath -Raw | ConvertFrom-Json
    if ($Journal.stage -ne 'complete') { throw "The extraction workspace is incomplete at stage '$($Journal.stage)'; resume it before starting an incremental update." }
    if ($Journal.fingerprints.toolLockSha256 -ne $Fingerprint.toolLockSha256) {
        throw 'The packaged extraction tools changed and cannot safely reuse this raw cache. Discard it once and perform a full extraction.'
    }
    $PreviousFingerprints = $Journal.fingerprints
    $GameChanged = $PreviousFingerprints.gameExecutableSha256 -ne $Fingerprint.gameExecutableSha256
    $Journal | Add-Member -NotePropertyName previousFingerprints -NotePropertyValue $PreviousFingerprints -Force
    $Journal.fingerprints = $Fingerprint
    $Journal.stage = if ($GameChanged) { 'created' } else { 'mappings' }
    $Journal.updatedAt = [DateTime]::UtcNow.ToString('o')
    Write-JsonAtomic $JournalPath $Journal
    Write-ZanikEvent 'incremental' 'Updating the retained private asset cache; only changed, added, or removed game packages will be exported'
} else {
    if (Test-Path -LiteralPath $JournalPath) { throw "A resumable extraction already exists at '$Work'. Resume or discard it before starting another extraction." }
    New-Item -ItemType Directory -Path $Work -Force | Out-Null
    Write-JsonAtomic $MarkerPath @{ schemaVersion = 1; owner = 'Zanik asset extractor' }
    $Journal = [pscustomobject]@{ schemaVersion = 1; stage = 'created'; createdAt = [DateTime]::UtcNow.ToString('o'); updatedAt = [DateTime]::UtcNow.ToString('o'); fingerprints = $Fingerprint }
    Write-JsonAtomic $JournalPath $Journal
}
if ($PreserveExistingLoader -and $StateOrder[$Journal.stage] -lt $StateOrder['mappings']) {
    $Journal | Add-Member -NotePropertyName preserveExistingLoader -NotePropertyValue $true -Force
    Write-JsonAtomic $JournalPath $Journal
}
$Generated = Join-Path $Work 'generated-web'
$MappingWasGenerated = $false
$Succeeded = $false
try {
$PipelineWorkspace = Join-Path $Work 'pipeline'
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
$PipelineArguments = @($Pipeline, '--game-root', $GameRoot, '--output', $Generated, '--tool-root', $Root, '--usmap-path', $UsmapPath, '--recipe', $Recipe, '--work-root', $Work, '--stage-journal', $JournalPath)
$IncrementalPipeline = $Incremental -and (Test-Path -LiteralPath $PipelineWorkspace -PathType Container)
$ResumePipeline = (($Resume -and (Test-Path -LiteralPath $PipelineWorkspace -PathType Container)) -or ($Journal.stage -in @('exports', 'generating') -and -not $IncrementalPipeline))
if ($ResumePipeline) { $PipelineArguments += @('--resume-work', $PipelineWorkspace) }
elseif ($IncrementalPipeline) { $PipelineArguments += @('--incremental-work', $PipelineWorkspace) }
else { $PipelineArguments += @('--workspace', $PipelineWorkspace) }
if ($GameChanged) { $PipelineArguments += '--game-changed' }
if ($BaselineBundle) { $PipelineArguments += @('--baseline-bundle', ([IO.Path]::GetFullPath($BaselineBundle))) }
if ($KeepWork) { $PipelineArguments += '--keep-work' }
if ($CatalogsOnly) { Write-Warning '-CatalogsOnly retains artwork referenced by runtime catalogs and omits only unreachable exports.' }
if ($IncrementalPipeline) { Write-ZanikEvent 'extract' 'Checking retained exports and applying only changed, added, or removed packages' }
elseif ($ResumePipeline) { Write-ZanikEvent 'extract' 'Validating retained exports; game launch and UnrealAssetScout extraction are skipped' }
else { Write-ZanikEvent 'extract' 'Exporting targeted catalogs and artwork' }
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
$ExtractionMode = if ($Incremental) { 'incremental' } else { 'full' }
& $Python $AssetTool bundle --game-root $GameRoot --generated-root $Generated --output $Bundle --mapping $UsmapPath --tool-versions-file $ToolLock --recipe-version $RecipeVersion --profile $Recipe --extraction-mode $ExtractionMode --generation-provenance-file (Join-Path $PipelineWorkspace 'generation-state.json')
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
        Write-ZanikEvent 'incremental-cache' "Retained the private incremental extraction cache at '$Work'. Use Discard generated work to remove it; the next update would then require a full extraction."
    } elseif (-not $Succeeded) {
        Write-ZanikEvent 'resume' "Extraction work was preserved at '$Work' and can be resumed from stage $($Journal.stage)."
    }
}
