[CmdletBinding(PositionalBinding=$false)]
param (
    [Parameter(Mandatory=$true)]
    [string] $DPath,
#
#     [Parameter(Mandatory=$false)]
#     [string] $EncodingType = "WIN32API",

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
    $null = New-Item -Path build -ItemType Directory -Force
    cd build

    $env:VERBOSE = '1'

    cmake .. @ExtraArgs
    if ($LastExitCode -ne 0) {
        throw "'cmake' exited with code $LastExitCode"
    }

    cmake --build .
    if ($LastExitCode -ne 0) {
        throw "'cmake --build' exited with code $LastExitCode"
    }

    dir $DPath\debug\bin\*.dll
    dir $DPath\debug\bin\*.exe
    dir $DPath\release\bin\*.dll
    dir $DPath\release\bin\*.exe
} finally {
    Pop-Location
}
