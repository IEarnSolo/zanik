param(
    [string]$JournalRoot = 'dist/recipe-unlock-all-json-current/RSDragonwilds',
    [string]$ExtractedIcons = 'dist/storage-all-icons-current',
    [string]$Output = 'web/npcs'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
function Workspace([string]$path) { if ([IO.Path]::IsPathRooted($path)) { $path } else { Join-Path $root $path } }
$journal = Workspace $JournalRoot
$icons = Workspace $ExtractedIcons
$output = Workspace $Output
New-Item -ItemType Directory -Path $output -Force | Out-Null

$wanted = @{}
Get-ChildItem $journal -Recurse -File -Filter '*.json' | Where-Object {
    $_.FullName -match '[\\/](World[\\/](Factions|Fauna)|Knowledge[\\/]People)[\\/]'
} | ForEach-Object {
    $entry = (Get-Content $_.FullName -Raw | ConvertFrom-Json)[0]
    $asset = $entry.Properties.Image.AssetPathName
    if ($asset -match '/([^/.]+)\.[^/.]+$') { $wanted[$Matches[1]] = $true }
}

$available = @{}
Get-ChildItem $icons -Recurse -File -Filter '*.png' | ForEach-Object {
    if (-not $available.ContainsKey($_.BaseName)) { $available[$_.BaseName] = $_.FullName }
}
$copied = 0; $missing = @()
foreach ($name in ($wanted.Keys | Sort-Object)) {
    if ($available.ContainsKey($name)) {
        Copy-Item -LiteralPath $available[$name] -Destination (Join-Path $output "$name.png") -Force
        $copied++
    } else { $missing += $name }
}
$manifest = [ordered]@{generatedAt=[DateTime]::UtcNow.ToString('o'); requested=$wanted.Count; copied=$copied; missing=$missing}
$manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $output 'manifest.json') -Encoding UTF8
[pscustomobject]@{Requested=$wanted.Count; Copied=$copied; Missing=$missing.Count; Output=$output}
