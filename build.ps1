param(
    [string]$ToolchainBin
)

$ErrorActionPreference = 'Stop'

if (-not $ToolchainBin) {
    $compiler = Get-Command g++.exe -ErrorAction SilentlyContinue
    if ($compiler) {
        $ToolchainBin = Split-Path $compiler.Source
    } else {
        $ToolchainBin = 'C:\msys64\mingw64\bin'
    }
}
$ToolchainBin = (Resolve-Path -LiteralPath $ToolchainBin).Path
$toolchainRoot = Split-Path $ToolchainBin
$compilerPath = Join-Path $ToolchainBin 'g++.exe'
$includePath = Join-Path $toolchainRoot 'include'
$libraryPath = Join-Path $toolchainRoot 'lib'
foreach ($required in @($compilerPath, "$includePath/SDL2/SDL.h", "$includePath/tinyxml2.h")) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Missing $required. Install the matching MSYS2 GCC, SDL2, and TinyXML2 packages; see README.md."
    }
}

$buildPath = Join-Path $PSScriptRoot 'build'
$executable = Join-Path $buildPath 'dungeon-maintenance.exe'
$null = New-Item -ItemType Directory -Force -Path $buildPath
$previousPath = $env:PATH
try {
    $env:PATH = "$ToolchainBin;$previousPath"
    $sources = @(Get-ChildItem -LiteralPath "$PSScriptRoot/src" -Filter '*.cpp' | ForEach-Object FullName)
    & $compilerPath -std=c++17 -O2 -Wall -Wextra -Wpedantic "-I$includePath" @sources "-L$libraryPath" -lSDL2 -ltinyxml2 -o $executable
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed with exit code $LASTEXITCODE." }

    Copy-Item -LiteralPath "$PSScriptRoot/data" -Destination $buildPath -Recurse -Force
    foreach ($dll in @('SDL2.dll', 'libtinyxml2.dll', 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll')) {
        Copy-Item -LiteralPath (Join-Path $ToolchainBin $dll) -Destination $buildPath -Force
    }
} finally {
    $env:PATH = $previousPath
}
Write-Host "Built successfully: $executable"
Write-Host 'Run .\build\dungeon-maintenance.exe, or double-click it in File Explorer.'
