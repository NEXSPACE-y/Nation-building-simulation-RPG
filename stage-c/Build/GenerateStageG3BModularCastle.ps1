[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [string]$Blender = 'C:\Program Files\Blender Foundation\Blender 5.1\blender.exe'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Generator = Join-Path $PSScriptRoot 'GenerateStageG3BModularCastle.py'
$ModuleGenerator = Join-Path $PSScriptRoot 'GenerateStageG3BModularCastleModules.ps1'
$G3AMap = Join-Path $ProjectRoot 'Content\Maps\StageG3A_CapitalBlockout_PoC.umap'
$Output = Join-Path $RepositoryRoot 'out\stage-g3br-modular-castle\generation'
New-Item -ItemType Directory -Force -Path $Output | Out-Null

foreach ($Required in @($Project, $Build, $Editor, $Generator, $ModuleGenerator, $G3AMap)) {
    if (-not (Test-Path -LiteralPath $Required)) {
        throw "Required G-3B-R generation input is missing: $Required"
    }
}

$G3ABefore = (Get-FileHash -LiteralPath $G3AMap -Algorithm SHA256).Hash
& $ModuleGenerator -Blender $Blender
if ($LASTEXITCODE -ne 0) { throw 'G-3B-R modular FBX generation failed.' }

$BuildLog = Join-Path $Output 'editor_build.log'
& $Build NationSimulationStageCEditor Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1 *> $BuildLog
if ($LASTEXITCODE -ne 0) {
    throw "G-3B-R Editor build failed with exit code $LASTEXITCODE. Log: $BuildLog"
}

$GenerateLog = Join-Path $Output 'map_generation.log'
& $Editor $Project -unattended -nop4 -nosplash -nullrhi -NoSound `
    "-ExecutePythonScript=$Generator" "-abslog=$GenerateLog"
if ($LASTEXITCODE -ne 0) {
    throw "G-3B-R map generation failed with exit code $LASTEXITCODE. Log: $GenerateLog"
}
if ((Get-Content -Raw -LiteralPath $GenerateLog) -notmatch 'STAGE_G3BR_MODULAR_MAP PASS') {
    throw 'G-3B-R map generation PASS marker is missing.'
}

$G3AAfter = (Get-FileHash -LiteralPath $G3AMap -Algorithm SHA256).Hash
if ($G3ABefore -ne $G3AAfter) {
    throw 'Accepted Stage G-3A map changed during G-3B-R generation.'
}

Write-Host "STAGE_G3BR_MODULAR_GENERATION_PASS g3a_sha=$G3AAfter"
