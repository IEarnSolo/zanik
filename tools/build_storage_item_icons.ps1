param(
    [string]$RawItems = 'dist/storage-all-items-raw',
    [string]$ExtractedIcons = 'dist/storage-all-icons-current',
    [string]$Output = 'web/items'
)
$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot -Parent
function Resolve-WorkspacePath([string]$path) {
    if ([IO.Path]::IsPathRooted($path)) { return $path }
    return Join-Path $root $path
}
$rawRoot = Resolve-WorkspacePath $RawItems
$iconRoot = Resolve-WorkspacePath $ExtractedIcons
$outputRoot = Resolve-WorkspacePath $Output
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null

$iconsByName = @{}
$iconRoots = @($iconRoot)
$nonstandardRoot = Join-Path $root 'dist/storage-nonstandard-icons'
if (Test-Path $nonstandardRoot) { $iconRoots += $nonstandardRoot }
foreach ($sourceRoot in $iconRoots) {
    Get-ChildItem $sourceRoot -Recurse -File -Filter '*.png' | ForEach-Object {
        if (-not $iconsByName.ContainsKey($_.BaseName)) {
            $iconsByName[$_.BaseName] = $_.FullName
        }
    }
}

# Item packages also reference buff, skill, journal, and status icons. Inventory
# icons are the remaining exact texture dependencies. The filter is deliberately
# conservative: ambiguous packages are rejected instead of receiving a wrong icon.
$nonInventory = 'Journal|Buff_|Tag_Skill|Status|Unlock|Placeholder|Category|Recipe|T_Icon_Sml_|T_Icon_Stamina$|T_Icon_Health$'
$mapped = 0
$ambiguous = @()
$unresolved = @()
Get-ChildItem $rawRoot -Recurse -File -Filter 'ITEM_*.uasset' | ForEach-Object {
    $itemName = $_.BaseName
    $text = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($_.FullName))
    $candidates = @(
        [regex]::Matches($text, 'T_[A-Za-z0-9_\-]+') |
            ForEach-Object {
                # Unreal's name table can place adjacent names without a NUL in
                # the raw package. Trim the captured suffix until it exactly
                # matches an extracted texture name.
                $candidate = $_.Value
                while ($candidate.Length -gt 2 -and -not $iconsByName.ContainsKey($candidate)) {
                    $candidate = $candidate.Substring(0, $candidate.Length - 1)
                }
                if ($iconsByName.ContainsKey($candidate)) { $candidate }
            } |
            Where-Object { $_ -notmatch $nonInventory } |
            Sort-Object -Unique
    )
    if ($candidates.Count -gt 1) {
        $preferred = @($candidates | Where-Object { $_ -cmatch '^T_Icon_' })
        if ($preferred.Count -eq 1) { $candidates = $preferred }
    }
    if ($candidates.Count -eq 1) {
        Copy-Item -LiteralPath $iconsByName[$candidates[0]] -Destination (Join-Path $outputRoot "$itemName.png") -Force
        $mapped++
    } elseif ($candidates.Count -gt 1) {
        $ambiguous += [pscustomobject]@{ item = $itemName; candidates = $candidates }
    } else {
        $unresolved += $itemName
    }
}

$report = [ordered]@{
    generatedAt = [DateTime]::UtcNow.ToString('o')
    mapped = $mapped
    ambiguous = $ambiguous
    unresolved = $unresolved
}
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $outputRoot 'manifest.json') -Encoding UTF8
[pscustomobject]@{ Mapped=$mapped; Ambiguous=$ambiguous.Count; Unresolved=$unresolved.Count; OutputPngs=(Get-ChildItem $outputRoot -File -Filter '*.png').Count }
