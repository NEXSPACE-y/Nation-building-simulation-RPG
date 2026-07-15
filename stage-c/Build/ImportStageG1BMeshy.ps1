[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' })
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Python = Join-Path $PSScriptRoot 'ImportStageG1BMeshy.py'
$Base = Join-Path $ProjectRoot 'SourceArt\StageG1B\PLAYER_M\Meshy\v0.1\Working'
$BaseFbx = Join-Path $Base 'FBX\PLAYER_M_Meshy_v0.1_Base_Rigged.fbx'
$AnimationFbx = Join-Path $Base 'FBX\PLAYER_M_Meshy_v0.1_AllAnimations.fbx'
$Reports = Join-Path $Base 'Reports'
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Log = Join-Path $RepositoryRoot 'out\stage-g1b-meshy\import\unreal_import.log'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Log) | Out-Null

foreach ($Required in @($Project, $Editor, $Python, $BaseFbx, $AnimationFbx, $Reports)) {
    if (-not (Test-Path -LiteralPath $Required)) { throw "Missing Stage G-1B import input: $Required" }
}

& $Editor $Project -unattended -nop4 -nosplash -NoSound `
    "-ExecutePythonScript=$Python" "-abslog=$Log"
if ($LASTEXITCODE -ne 0) { throw "Stage G-1B Meshy import failed with exit code $LASTEXITCODE" }
if ((Get-Content -Raw -LiteralPath $Log) -notmatch 'STAGE_G1B_IMPORT PASS') {
    throw 'Stage G-1B Meshy import PASS marker was not found.'
}

Write-Host "STAGE_G1B_MESHY_IMPORT_PASS log=$Log"
