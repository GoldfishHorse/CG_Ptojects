[CmdletBinding()]
param(
    [ValidateSet("All", "CMD", "UI", "Clean")]
    [string]$Target = "All"
)

$ErrorActionPreference = "Stop"
$compiler = (Get-Command g++ -ErrorAction Stop).Source
$compileFlags = @(
    "-std=c++17", "-Wall", "-Wextra", "-O2",
    "-I$PSScriptRoot/src", "-I$PSScriptRoot/src/ZBuffer")
$linkFlags = @("-ld3d9", "-ld3dx9", "-lcomdlg32")

$commonSources = Get-ChildItem -LiteralPath "$PSScriptRoot/src" -Recurse -File -Filter "*.cpp" |
    Where-Object { $_.Name -notin @("mainCMD.cpp", "mainUI.cpp") } |
    ForEach-Object { $_.FullName }

function Build-Executable {
    param(
        [Parameter(Mandatory)] [string]$MainSource,
        [Parameter(Mandatory)] [string]$Output
    )

    Write-Host "[build] $Output"
    $arguments = @($commonSources) + $MainSource + $compileFlags + @("-o", $Output) + $linkFlags
    & $compiler @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "g++ failed while building $Output."
    }
}

if ($Target -eq "Clean") {
    foreach ($file in @("ZBufferCMD.exe", "ZBufferUI.exe")) {
        $path = Join-Path $PSScriptRoot $file
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Force
        }
    }
    return
}

if ($Target -in @("All", "CMD")) {
    Build-Executable "$PSScriptRoot/src/mainCMD.cpp" "$PSScriptRoot/ZBufferCMD.exe"
}
if ($Target -in @("All", "UI")) {
    Build-Executable "$PSScriptRoot/src/mainUI.cpp" "$PSScriptRoot/ZBufferUI.exe"
}
