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
$Python = Join-Path $PSScriptRoot 'GenerateStageG3ACapitalBlockout.py'
$SourceMap = Join-Path $ProjectRoot 'Content\Maps\StageG2B_GuardM_PoC.umap'
$TargetMap = Join-Path $ProjectRoot 'Content\Maps\StageG3A_CapitalBlockout_PoC.umap'
$ReferenceImage = Join-Path $RepositoryRoot 'Docs\ChatGPT Image 2026年7月11日 21_50_21.png'
$ExpectedSourceMapSha = '71FE747D80E83C990D328FF879FA3AF837D2F01724D35F66C548EBD8AC069D59'
$Log = Join-Path $RepositoryRoot 'out\stage-g3a-capital-blockout\generate\generate_map.log'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Log) | Out-Null

foreach ($Required in @($Project,$Build,$Editor,$Python,$SourceMap,$ReferenceImage)) {
    if (-not (Test-Path -LiteralPath $Required)) { throw "Missing Stage G-3A generation input: $Required" }
}
if ((Get-FileHash -LiteralPath $SourceMap -Algorithm SHA256).Hash -ne $ExpectedSourceMapSha) {
    throw 'Accepted Stage G-2B map SHA changed before Stage G-3A generation.'
}

& $Build NationSimulationStageCEditor Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1
if ($LASTEXITCODE -ne 0) { throw "Stage G-3A Editor build failed with exit code $LASTEXITCODE" }

& $Editor $Project -unattended -nop4 -nosplash -NoSound `
    "-ExecutePythonScript=$Python" "-abslog=$Log"
if ($LASTEXITCODE -ne 0) { throw "Stage G-3A map generation failed with exit code $LASTEXITCODE" }
if ((Get-Content -Raw -LiteralPath $Log) -notmatch 'STAGE_G3A_CAPITAL_MAP PASS') {
    throw 'Stage G-3A map generation PASS marker was not found.'
}
if (-not (Test-Path -LiteralPath $TargetMap)) { throw 'Stage G-3A target map was not generated.' }
if ((Get-FileHash -LiteralPath $SourceMap -Algorithm SHA256).Hash -ne $ExpectedSourceMapSha) {
    throw 'Accepted Stage G-2B map changed during Stage G-3A generation.'
}

$Large = @(Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'Content\StageG3A') -File -Recurse |
    Where-Object Length -gt 50MB)
if ($Large.Count -ne 0) { throw 'Stage G-3A generated a new Content asset over 50MB.' }

Write-Host "STAGE_G3A_CAPITAL_ASSET_GENERATION_PASS target=$TargetMap"
