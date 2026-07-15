[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' })
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Python = Join-Path $PSScriptRoot 'GenerateStageG1BMeshyAssets.py'
$Log = Join-Path $RepositoryRoot 'out\stage-g1b-meshy\generate\generate_assets.log'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Log) | Out-Null

foreach ($Required in @($Project, $Editor, $Python)) {
    if (-not (Test-Path -LiteralPath $Required)) { throw "Missing Stage G-1B generation input: $Required" }
}

& $Editor $Project -unattended -nop4 -nosplash -NoSound `
    "-ExecutePythonScript=$Python" "-abslog=$Log"
if ($LASTEXITCODE -ne 0) { throw "Stage G-1B asset generation failed with exit code $LASTEXITCODE" }
if ((Get-Content -Raw -LiteralPath $Log) -notmatch 'STAGE_G1B_ASSETS PASS') {
    throw 'Stage G-1B asset generation PASS marker was not found.'
}

Write-Host "STAGE_G1B_ASSET_GENERATION_PASS log=$Log"
