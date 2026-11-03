param([switch]$All, [switch]$Full, [string]$ReportDirectory = '')
$ErrorActionPreference = 'Stop'
$submissionRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $ReportDirectory) { $ReportDirectory = Join-Path $submissionRoot 'build/reports/current' }
New-Item -ItemType Directory -Force -Path $ReportDirectory | Out-Null
$submissionTests = @('tests/unit')
if ($All) { $submissionTests += 'tests/integration' }
$submissionArguments = @()
foreach ($submissionTest in $submissionTests) { $submissionArguments += (Join-Path $PSScriptRoot $submissionTest) }
$submissionArguments += @('--rootdir', $submissionRoot)
if (-not $Full) {
    $submissionProfile = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'host_profile.json') -Encoding UTF8 -Raw | ConvertFrom-Json
    if ($env:OS -eq 'Windows_NT') {
        foreach ($submissionRequirement in $submissionProfile.windows_requirements) {
            Write-Output ('Excluded Linux-specific file: ' + $submissionRequirement.path + ' - ' + $submissionRequirement.reason)
            $submissionArguments += @('--ignore', (Join-Path $PSScriptRoot $submissionRequirement.path))
        }
    }
    foreach ($submissionKnown in $submissionProfile.known_baseline_failures) {
        Write-Output ('Excluded reproduced baseline failure: ' + $submissionKnown.node + ' - ' + $submissionKnown.reason)
        $submissionArguments += @('--deselect', ('sim/' + $submissionKnown.node))
    }
}
$submissionArguments += @('-q','--junitxml', (Join-Path $ReportDirectory 'software.xml'))
& poetry -C (Join-Path $submissionRoot 'src/ps') run pytest @submissionArguments
if ($LASTEXITCODE -ne 0) { throw 'Software checks failed. Review software.xml; platform-specific requirements are documented in sim/readme.md.' }
