[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' })
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Python = Join-Path $PSScriptRoot 'GenerateStageG1AMap.py'

& $Editor $Project -unattended -nop4 -nosplash -NoSound "-ExecutePythonScript=$Python" -log
if ($LASTEXITCODE -ne 0) { throw "Stage G-1A map generation failed with exit code $LASTEXITCODE" }

$Map = Join-Path $ProjectRoot 'Content\Maps\StageG1_3DCharacterPoC.umap'
if (-not (Test-Path -LiteralPath $Map)) { throw "Stage G-1A map was not generated: $Map" }

Write-Host "STAGE_G1A_MAP_READY $Map"
