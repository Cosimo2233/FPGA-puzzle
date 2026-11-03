param([string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
$submissionRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $submissionRoot 'board/software' }
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
& poetry -C (Join-Path $submissionRoot 'src/ps') check
if ($LASTEXITCODE -ne 0) { throw 'Poetry metadata validation failed' }
& poetry -C (Join-Path $submissionRoot 'src/ps') build -f wheel -o $OutputDirectory
if ($LASTEXITCODE -ne 0) { throw 'Application wheel build failed' }
Get-ChildItem -LiteralPath (Join-Path $submissionRoot 'data/dependencies') -Filter '*.whl' | Copy-Item -Destination $OutputDirectory
& poetry -C (Join-Path $submissionRoot 'src/ps') run python (Join-Path $PSScriptRoot 'software_manifest.py') --output $OutputDirectory
if ($LASTEXITCODE -ne 0) { throw 'Software identity manifest failed' }
Write-Output "Application and independent dependency wheels: $OutputDirectory"
