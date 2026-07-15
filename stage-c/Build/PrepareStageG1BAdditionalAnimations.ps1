[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$Script = Join-Path $PSScriptRoot 'PrepareStageG1BAdditionalAnimations.py'
python $Script
if ($LASTEXITCODE -ne 0) {
    throw "Stage G-1B additional-animation source preparation failed with exit code $LASTEXITCODE"
}
