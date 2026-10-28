[CmdletBinding()]
param(
    [ValidateSet("bathroom2", "cornell-box", "veach-mis")]
    [string[]]$Scene = @("bathroom2", "cornell-box", "veach-mis"),
    [ValidateRange(1, 1000000)]
    [int]$Iterations = 5000,
    [ValidateRange(1, 1000000)]
    [int]$FrameInterval = 100,
    [ValidatePattern("^[A-Za-z0-9._-]+$")]
    [string]$RunLabel = (Get-Date -Format "yyyyMMdd-HHmmss"),
    [switch]$SkipBuild,
    [switch]$SkipGif,
    [switch]$KeepFrames
)

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$sceneArchive = Join-Path $PSScriptRoot "example-scenes-cg24.zip"
$sceneRoot = Join-Path $PSScriptRoot "example-scenes-cg24"
$executable = Join-Path $PSScriptRoot "build/windows-msvc/Release/cpu_path_tracer.exe"

$sceneDefinitions = @{
    "bathroom2" = @{
        Obj = "example-scenes-cg24/bathroom2/bathroom2.obj"
        Mtl = "example-scenes-cg24/bathroom2/bathroom2.mtl"
        Config = "example_configs/bathroom2.xml"
    }
    "cornell-box" = @{
        Obj = "example-scenes-cg24/cornell-box/cornell-box.obj"
        Mtl = "example-scenes-cg24/cornell-box/cornell-box.mtl"
        Config = "example_configs/cornell-box.xml"
    }
    "veach-mis" = @{
        Obj = "example-scenes-cg24/veach-mis/veach-mis.obj"
        Mtl = "example-scenes-cg24/veach-mis/veach-mis.mtl"
        Config = "example_configs/veach-mis.xml"
    }
}

Push-Location -LiteralPath $PSScriptRoot
try {
    if (-not $SkipBuild) {
        Write-Host "[build] Rebuilding the Release executable..."
        & (Join-Path $PSScriptRoot "build.ps1") -Configuration Release -Rebuild
        if ($LASTEXITCODE -ne 0) {
            throw "Release build failed with exit code $LASTEXITCODE."
        }
    }

    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw "Executable not found: $executable"
    }

    $missingScene = $false
    foreach ($name in $Scene) {
        $definition = $sceneDefinitions[$name]
        if (-not (Test-Path -LiteralPath (Join-Path $PSScriptRoot $definition.Obj))) {
            $missingScene = $true
            break
        }
    }
    if ($missingScene) {
        if (-not (Test-Path -LiteralPath $sceneArchive -PathType Leaf)) {
            throw "Scene archive not found: $sceneArchive"
        }
        Write-Host "[scene] Extracting example-scenes-cg24.zip..."
        Expand-Archive -LiteralPath $sceneArchive -DestinationPath $PSScriptRoot -Force
    }

    foreach ($name in $Scene) {
        $definition = $sceneDefinitions[$name]
        # Never overwrite the course-stage PNGs or GIFs under result/<scene>.
        # Every rerun receives an independent directory identified by RunLabel.
        $resultDirectory = Join-Path $PSScriptRoot "result/$name/reruns/$RunLabel"
        New-Item -ItemType Directory -Path $resultDirectory -Force | Out-Null
        $finalImage = Join-Path $resultDirectory "$Iterations.png"
        $logPath = Join-Path $resultDirectory "render-$Iterations.log"
        if (Test-Path -LiteralPath $finalImage -PathType Leaf) {
            throw "Run output already exists: $finalImage. Choose another -RunLabel."
        }

        # Redirect periodic frames to the rerun directory as well. Otherwise the
        # saveimgpath in the original XML would overwrite historical frames.
        $runtimeConfig = Get-Content -LiteralPath $definition.Config -Encoding UTF8 -Raw
        $relativeResultDirectory = "result/$name/reruns/$RunLabel"
        $saveFrequency = if ($SkipGif) { 0 } else { $FrameInterval }
        $runtimeConfig = [regex]::Replace(
            $runtimeConfig,
            'saveimgfrequenc(?:y|e)="[^"]*"',
            "saveimgfrequency=`"$saveFrequency`"")
        $runtimeConfig = [regex]::Replace(
            $runtimeConfig,
            'saveimgpath="[^"]*"',
            "saveimgpath=`"$relativeResultDirectory`"")
        $runtimeConfigPath = Join-Path $resultDirectory "render-config.xml"
        Set-Content -LiteralPath $runtimeConfigPath -Value $runtimeConfig -Encoding UTF8

        Write-Host "[render] ${name}: $Iterations iterations with $($definition.Config)"
        $arguments = @(
            "--obj", $definition.Obj,
            "--mtl", $definition.Mtl,
            "--config", $runtimeConfigPath,
            "--name", $name,
            "--headless",
            "--iterations", $Iterations,
            "--output", $finalImage)

        & $executable @arguments 2>&1 | Tee-Object -FilePath $logPath
        if ($LASTEXITCODE -ne 0) {
            throw "Rendering $name failed with exit code $LASTEXITCODE."
        }
        if (-not (Test-Path -LiteralPath $finalImage -PathType Leaf)) {
            throw "Renderer did not create $finalImage."
        }

        if (-not $SkipGif) {
            $magick = Get-Command magick -ErrorAction SilentlyContinue
            if (-not $magick) {
                throw "ImageMagick was not found. Install it or pass -SkipGif."
            }

            $frames = @(for ($iteration = $FrameInterval; $iteration -le $Iterations; $iteration += $FrameInterval) {
                Join-Path $resultDirectory "$iteration.png"
            })
            if ($frames.Count -eq 0 -or $frames[-1] -ne $finalImage) {
                $frames += $finalImage
            }
            $missingFrames = $frames | Where-Object { -not (Test-Path -LiteralPath $_ -PathType Leaf) }
            if ($missingFrames) {
                throw "Cannot generate GIF for $name because one or more 100-iteration frames are missing."
            }

            $gifPath = Join-Path $resultDirectory "$name.gif"
            Write-Host "[gif] $gifPath"
            & $magick.Source -delay 10 -loop 0 @frames $gifPath
            if ($LASTEXITCODE -ne 0) {
                throw "GIF generation for $name failed with exit code $LASTEXITCODE."
            }
        }

        if (-not $KeepFrames) {
            for ($iteration = $FrameInterval; $iteration -lt $Iterations; $iteration += $FrameInterval) {
                $frame = Join-Path $resultDirectory "$iteration.png"
                if (Test-Path -LiteralPath $frame -PathType Leaf) {
                    Remove-Item -LiteralPath $frame -Force
                }
            }
        }
    }
}
finally {
    Pop-Location
}
