# Deploy freshly-built Meshlets DLLs + shader source to all load locations.
# Run AFTER a successful build AND after the Editor is closed (it locks the install DLL).
param(
    [switch]$SkipDllCheck
)

$ErrorActionPreference = 'Stop'
$buildBin = 'F:\engine\build\windows\bin\profile'
$srcGemAssets = 'F:\engine\Gems\Meshlets\Assets'
$installGemAssets = 'F:\engine\install\Gems\Meshlets\Assets'

# DLL deploy targets (both game Meshlets.dll and editor Meshlets.Editor.dll).
$dllTargets = @(
    'F:\engine\install\bin\Windows\profile\Default',
    'F:\engine\install\bin\Windows\profile\Default\EditorPlugins',
    'F:\engine-projects\o3de-multiplayersample\build\windows\bin\profile'
)
$dlls = @('Meshlets.dll', 'Meshlets.Editor.dll')

# Abort early if the install editor DLL is locked (editor still running).
$lockProbe = 'F:\engine\install\bin\Windows\profile\Default\Meshlets.Editor.dll'
if (-not $SkipDllCheck -and (Test-Path $lockProbe)) {
    try {
        $fs = [System.IO.File]::Open($lockProbe, 'Open', 'ReadWrite', 'None')
        $fs.Close()
    } catch {
        Write-Error "LOCKED: $lockProbe is in use (Editor running?). Close the Editor first."
        exit 1
    }
}

foreach ($dll in $dlls) {
    $src = Join-Path $buildBin $dll
    if (-not (Test-Path $src)) { Write-Warning "missing build output: $src"; continue }
    foreach ($t in $dllTargets) {
        if (-not (Test-Path $t)) { Write-Warning "missing target dir: $t"; continue }
        Copy-Item $src (Join-Path $t $dll) -Force
        Write-Output ("copied {0} -> {1}" -f $dll, $t)
    }
}

# Shader/pass source sync: copy the whole Assets tree so any changed azsl/pass/azsli
# lands in the install gem the mp-sample resolves against (AP recompiles from here).
Copy-Item "$srcGemAssets\*" $installGemAssets -Recurse -Force
Write-Output "synced Assets/ -> install gem"
Write-Output "DONE. Now run AssetProcessor for the mp-sample project to recompile shaders."
