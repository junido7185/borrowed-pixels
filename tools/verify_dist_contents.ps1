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

Write-Output 'DIST_WHITELIST_STATUS=PASS'
exit 0
