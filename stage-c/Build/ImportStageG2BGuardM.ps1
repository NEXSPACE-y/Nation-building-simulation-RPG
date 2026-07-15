[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' })
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Python = Join-Path $PSScriptRoot 'ImportStageG2BGuardM.py'
$Working = Join-Path $ProjectRoot 'SourceArt\StageG2B\GUARD_M\Meshy\v0.1\Working'
$Log = Join-Path $RepositoryRoot 'out\stage-g2b-guardm\import\unreal_import.log'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Log) | Out-Null

$Required = @(
    $Project,$Editor,$Python,
    (Join-Path $Working 'FBX\GUARD_M_Meshy_v0.1_Base_Rigged.fbx'),
    (Join-Path $Working 'FBX\GUARD_M_Meshy_v0.1_AllAnimations.fbx'),
    (Join-Path $Working 'Textures\T_GUARD_M_BaseColor_2K.png'),
    (Join-Path $Working 'Textures\T_GUARD_M_Normal_2K.png'),
    (Join-Path $Working 'Textures\T_GUARD_M_Metallic_2K.png'),
    (Join-Path $Working 'Textures\T_GUARD_M_Roughness_2K.png')
)
foreach ($Path in $Required) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Missing Stage G-2B import input: $Path" }
}

& $Editor $Project -unattended -nop4 -nosplash -NoSound `
    "-ExecutePythonScript=$Python" "-abslog=$Log"
if ($LASTEXITCODE -ne 0) { throw "Stage G-2B GUARD_M import failed with exit code $LASTEXITCODE" }
if ((Get-Content -Raw -LiteralPath $Log) -notmatch 'STAGE_G2B_IMPORT PASS') {
    throw 'Stage G-2B GUARD_M import PASS marker was not found.'
}

Write-Host "STAGE_G2B_GUARD_M_IMPORT_PASS log=$Log"
