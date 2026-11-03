param(
    [ValidateSet('check','hls','project','compile','all','software','rtl','package')]
    [string]$Stage = 'check',
    [string]$AmdRoot = $env:AMD_ROOT,
    [switch]$Cosim
)
$ErrorActionPreference = 'Stop'
$submissionRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
# 构建阶段只使用本交付树的路径；AMD 安装位置由调用者提供。
if ($Stage -in @('hls','project','compile','all','rtl','package') -and -not $AmdRoot) {
    throw 'Pass -AmdRoot <AMD 2026.1 root> or set AMD_ROOT.'
}
switch ($Stage) {
    'check' {
        & poetry -C (Join-Path $submissionRoot 'src/ps') run python (Join-Path $PSScriptRoot 'validate_repository.py') check
    }
    'software' { & (Join-Path $PSScriptRoot 'build-software.ps1') }
    'rtl' { & (Join-Path $PSScriptRoot 'sim_hdmi.ps1') -VivadoBin (Join-Path $AmdRoot 'Vivado/bin') }
    'package' {
        & poetry -C (Join-Path $submissionRoot 'src/ps') run python (Join-Path $PSScriptRoot 'package_hardware.py') --amd-root $AmdRoot
    }
    default { & (Join-Path $PSScriptRoot 'build-axu5ev.ps1') -Stage $Stage -AmdRoot $AmdRoot -Cosim:$Cosim }
}
if ($LASTEXITCODE -ne 0) { throw "Stage failed: $Stage" }
