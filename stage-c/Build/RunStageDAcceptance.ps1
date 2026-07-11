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

& $Build NationSimulationStageCEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw "UnrealBuildTool failed with exit code $LASTEXITCODE" }

& $Editor $Project -unattended -nop4 -NullRHI -nosplash -NoSound `
    '-ExecCmds=Automation RunTests NationSimulation.Stage;Quit' `
    '-TestExit=Automation Test Queue Empty' -log
if ($LASTEXITCODE -ne 0) { throw "Stage C/D Automation failed with exit code $LASTEXITCODE" }

$StageC = Join-Path $RepositoryRoot 'out\stage-c\test-output\acceptance_results.txt'
$StageD = Join-Path $RepositoryRoot 'out\stage-d\test-output\stage_d_acceptance_results.txt'
$StageDFix = Join-Path $RepositoryRoot 'out\stage-d\fix-output\stage_d_fix_results.txt'
if ((Get-Content -LiteralPath $StageC | Select-Object -Last 1) -ne 'SUMMARY | 11/11 tests passed') {
    throw 'Existing Stage C 11/11 acceptance was not maintained.'
}
if ((Get-Content -LiteralPath $StageD | Select-Object -Last 1) -ne 'SUMMARY | 9/9 tests passed') {
    throw 'Stage D 9/9 acceptance failed.'
}
if ((Get-Content -LiteralPath $StageDFix | Select-Object -Last 1) -ne 'SUMMARY | 8/8 tests passed') {
    throw 'Stage D fixes 8/8 acceptance failed.'
}

Write-Host 'STAGE_C_REGRESSION | 11/11 tests passed'
Write-Host 'STAGE_D_ACCEPTANCE | 9/9 tests passed'
Write-Host 'STAGE_D_FIXES | 8/8 tests passed'
