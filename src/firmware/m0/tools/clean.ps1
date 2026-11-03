param([string]$BuildDir = '../../../build/generated/m0')
$ErrorActionPreference = 'Stop'
$firmwareRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$submissionRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../..'))
$allowedRoot = [IO.Path]::GetFullPath((Join-Path $submissionRoot 'build/generated')).TrimEnd('\') + '\'
if ([IO.Path]::IsPathRooted($BuildDir)) { $firmwareBuildTarget = [IO.Path]::GetFullPath($BuildDir) }
else { $firmwareBuildTarget = [IO.Path]::GetFullPath((Join-Path $firmwareRoot $BuildDir)) }
# 固件也统一输出到仓库生成区；禁止按参数清除源码或仓库外目录。
if (-not $firmwareBuildTarget.StartsWith($allowedRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to clean outside submission build/generated: $firmwareBuildTarget"
}
if (Test-Path -LiteralPath $firmwareBuildTarget) {
    Remove-Item -LiteralPath $firmwareBuildTarget -Recurse -Force
}
