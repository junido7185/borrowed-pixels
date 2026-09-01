[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Path,

    [Parameter(Mandatory = $true)]
    [Int64]$InternalCap,

    [Parameter(Mandatory = $true)]
    [Int64]$HardCap
)

$ErrorActionPreference = 'Stop'

if ($InternalCap -lt 0 -or $HardCap -lt 0 -or $InternalCap -gt $HardCap) {
    Write-Error 'Caps must be non-negative and InternalCap must not exceed HardCap.'
    exit 3
}

if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
    Write-Error "Distribution directory does not exist: $Path"
    exit 3
}

$resolvedPath = (Resolve-Path -LiteralPath $Path).Path.TrimEnd('\', '/')
$files = @(Get-ChildItem -LiteralPath $resolvedPath -File -Recurse -Force | Sort-Object -Property FullName)
$totalBytes = [Int64]0

foreach ($file in $files) {
    $relativePath = $file.FullName.Substring($resolvedPath.Length).TrimStart('\', '/')
    $totalBytes += [Int64]$file.Length
    Write-Output ("FILE={0}`tBYTES={1}" -f $relativePath, $file.Length)
}

Write-Output ("DIST_PATH={0}" -f $resolvedPath)
Write-Output ("DIST_FILES={0}" -f $files.Count)
Write-Output ("DIST_BYTES={0}" -f $totalBytes)
Write-Output ("INTERNAL_CAP={0}" -f $InternalCap)
Write-Output ("HARD_CAP={0}" -f $HardCap)

if ($totalBytes -gt $HardCap) {
    Write-Output 'DIST_STATUS=HARD_CAP_EXCEEDED'
    exit 2
}

if ($totalBytes -gt $InternalCap) {
    Write-Output 'DIST_STATUS=RC_CAP_EXCEEDED'
    exit 1
}

Write-Output 'DIST_STATUS=PASS'
exit 0
