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

foreach ($RequiredPath in @($Project, $Build, $Editor)) {
    if (-not (Test-Path -LiteralPath $RequiredPath)) {
        throw "Required Stage C path does not exist: $RequiredPath"
    }
}

& $Build NationSimulationStageCEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) {
    throw "UnrealBuildTool failed with exit code $LASTEXITCODE"
}

& $Editor $Project -unattended -nop4 -NullRHI -nosplash -NoSound `
    '-ExecCmds=Automation RunTests NationSimulation.StageC.Acceptance;Quit' `
    '-TestExit=Automation Test Queue Empty' -log
if ($LASTEXITCODE -ne 0) {
    throw "Unreal Automation Test failed with exit code $LASTEXITCODE"
}

$Results = Join-Path $RepositoryRoot 'out\stage-c\test-output\acceptance_results.txt'
if (-not (Test-Path -LiteralPath $Results)) {
    throw "Stage C acceptance evidence was not generated: $Results"
}
$Summary = Get-Content -LiteralPath $Results | Select-Object -Last 1
if ($Summary -ne 'SUMMARY | 11/11 tests passed') {
    throw "Unexpected Stage C acceptance summary: $Summary"
}

Write-Host $Summary
