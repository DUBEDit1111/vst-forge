# get-pluginval.ps1 - prints the path to pluginval.exe, downloading the latest release
# from https://github.com/Tracktion/pluginval into %LOCALAPPDATA%\pluginval if needed.
# Set the PLUGINVAL environment variable to use your own copy.
$ErrorActionPreference = "Stop"
if ($env:PLUGINVAL -and (Test-Path $env:PLUGINVAL)) { return $env:PLUGINVAL }
$dir = Join-Path $env:LOCALAPPDATA "pluginval"
$exe = Join-Path $dir "pluginval.exe"
if (-not (Test-Path $exe)) {
    New-Item -ItemType Directory -Force $dir | Out-Null
    $zip = Join-Path $dir "pluginval_Windows.zip"
    Write-Host "Downloading pluginval..." -ForegroundColor Cyan
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -UseBasicParsing "https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_Windows.zip" -OutFile $zip
    Expand-Archive -Force $zip $dir
    Remove-Item $zip
}
if (-not (Test-Path $exe)) { throw "pluginval download failed: $exe not found" }
return $exe
