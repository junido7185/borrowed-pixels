[CmdletBinding()]
param(
    [string]$Path = 'dist/contest'
)

$ErrorActionPreference = 'Stop'
$allowed = @('BorrowedPixels.exe', 'LICENSES.txt', 'README.txt')

if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
    Write-Error "Distribution directory does not exist: $Path"
    exit 2
}

$root = (Resolve-Path -LiteralPath $Path).Path
$files = @(Get-ChildItem -LiteralPath $root -File -Recurse -Force)
$actual = @($files | ForEach-Object {
    $_.FullName.Substring($root.Length).TrimStart('\', '/') -replace '\\', '/'
} | Sort-Object)

$unexpected = @($actual | Where-Object { $_ -notin $allowed })
$missing = @($allowed | Where-Object { $_ -notin $actual })

foreach ($file in $actual) { Write-Output "DIST_FILE=$file" }
if ($unexpected.Count -ne 0) {
    foreach ($file in $unexpected) { Write-Output "UNEXPECTED=$file" }
}
if ($missing.Count -ne 0) {
    foreach ($file in $missing) { Write-Output "MISSING=$file" }
}
if ($unexpected.Count -ne 0 -or $missing.Count -ne 0) {
    Write-Output 'DIST_WHITELIST_STATUS=FAIL'
    exit 1
}

$readmePath = Join-Path $root 'README.txt'
$readmeBytes = [System.IO.File]::ReadAllBytes($readmePath)
if ($readmeBytes.Length -ge 3 -and $readmeBytes[0] -eq 0xEF `
        -and $readmeBytes[1] -eq 0xBB -and $readmeBytes[2] -eq 0xBF) {
    Write-Output 'README_UTF8_BOM=FAIL'
    exit 1
}
$readme = [System.Text.Encoding]::UTF8.GetString($readmeBytes)
$total = ($files | Measure-Object -Property Length -Sum).Sum
if ($readme -notmatch '[\uAC00-\uD7A3]' -or -not $readme.Contains('[ENGLISH]') `
        -or -not $readme.Contains('Windows 10/11 x64') `
        -or -not $readme.Contains('Options/accessibility') `
        -or -not $readme.Contains("EXTRACTED_TOTAL_BYTES=$total")) {
    Write-Output "README_CONTENT_STATUS=FAIL EXPECTED_TOTAL=$total"
    exit 1
}
Write-Output 'README_UTF8_BOM=PASS'
Write-Output "README_CONTENT_STATUS=PASS EXTRACTED_TOTAL_BYTES=$total"
Write-Output 'DIST_WHITELIST_STATUS=PASS'
exit 0
