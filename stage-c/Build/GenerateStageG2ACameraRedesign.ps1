[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' })
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Python = Join-Path $PSScriptRoot 'GenerateStageG2ACameraRedesign.py'
$Log = Join-Path $RepositoryRoot 'out\stage-g2a-redesign\generate\generate_map.log'
$AcceptedMap = Join-Path $ProjectRoot 'Content\Maps\StageG1B_OriginalPlayerPoC.umap'
$TargetMap = Join-Path $ProjectRoot 'Content\Maps\StageG2A_CameraRedesignPoC.umap'
$ExpectedAcceptedMapSha = '623AF018D2682CA9CB573CA891CD6DF1B118584F260623282E2F874582FD062E'

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Log) | Out-Null
foreach ($Required in @($Project,$Build,$Editor,$Python,$AcceptedMap)) {
    if (-not (Test-Path -LiteralPath $Required)) { throw "Missing Stage G-2A redesign generation input: $Required" }
}
if ((Get-FileHash -LiteralPath $AcceptedMap -Algorithm SHA256).Hash -ne $ExpectedAcceptedMapSha) {
    throw 'Accepted Stage G-1B map SHA changed before Stage G-2A redesign generation.'
}

& $Build NationSimulationStageCEditor Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1
if ($LASTEXITCODE -ne 0) { throw "Stage G-2A redesign Editor build failed with exit code $LASTEXITCODE" }

& $Editor $Project -unattended -nop4 -nosplash -NoSound `
    "-ExecutePythonScript=$Python" "-abslog=$Log"
if ($LASTEXITCODE -ne 0) { throw "Stage G-2A redesign map generation failed with exit code $LASTEXITCODE" }
if ((Get-Content -Raw -LiteralPath $Log) -notmatch 'STAGE_G2A_REDESIGN_MAP PASS') {
    throw 'Stage G-2A redesign map PASS marker was not found.'
}
if (-not (Test-Path -LiteralPath $TargetMap)) { throw 'Stage G-2A redesign target map was not generated.' }
if ((Get-FileHash -LiteralPath $AcceptedMap -Algorithm SHA256).Hash -ne $ExpectedAcceptedMapSha) {
    throw 'Accepted Stage G-1B map changed during Stage G-2A redesign generation.'
}

Write-Host "STAGE_G2A_REDESIGN_MAP_GENERATION_PASS target=$TargetMap"
