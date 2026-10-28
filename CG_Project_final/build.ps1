[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [switch]$Rebuild
)

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

function Invoke-Checked {
    param(
        [Parameter(Mandatory)] [string]$Program,
        [Parameter(Mandatory)] [string[]]$Arguments
    )

    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program failed with exit code $LASTEXITCODE."
    }
}

function Initialize-Dependency {
    param(
        [Parameter(Mandatory)] [string]$Name,
        [Parameter(Mandatory)] [string]$Repository,
        [Parameter(Mandatory)] [string]$Revision,
        [Parameter(Mandatory)] [string]$RequiredFile
    )

    $destination = Join-Path $PSScriptRoot "third_party/$Name"
    $marker = Join-Path $destination $RequiredFile
    if (Test-Path -LiteralPath $marker -PathType Leaf) {
        Write-Host "[dependency] ${Name}: ready"
        return
    }

    if (Test-Path -LiteralPath $destination) {
        throw "Incomplete dependency directory: $destination. Remove it and run the script again."
    }

    Write-Host "[dependency] ${Name}: cloning $Revision"
    Invoke-Checked git @("init", $destination)
    Invoke-Checked git @("-C", $destination, "remote", "add", "origin", $Repository)
    Invoke-Checked git @("-C", $destination, "fetch", "--depth", "1", "origin", $Revision)
    Invoke-Checked git @("-C", $destination, "checkout", "--detach", "FETCH_HEAD")
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake 3.20 or newer is required."
}
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git is required to prepare third-party dependencies."
}

Push-Location -LiteralPath $PSScriptRoot
try {
    Initialize-Dependency "glfw" `
        "https://github.com/glfw/glfw.git" `
        "a74efa0d5628b74adc0426af4c5710e287fa7c2c" "CMakeLists.txt"
    Initialize-Dependency "glm" `
        "https://github.com/g-truc/glm.git" `
        "0af55ccecd98d4e5a8d1fad7de25ba429d60e863" "glm/glm.hpp"
    Initialize-Dependency "tinyxml2" `
        "https://github.com/leethomason/tinyxml2.git" `
        "321ea883b7190d4e85cae5512a12e5eaa8f8731f" "tinyxml2.cpp"
    Initialize-Dependency "tinyobjloader" `
        "https://github.com/tinyobjloader/tinyobjloader.git" `
        "2945a967c5303b2c8c14174117c45f3302591150" "tiny_obj_loader.h"
    Initialize-Dependency "stb" `
        "https://github.com/nothings/stb.git" `
        "003501f6d7a8668ce8b6d875e34dd97ad0d5d5b5" "stb_image.h"

    Write-Host "[build] Configuring $Configuration..."
    Invoke-Checked cmake @("--preset", "windows-msvc")

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio/Installer/vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw "vswhere.exe was not found. Install Visual Studio 2022 C++ Build Tools."
    }
    $msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild `
        -find "MSBuild\**\Bin\amd64\MSBuild.exe" | Select-Object -First 1
    if (-not $msbuild) {
        throw "MSBuild was not found. Install Visual Studio 2022 C++ Build Tools."
    }

    $msbuildArguments = @(
        "build/windows-msvc/cpu_path_tracer.vcxproj",
        "/p:Configuration=$Configuration", "/p:Platform=x64",
        "/m:1", "/nr:false", "/v:minimal")
    if ($Rebuild) {
        $msbuildArguments += "/t:Rebuild"
    }
    Invoke-Checked $msbuild $msbuildArguments

    $executable = Join-Path $PSScriptRoot "build/windows-msvc/$Configuration/cpu_path_tracer.exe"
    Write-Host "[build] Done: $executable"
}
finally {
    Pop-Location
}
