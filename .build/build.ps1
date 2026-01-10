[CmdletBinding(PositionalBinding=$false)]
param (
    [Parameter(Mandatory=$true)]
    [string] $DPath,

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
    $null = New-Item -Path build -ItemType Directory -Force
    cd build

    $env:VERBOSE = '1'

    cmake .. -DCMAKE_BUILD_TYPE="$ReleaseType" -DCMAKE_CONFIGURATION_TYPES="$ReleaseType" @ExtraArgs
    if ($LastExitCode -ne 0) {
        throw "'cmake' exited with code $LastExitCode"
    }

    cmake --build . --config "$ReleaseType"
    if ($LastExitCode -ne 0) {
        throw "'cmake --build' exited with code $LastExitCode"
    }

    if (Test-Path -Path "'$DPath\debug\bin\'" -PathType Container) {
        dir $DPath\debug\bin\*.dll
        dir $DPath\debug\bin\*.exe
    } else {
        Write-Information "Directory '$DPath\debug\bin\' not exists"
    }

    if (Test-Path -Path "'$DPath\release\bin\'" -PathType Container) {
        dir $DPath\release\bin\*.dll
        dir $DPath\release\bin\*.exe
    } else {
        Write-Information "Directory '$DPath\release\bin\' not exists"
    }
} finally {
    Pop-Location
}
