param([Parameter(Mandatory=$true)][string]$AmdRoot, [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
$submissionRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$submissionArguments = @((Join-Path $PSScriptRoot 'package_hardware.py'), '--amd-root', $AmdRoot)
if ($OutputDirectory) { $submissionArguments += @('--output', $OutputDirectory) }
& poetry -C (Join-Path $submissionRoot 'src/ps') run python @submissionArguments
if ($LASTEXITCODE -ne 0) { throw 'Hardware package reconstruction failed' }
