[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$manifestPath = Join-Path $repoRoot 'docs\authority\BORROWED_PIXELS_AUTHORITY_v2.1.md'

if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    Write-Error "Authority manifest does not exist: $manifestPath"
    exit 1
}

$manifest = Get-Content -Raw -Encoding UTF8 -LiteralPath $manifestPath
$requiredFiles = @(
    'BORROWED_PIXELS_RUNTIME_CONTRACT_v2.1.json',
    'BORROWED_PIXELS_FONT_GLYPHS_v2.1.json',
    'BORROWED_PIXELS_FINAL_PRODUCTION_SPEC_v2.1.md',
    'BORROWED_PIXELS_AGENT_RUNBOOK_v2.1.md',
    'BORROWED_PIXELS_UI_REFERENCE_ATLAS_v2.1.svg',
    'BORROWED_PIXELS_UI_REFERENCE_ATLAS_v2.1.png'
)

$failed = $false

foreach ($fileName in $requiredFiles) {
    $escapedName = [Regex]::Escape($fileName)
    $pattern = '(?s)`' + $escapedName + '`.{0,240}?`(?<hash>[0-9a-fA-F]{64})`'
    $match = [Regex]::Match($manifest, $pattern)

    if (-not $match.Success) {
        Write-Output ("FAIL`tMISSING_MANIFEST_HASH`t{0}" -f $fileName)
        $failed = $true
        continue
    }

    $filePath = Join-Path $repoRoot (Join-Path 'docs\authority' $fileName)
    if (-not (Test-Path -LiteralPath $filePath -PathType Leaf)) {
        Write-Output ("FAIL`tMISSING_FILE`t{0}" -f $fileName)
        $failed = $true
        continue
    }

    $expected = $match.Groups['hash'].Value.ToLowerInvariant()
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $filePath).Hash.ToLowerInvariant()
    if ($actual -ne $expected) {
        Write-Output ("FAIL`tEXPECTED={0}`tACTUAL={1}`t{2}" -f $expected, $actual, $fileName)
        $failed = $true
        continue
    }

    Write-Output ("PASS`t{0}`t{1}" -f $actual, $fileName)
}

$manifestHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $manifestPath).Hash.ToLowerInvariant()
Write-Output ("INFO`tMANIFEST_SHA256={0}" -f $manifestHash)

if ($failed) {
    exit 1
}

Write-Output 'AUTHORITY_STATUS=PASS'
exit 0
