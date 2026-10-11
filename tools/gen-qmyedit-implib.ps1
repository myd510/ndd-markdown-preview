# ===================================================================
#  从已安装的 Notepad-- 生成 qmyedit_qt5 导入库（.lib）
#  ------------------------------------------------------------------
#  背景：Notepad-- 二进制发行版只带 qmyedit_qt5.dll，不带 .lib 导入库。
#        插件必须链接该库才能调用 QsciScintilla，因此用 dumpbin 读导出表、
#        再用 lib.exe 现场生成一个导入库。
#
#  用法：
#     powershell -ExecutionPolicy Bypass -File tools\gen-qmyedit-implib.ps1 ^
#                -NddDir <Notepad-- 安装目录>
#
#  参数（均可用环境变量代替，脚本内不含任何硬编码路径）：
#     -NddDir  Notepad-- 安装目录（含 qmyedit_qt5.dll）
#              未指定时读取环境变量 NDD_INSTALL_DIR
#     -OutDir  输出目录，默认 <仓库根>\libs
#
#  工具链（dumpbin / lib）优先从 PATH 查找，找不到时用 vswhere 定位
#  最新 Visual Studio 的 MSVC 工具集。
# ===================================================================
param(
    [string]$NddDir = $env:NDD_INSTALL_DIR,
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"

# 仓库根目录 = 本脚本所在目录的上一级
$projRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
if ([string]::IsNullOrWhiteSpace($OutDir)) { $OutDir = Join-Path $projRoot "libs" }

if ([string]::IsNullOrWhiteSpace($NddDir)) {
    Write-Error "未指定 Notepad-- 安装目录。请用 -NddDir <目录>，或设置环境变量 NDD_INSTALL_DIR。"
    exit 1
}
$dll = Join-Path $NddDir "qmyedit_qt5.dll"
if (-not (Test-Path $dll)) {
    Write-Error "未找到 $dll"
    exit 1
}

# ---- 定位 VS 工具 ----
function Find-VsTool {
    param([string]$Name)

    $onPath = Get-Command $Name -ErrorAction SilentlyContinue
    if ($onPath) { return $onPath.Source }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { return $null }

    $vsPath = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
    if (-not $vsPath) { return $null }

    $msvcRoot = Join-Path $vsPath "VC\Tools\MSVC"
    if (-not (Test-Path $msvcRoot)) { return $null }

    $msvc = Get-ChildItem $msvcRoot -Directory |
            Sort-Object Name -Descending | Select-Object -First 1
    if (-not $msvc) { return $null }

    $candidate = Join-Path $msvc.FullName "bin\Hostx64\x64\$Name"
    if (Test-Path $candidate) { return $candidate }
    return $null
}

$dumpbin = Find-VsTool "dumpbin.exe"
$lib     = Find-VsTool "lib.exe"
if (-not $dumpbin) { Write-Error "未找到 dumpbin.exe，请安装 Visual Studio（C++ 工作负载）或在开发者命令提示符中运行。"; exit 1 }
if (-not $lib)     { Write-Error "未找到 lib.exe"; exit 1 }

if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir | Out-Null }

Write-Host "[1/3] 读取导出表 ..."
$raw = & $dumpbin /nologo /exports $dll
# 行格式: "     9    8 001943A0 ??0QsciCommand@@AEAA@..."
$names = @()
foreach ($line in $raw) {
    if ($line -match '^\s+\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]{8}\s+(\S+)\s*$') {
        $names += $Matches[1]
    }
}
if ($names.Count -eq 0) { Write-Error "未解析到任何导出符号"; exit 1 }
Write-Host ("      共 " + $names.Count + " 个导出符号")

Write-Host "[2/3] 生成 .def ..."
$defPath = Join-Path $OutDir "qmyedit_qt5.def"
$defLines = @("LIBRARY qmyedit_qt5.dll", "EXPORTS")
$defLines += $names | ForEach-Object { "    " + $_ }
[System.IO.File]::WriteAllLines($defPath, $defLines, [System.Text.Encoding]::ASCII)
Write-Host ("      " + $defPath)

Write-Host "[3/3] 生成导入库 ..."
$libPath = Join-Path $OutDir "qmyedit_qt5.lib"
& $lib /nologo "/def:$defPath" /machine:x64 "/out:$libPath"
if (-not (Test-Path $libPath)) { Write-Error "导入库生成失败"; exit 1 }

$size = (Get-Item $libPath).Length
Write-Host ""
Write-Host ("完成: " + $libPath + "  (" + [math]::Round($size / 1KB, 1) + " KB)")
