[CmdletBinding()]
param([long]$InternalCap = 1350000)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    $stage = Join-Path $root 'build/ci/dist'
    $manifestPath = Join-Path $root 'build/ci/sha256.txt'
    if (Test-Path -LiteralPath $manifestPath -PathType Leaf) {
        Remove-Item -LiteralPath $manifestPath -Force
    }
    New-Item -ItemType Directory -Path $stage -Force | Out-Null
    foreach ($name in @('BorrowedPixels.exe','README.txt','LICENSES.txt')) {
        Copy-Item -LiteralPath (Join-Path 'dist/contest' $name) -Destination $stage
    }
    # The archived README contains an old byte total. Update only the staged copy.
    $readme = Join-Path $stage 'README.txt'
    $utf8 = [System.Text.UTF8Encoding]::new($false)
    $text = [IO.File]::ReadAllText($readme, $utf8)
    $pattern = 'EXTRACTED_TOTAL_BYTES=\d+'
    if ([regex]::Matches($text, $pattern).Count -ne 1) { throw 'Expected exactly one byte-total field' }
    $otherBytes = (Get-Item (Join-Path $stage 'BorrowedPixels.exe')).Length + (Get-Item (Join-Path $stage 'LICENSES.txt')).Length
    $total = [long]0
    $stable = $false
    for ($i = 0; $i -lt 10; $i++) {
        $updated = [regex]::Replace($text, $pattern, "EXTRACTED_TOTAL_BYTES=$total")
        $next = $otherBytes + $utf8.GetByteCount($updated)
        if ($next -eq $total) { $stable = $true; break }
        $total = $next
    }
    if (-not $stable) { throw 'Byte-total field did not converge' }
    [IO.File]::WriteAllText($readme, $updated, $utf8)
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify_dist_contents.ps1 -Path $stage
    if ($LASTEXITCODE -ne 0) { throw "Distribution contents failed: $LASTEXITCODE" }
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/measure_dist.ps1 -Path $stage -InternalCap $InternalCap -HardCap 1474560
    if ($LASTEXITCODE -ne 0) { throw "Distribution size failed: $LASTEXITCODE" }
    $manifest = foreach ($file in (Get-ChildItem -LiteralPath $stage -File | Sort-Object Name)) {
        '{0}  {1}' -f (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant(), $file.Name
    }
    $manifest | Set-Content -Encoding ascii -LiteralPath $manifestPath
} finally { Pop-Location }
