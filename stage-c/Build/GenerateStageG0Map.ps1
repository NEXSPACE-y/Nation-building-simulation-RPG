[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' })
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Python = Join-Path $PSScriptRoot 'GenerateStageG0Map.py'

& $Editor $Project -unattended -nop4 -nosplash -NoSound "-ExecutePythonScript=$Python" -log
if ($LASTEXITCODE -ne 0) { throw "Stage G-0 map generation failed with exit code $LASTEXITCODE" }

$Map = Join-Path $ProjectRoot 'Content\Maps\StageG0_VisualPoC.umap'
if (-not (Test-Path -LiteralPath $Map)) { throw "Stage G-0 map was not generated: $Map" }

Write-Host "STAGE_G0_MAP_READY $Map"

