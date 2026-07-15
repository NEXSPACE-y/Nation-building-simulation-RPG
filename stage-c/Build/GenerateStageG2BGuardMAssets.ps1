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
$Python = Join-Path $PSScriptRoot 'GenerateStageG2BGuardMAssets.py'
$SourceMap = Join-Path $ProjectRoot 'Content\Maps\StageG2A_CameraRedesignPoC.umap'
$TargetMap = Join-Path $ProjectRoot 'Content\Maps\StageG2B_GuardM_PoC.umap'
$ExpectedSourceMapSha = '23D273BB49538EA1365DBDBC67074701B83CC5CBC8C632BB73E27B241196C6E6'
$Log = Join-Path $RepositoryRoot 'out\stage-g2b-guardm\generate\generate_map.log'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Log) | Out-Null

foreach ($Required in @($Project,$Build,$Editor,$Python,$SourceMap)) {
    if (-not (Test-Path -LiteralPath $Required)) { throw "Missing Stage G-2B generation input: $Required" }
}
if ((Get-FileHash -LiteralPath $SourceMap -Algorithm SHA256).Hash -ne $ExpectedSourceMapSha) {
    throw 'Accepted Stage G-2A map SHA changed before Stage G-2B generation.'
}

& $Build NationSimulationStageCEditor Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1
if ($LASTEXITCODE -ne 0) { throw "Stage G-2B Editor build failed with exit code $LASTEXITCODE" }

& $Editor $Project -unattended -nop4 -nosplash -NoSound `
    "-ExecutePythonScript=$Python" "-abslog=$Log"
if ($LASTEXITCODE -ne 0) { throw "Stage G-2B asset generation failed with exit code $LASTEXITCODE" }
if ((Get-Content -Raw -LiteralPath $Log) -notmatch 'STAGE_G2B_GUARD_M_ASSETS PASS') {
    throw 'Stage G-2B asset generation PASS marker was not found.'
}
if (-not (Test-Path -LiteralPath $TargetMap)) { throw 'Stage G-2B target map was not generated.' }
if ((Get-FileHash -LiteralPath $SourceMap -Algorithm SHA256).Hash -ne $ExpectedSourceMapSha) {
    throw 'Accepted Stage G-2A map changed during Stage G-2B generation.'
}

Write-Host "STAGE_G2B_GUARD_M_ASSET_GENERATION_PASS target=$TargetMap"
