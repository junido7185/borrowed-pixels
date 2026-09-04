[CmdletBinding()]
param(
    [string]$EvidenceDir = 'docs/evidence/T14'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$evidence = Join-Path $repoRoot $EvidenceDir
New-Item -ItemType Directory -Force -Path $evidence | Out-Null
$log = Join-Path $evidence 'full-validation.verbose.log'

function Invoke-Checked([string]$Name, [scriptblock]$Command) {
    Write-Output "RC_STEP=$Name"
    & $Command
    if ($LASTEXITCODE -ne 0) { throw "$Name failed with exit $LASTEXITCODE" }
    Write-Output "RC_STEP_RESULT=$Name PASS"
}

Push-Location $repoRoot
Start-Transcript -Path $log -Force | Out-Null
try {
    Invoke-Checked 'AUTHORITY' { powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify_authority.ps1 }
    Invoke-Checked 'CONFIGURE' { cmake --preset windows-msvc }
    Invoke-Checked 'TEST_BUILD' { cmake --build --preset test --clean-first }
    Invoke-Checked 'DIRECT_TEST' { & .\build\windows-msvc\bin\Test\bp_test.exe }
    Invoke-Checked 'CTEST' { ctest --preset all --output-on-failure }
    Invoke-Checked 'DEBUG_BUILD' { cmake --build --preset debug --clean-first }
    Invoke-Checked 'DEBUG_SMOKE' { & .\build\windows-msvc\bin\Debug\bp_debug.exe --smoke-test }
    $mapPath = Join-Path $evidence 'rc-final.map'
    $previousLink = $env:LINK
    $env:LINK = "/MAP:$mapPath"
    try {
        Invoke-Checked 'CONTEST_BUILD' { cmake --build --preset contest --clean-first }
    } finally {
        $env:LINK = $previousLink
    }
    if (-not (Test-Path -LiteralPath $mapPath -PathType Leaf)) { throw 'Contest MAP was not produced.' }
    $mapText = Get-Content -Raw -LiteralPath $mapPath
    $reachable = @('wWinMain', 'RenderPrecedentRuntime', 'RenderNarrativeRuntime',
        'RenderOptionsRuntime', 'RenderAudioFrames', 'RuntimeStrings')
    $reachability = foreach ($symbol in $reachable) {
        $present = $mapText.Contains($symbol)
        "SYMBOL=$symbol REACHABLE=$present"
        if (-not $present) { throw "Required contest symbol is not reachable: $symbol" }
    }
    $reachability | Set-Content -Encoding ASCII -LiteralPath (Join-Path $evidence 'opt-ref-reachability.txt')
    Write-Output 'OPT_REF_REACHABILITY=PASS'
    Invoke-Checked 'DIST_WHITELIST' {
        powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify_dist_contents.ps1 -Path dist/contest
    }
    Invoke-Checked 'DIST_BYTES' {
        powershell -NoProfile -ExecutionPolicy Bypass -File tools/measure_dist.ps1 `
            -Path dist/contest -InternalCap 1350000 -HardCap 1474560
    }

    $exe = (Resolve-Path '.\dist\contest\BorrowedPixels.exe').Path
    $process = Start-Process -FilePath $exe -PassThru
    $live = $false
    for ($attempt = 0; $attempt -lt 80; ++$attempt) {
        Start-Sleep -Milliseconds 100
        $process.Refresh()
        if ($process.MainWindowHandle -ne 0) { $live = $true; break }
        if ($process.HasExited) { break }
    }
    if (-not $live) { throw 'Contest EXE did not expose a live window.' }
    $liveHandle = $process.MainWindowHandle
    Start-Sleep -Milliseconds 500

    Add-Type -AssemblyName System.Drawing
    if (-not ('BpRcCapture' -as [type])) {
        Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class BpRcCapture {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT point);
}
'@
    }
    $rect = New-Object BpRcCapture+RECT
    $point = New-Object BpRcCapture+POINT
    if (-not [BpRcCapture]::GetClientRect($process.MainWindowHandle, [ref]$rect) -or
        -not [BpRcCapture]::ClientToScreen($process.MainWindowHandle, [ref]$point)) {
        throw 'Unable to resolve contest client rectangle.'
    }
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if ($width -le 0 -or $height -le 0) { throw 'Contest client rectangle is empty.' }
    $bitmap = [System.Drawing.Bitmap]::new($width, $height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($point.X, $point.Y, 0, 0, $bitmap.Size)
        $bitmap.Save((Join-Path $evidence 'contest-live.bmp'), [System.Drawing.Imaging.ImageFormat]::Bmp)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
    $null = $process.CloseMainWindow()
    if (-not $process.WaitForExit(5000)) { Stop-Process -Id $process.Id; throw 'Contest EXE did not close.' }
    if ($process.ExitCode -ne 0) { throw "Contest EXE exited $($process.ExitCode)." }
    Write-Output "LIVE_EXE_STATUS=PASS HANDLE=$liveHandle CAPTURE=contest-live.bmp"

    $hash = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()
    $bytes = (Get-Item -LiteralPath $exe).Length
    "EXE_PATH=$exe`nEXE_BYTES=$bytes`nEXE_SHA256=$hash" |
        Set-Content -Encoding ASCII -LiteralPath (Join-Path $evidence 'rc-artifact.txt')
    Write-Output "EXE_BYTES=$bytes"
    Write-Output "EXE_SHA256=$hash"

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'vswhere.exe is unavailable.' }
    $installation = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or -not $installation) { throw 'MSVC installation was not found.' }
    $dumpbin = Get-ChildItem -LiteralPath (Join-Path $installation 'VC\Tools\MSVC') `
        -Filter dumpbin.exe -File -Recurse | Where-Object { $_.FullName -match 'Hostx64\\x64' } |
        Sort-Object FullName -Descending | Select-Object -First 1
    if (-not $dumpbin) { throw 'x64 dumpbin.exe was not found.' }
    & $dumpbin.FullName /dependents $exe | Set-Content -Encoding ASCII -LiteralPath (Join-Path $evidence 'dependencies.txt')
    if ($LASTEXITCODE -ne 0) { throw "DEPENDENCIES failed with exit $LASTEXITCODE" }
    Write-Output 'DEPENDENCIES_STATUS=PASS'
    Write-Output 'RC_VALIDATION_STATUS=PASS'
} finally {
    if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
    Stop-Transcript | Out-Null
    Pop-Location
}
