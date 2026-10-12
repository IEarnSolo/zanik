param(
    [string[]]$MetadataRoots = @('dist/vestige-items-current', 'dist/mount-unlock-items-current'),
    [string[]]$IconRoots = @('dist/storage-all-icons-current', 'dist/storage-nonstandard-icons', 'dist/pharaoh-sceptre-icon', 'dist/rsdw-editor-main/RS-Dragonwilds-Editor-main/RunescapeDragonwildsEditor/assets'),
    [string]$Output = 'web/items',
    [string]$GraveMetadata = 'web/grave-items.tsv'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
function WorkspacePath([string]$path) { if ([IO.Path]::IsPathRooted($path)) { return $path }; Join-Path $root $path }

$icons = @{}
foreach ($iconRoot in $IconRoots) {
    $resolved = WorkspacePath $iconRoot
    if (-not (Test-Path $resolved)) { continue }
    Get-ChildItem $resolved -Recurse -File -Filter '*.png' | ForEach-Object {
        if (-not $icons.ContainsKey($_.BaseName)) { $icons[$_.BaseName] = $_.FullName }
    }
}

$records = @()
foreach ($metadataRoot in $MetadataRoots) {
    $resolved = WorkspacePath $metadataRoot
    if (-not (Test-Path $resolved)) { continue }
    Get-ChildItem $resolved -Recurse -File -Filter '*.json' | ForEach-Object {
        foreach ($entry in @(Get-Content $_.FullName -Raw | ConvertFrom-Json)) {
            $properties = $entry.Properties
            $name = $properties.Name.LocalizedString
            $id = $properties.PersistenceID
            $iconPath = $properties.Icon.AssetPathName
            if (-not $name -or -not $id -or -not $iconPath -or $entry.Name -match 'Deprecated') { continue }
            $icon = Split-Path (($iconPath -split '\.')[0]) -Leaf
            $records += [pscustomobject]@{
                Asset = $entry.Name; Name = $name; PersistenceID = $id; Icon = $icon
                MaxStack = if ($properties.MaxStackSize) { [int]$properties.MaxStackSize } else { 1 }
            }
        }
    }
}

$outputRoot = WorkspacePath $Output
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$missing = @()
foreach ($record in $records) {
    if (-not $icons.ContainsKey($record.Icon)) { $missing += $record; continue }
    Copy-Item -LiteralPath $icons[$record.Icon] -Destination (Join-Path $outputRoot "$($record.Asset).png") -Force
}
if ($missing.Count) { throw "Missing authoritative icons for: $($missing.Asset -join ', ')" }

$gravePath = WorkspacePath $GraveMetadata
$known = @{}
if (Test-Path $gravePath) {
    Get-Content $gravePath | ForEach-Object { if ($_ -match '^([^\t]+)\t') { $known[$matches[1]] = $true } }
}
foreach ($record in $records | Sort-Object Name, Asset) {
    if ($known.ContainsKey($record.PersistenceID)) { continue }
    Add-Content -LiteralPath $gravePath -Encoding UTF8 -Value "$($record.PersistenceID)`t$($record.Name)`t$($record.MaxStack)`t$($record.Asset).png"
    $known[$record.PersistenceID] = $true
}

[pscustomobject]@{ Items = $records.Count; Icons = $records.Count; Missing = 0; GraveMappings = $known.Count }
