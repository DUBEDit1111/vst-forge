# build.ps1 - Windows: configure, build, run both test batteries, run pluginval, optionally install.
#   powershell -ExecutionPolicy Bypass -File scripts\build.ps1            # build + test + validate
#   powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Install   # ...then copy to Common Files\VST3 (admin)
# Close your DAW first: a loaded .vst3 is locked and the build/copy fails (LNK1104 / access denied).
param(
    [switch]$Install,
    [string]$Juce = "",          # optional local JUCE checkout (skips the git download)
    [int]$Seeds = 3              # pluginval runs with different random seeds
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$name = (Select-String -Path CMakeLists.txt -Pattern '^juce_add_plugin\((\w+)').Matches[0].Groups[1].Value

$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmake) {
    $vsCmake = Get-ChildItem "C:\Program Files*\Microsoft Visual Studio\*\*\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($vsCmake) { $cmake = $vsCmake.FullName } else { throw "CMake not found. Install it (winget install Kitware.CMake) or Visual Studio (2022 or newer) with the C++ workload." }
}

function Run($exe, [string[]]$a) {
    & $exe @a
    if ($LASTEXITCODE -ne 0) { throw "FAILED ($LASTEXITCODE): $exe $($a -join ' ')" }
}

if (-not (Test-Path build\CMakeCache.txt)) {
    # Use the newest Visual Studio with C++ tools (2022 = 17, 2026 = 18, ...), not a hard-coded one.
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found. Install Visual Studio (2022 or newer) with the 'Desktop development with C++' workload." }
    $vsVer = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion
    $vsYear = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property catalog_productLineVersion
    if (-not $vsVer) { throw "No Visual Studio with C++ tools found. Add the 'Desktop development with C++' workload." }
    $gen = "Visual Studio $($vsVer.Split('.')[0]) $vsYear"
    Write-Host "Generator: $gen"
    $cfg = @("-S", ".", "-B", "build", "-G", $gen, "-A", "x64")
    if ($Juce) { $cfg += "-DFETCHCONTENT_SOURCE_DIR_JUCE=$Juce" }
    Run $cmake $cfg
}
Run $cmake @("--build", "build", "--config", "Release", "--parallel", "--target", "EngineTests", "ProcessorTests", "${name}_VST3", "${name}_Standalone")

Write-Host "`n== Engine tests" -ForegroundColor Cyan
Run "build\Release\EngineTests.exe" @()
Write-Host "`n== Processor tests" -ForegroundColor Cyan
Run "build\ProcessorTests_artefacts\Release\ProcessorTests.exe" @()

$vst = (Resolve-Path "build\${name}_artefacts\Release\VST3\${name}.vst3").Path
$pv = & "$PSScriptRoot\get-pluginval.ps1"
Write-Host "`n== pluginval strictness 10, $Seeds seeds" -ForegroundColor Cyan
for ($i = 1; $i -le $Seeds; $i++) {
    $seed = "0x{0:x}" -f (0x1111 * $i)
    # Start-Process -Wait: plain & on pluginval (a GUI-subsystem exe) does not wait and leaves a stale exit code
    $p = Start-Process -FilePath $pv -ArgumentList @("--strictness-level", "10", "--validate-in-process", "--skip-gui-tests", "--seed", $seed, "--validate", "`"$vst`"") -NoNewWindow -Wait -PassThru
    if ($p.ExitCode -ne 0) { throw "pluginval FAILED on seed $seed (exit $($p.ExitCode))" }
    Write-Host "seed $seed passed" -ForegroundColor Green
}

Write-Host "`n*** BUILD OK, TESTS PASS, PLUGINVAL 10 PASS ***" -ForegroundColor Green
Write-Host "Plugin: $vst"
if ($Install) {
    $dest = "C:\Program Files\Common Files\VST3\${name}.vst3"
    if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
    Copy-Item -Recurse $vst $dest
    Write-Host "Installed to $dest - rescan plug-ins in your DAW." -ForegroundColor Green
}
