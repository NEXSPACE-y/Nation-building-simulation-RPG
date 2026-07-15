[CmdletBinding()]
param(
    [string]$SourceZip = 'C:\Users\rinpa\Desktop\Meshy_AI_Lioncrest_Knight_biped.zip',
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' })
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Python = Join-Path $PSScriptRoot 'PrepareStageG2BGuardMSource.py'
$PythonExe = Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\Python3\Win64\python.exe'
$Base = Join-Path $ProjectRoot 'SourceArt\StageG2B\GUARD_M\Meshy\v0.1'

foreach ($Required in @($SourceZip, $Python, $PythonExe)) {
    if (-not (Test-Path -LiteralPath $Required)) { throw "Missing Stage G-2B source input: $Required" }
}

& $PythonExe $Python --source-zip $SourceZip --base $Base
if ($LASTEXITCODE -ne 0) { throw "Stage G-2B source preparation failed with exit code $LASTEXITCODE" }

$Manifest = Join-Path $Base 'Working\Manifest\GUARD_M_Meshy_v0.1_SourceManifest.json'
if (-not (Test-Path -LiteralPath $Manifest)) { throw 'Stage G-2B source manifest was not generated.' }
$Data = Get-Content -Raw -LiteralPath $Manifest | ConvertFrom-Json
if ($Data.validation_status -ne 'PASS' -or $Data.internal_file_count -ne 6 -or $Data.clean_pack_used) {
    throw 'Stage G-2B source manifest contract failed.'
}

Write-Host "STAGE_G2B_GUARD_M_SOURCE_PASS manifest=$Manifest"
