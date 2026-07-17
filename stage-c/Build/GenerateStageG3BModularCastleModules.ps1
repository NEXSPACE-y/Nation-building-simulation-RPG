[CmdletBinding()]
param(
    [string]$Blender = 'C:\Program Files\Blender Foundation\Blender 5.1\blender.exe'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Script = Join-Path $PSScriptRoot 'GenerateStageG3BModularCastleModules.py'
$Output = Join-Path $ProjectRoot 'SourceArt\StageG3B\ModularCastle\v0.1\Models'
$Report = Join-Path $ProjectRoot 'SourceArt\StageG3B\ModularCastle\v0.1\Reports\module_source_manifest.json'

foreach ($Required in @($Blender, $Script)) {
    if (-not (Test-Path -LiteralPath $Required)) {
        throw "Required G-3B-R module input is missing: $Required"
    }
}

New-Item -ItemType Directory -Force -Path $Output | Out-Null
$Log = Join-Path $env:TEMP 'stage_g3br_blender_modules.log'
& $Blender --background --factory-startup --python $Script *> $Log
if ($LASTEXITCODE -ne 0) {
    throw "G-3B-R Blender module generation failed with exit code $LASTEXITCODE. Log: $Log"
}

$Fbx = @(Get-ChildItem -LiteralPath $Output -Filter '*.fbx' -File)
if ($Fbx.Count -ne 22 -or -not (Test-Path -LiteralPath $Report)) {
    throw "G-3B-R module contract failed: fbx=$($Fbx.Count) report=$([int](Test-Path -LiteralPath $Report))"
}
if (@($Fbx | Where-Object Length -gt 50MB).Count -ne 0) {
    throw 'G-3B-R generated an FBX over 50MB.'
}

Write-Host "STAGE_G3BR_MODULAR_SOURCE_PASS count=$($Fbx.Count)"
