[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' })
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Python = Join-Path $PSScriptRoot 'ImportStageG1BAdditionalAnimations.py'
$IdleFbx = Join-Path $ProjectRoot 'SourceArt\StageG1B\PLAYER_M\Meshy\v0.1\AdditionalAnimations\Working\IDLE\PLAYER_M_Meshy_v0.1_IDLE_Idle11.fbx'
$DashFbx = Join-Path $ProjectRoot 'SourceArt\StageG1B\PLAYER_M\Meshy\v0.1\AdditionalAnimations\Working\DASH\PLAYER_M_Meshy_v0.1_DASH_Run02.fbx'
$Log = Join-Path $RepositoryRoot 'out\stage-g1b-meshy\additional-animations\unreal_import.log'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Log) | Out-Null

foreach ($Required in @($Project, $Editor, $Python, $IdleFbx, $DashFbx)) {
    if (-not (Test-Path -LiteralPath $Required)) {
        throw "Missing Stage G-1B additional-animation input: $Required"
    }
}

& $Editor $Project -unattended -nop4 -nosplash -NoSound -nullrhi `
    "-ExecutePythonScript=$Python" "-abslog=$Log"
if ($LASTEXITCODE -ne 0) {
    throw "Stage G-1B additional-animation import failed with exit code $LASTEXITCODE"
}
if ((Get-Content -Raw -LiteralPath $Log) -notmatch 'STAGE_G1B_ADDITIONAL_IMPORT PASS') {
    throw 'Stage G-1B additional-animation import PASS marker was not found.'
}

Write-Host "STAGE_G1B_ADDITIONAL_IMPORT_PASS log=$Log"
