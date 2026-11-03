[CmdletBinding(SupportsShouldProcess=$true)]
param()
$ErrorActionPreference = 'Stop'
$submissionRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$submissionPrefix = $submissionRoot.TrimEnd('\') + '\'
# 工具仍使用本交付目录时拒绝清理，避免删除运行中的依赖。
$submissionProcesses = Get-CimInstance Win32_Process | Where-Object {
    $_.Name -match '^(vivado|vitis|vitis-run|xsim|xelab|xvlog|vitis_hls).*' -and
    $_.CommandLine -and $_.CommandLine.IndexOf($submissionRoot, [StringComparison]::OrdinalIgnoreCase) -ge 0
}
if ($submissionProcesses) { throw 'A tool process still refers to this submission. Wait for it to finish before cleaning.' }
function Remove-GeneratedPath([string]$Candidate) {
    # 先核验规范化绝对路径，再用同一 PowerShell 删除，不跨 shell 拼接命令。
    $submissionTarget = [IO.Path]::GetFullPath($Candidate)
    if (-not $submissionTarget.StartsWith($submissionPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refuse to remove a path outside the submission: $submissionTarget"
    }
    if ((Test-Path -LiteralPath $submissionTarget) -and
        ((Get-Item -LiteralPath $submissionTarget -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Refuse recursive cleanup through a junction or symbolic link: $submissionTarget"
    }
    if ((Test-Path -LiteralPath $submissionTarget) -and $PSCmdlet.ShouldProcess($submissionTarget,'Remove generated files')) {
        Remove-Item -LiteralPath $submissionTarget -Recurse -Force
    }
}
foreach ($submissionRelative in @('build/generated','board/generated','board/project/.Xil',
    'skill/python-poetry/.git','skill/xilinx-fpga/.git',
    'board/project/puzzle5ev.cache','board/project/puzzle5ev.gen','board/project/puzzle5ev.hw',
    'board/project/puzzle5ev.ip_user_files','board/project/puzzle5ev.runs','board/project/puzzle5ev.sim')) {
    Remove-GeneratedPath (Join-Path $submissionRoot $submissionRelative)
}
$submissionBdFolder = Join-Path $submissionRoot 'board/project/puzzle5ev.srcs/sources_1/bd/system'
# BD 是持久输入；仅删除同目录的工具生成内容，始终保留 system.bd。
if (Test-Path -LiteralPath $submissionBdFolder) {
    Get-ChildItem -LiteralPath $submissionBdFolder -Force | Where-Object Name -ne 'system.bd' | ForEach-Object { Remove-GeneratedPath $_.FullName }
}
Get-ChildItem -LiteralPath $submissionRoot -Directory -Recurse -Force |
    Where-Object Name -in @('__pycache__','.pytest_cache','.venv') |
    Sort-Object { $_.FullName.Length } -Descending | ForEach-Object { Remove-GeneratedPath $_.FullName }
Write-Output 'Only submission-generated directories were selected; saved XPR/BD, sources, vectors, reports and dependencies are preserved.'
