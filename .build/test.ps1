
[CmdletBinding(PositionalBinding=$false)]
param (

    [Parameter(Mandatory=$true)]
    [string] $ReleaseType,

    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]] $ExtraArgs
)

# Standard boilerplate
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSDefaultParameterValues['*:ErrorAction'] = 'Stop'

# Go to repo root
$repoRoot = (Resolve-Path "$PSScriptRoot\..").Path
Push-Location $repoRoot

try {
    cd build
    ctest . -C "$ReleaseType" --rerun-failed --output-on-failure
} finally {
    Pop-Location
}
