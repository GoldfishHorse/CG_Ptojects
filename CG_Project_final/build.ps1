
enum LogLevel {
    DEBUG = 0
    INFO = 1
    WARNING = 2
    ERROR = 3
}

class Logger {
    [LogLevel]$LogLevel

    Logger([LogLevel]$logLevel) {
        $this.LogLevel = $logLevel
    }

    [void]Log([LogLevel]$level, [string]$message) {
        if ([int]$level.value__ -ge  [int]$this.LogLevel.value__) {
            switch ($level) {
                ([LogLevel]::DEBUG) { 
                    Write-Host "[" -NoNewline
                    Write-Host "DEBUG" -ForegroundColor Gray -NoNewline
                    Write-Host "] $message"
                }
                ([LogLevel]::INFO) { 
                    Write-Host "[" -NoNewline
                    Write-Host "INFO" -ForegroundColor Green -NoNewline
                    Write-Host "] $message"
                }
                ([LogLevel]::WARNING) { 
                    Write-Host "[" -NoNewline
                    Write-Host "WARNING" -ForegroundColor Yellow -NoNewline
                    Write-Host "] $message"
                }
                ([LogLevel]::ERROR) {
                    Write-Host "[" -NoNewline
                    Write-Host "ERROR" -ForegroundColor Red -NoNewline
                    Write-Host "] $message"
                }
            }
        }
    }

    [void]Debug([string]$message) { $this.Log([LogLevel]::DEBUG, $message) }
    [void]Info([string]$message) { $this.Log([LogLevel]::INFO, $message) }
    [void]Warning([string]$message) { $this.Log([LogLevel]::WARNING, $message) }
    [void]Error([string]$message) { $this.Log([LogLevel]::ERROR, $message) }
}

$logger = [Logger]::new([LogLevel]::INFO)


function Git-Clone {
    param (
        [string]$repositoryUrl,
        [string]$localPath
    )

    [bool]$ret  = $False
    if (-not (Get-Command git -ErrorAction SilentlyContinue))
    {
        $logger.Error("系统中未找到 Git.")
    } elseif (Test-Path $localPath) {
        $logger.Warning("目标路径 $localPath 已存在")
        $ret = $True
    } else {
        try {
            git clone $repositoryUrl $localPath
            if ($LASTEXITCODE -eq 0) {
                $logger.Info("仓库 $repositoryUrl clone 成功到 $localPath." )
                $ret = $True
            } else {
                $logger.Error("仓库 $repositoryUrl clone 失败.")
            }
        } catch {
            $logger.Error("仓库 $repositoryUrl clone 中出现错误: $($_.Exception.Message)")
        }
    }
    
    return $ret
}

function Main {

    # 设置 PowerShell 控制台的输出和输入编码为 UTF-8
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    [Console]::InputEncoding = [System.Text.Encoding]::UTF8
    
    # https://learn.microsoft.com/zh-cn/powershell/module/microsoft.powershell.core/about/about_automatic_variables?view=powershell-7.5#psscriptroot
    [string]$projectPath = $PSScriptRoot
    [string]$thirdPartyPath = Join-Path $projectPath "third_party"
    [string]$buildPath = Join-Path $projectPath "build"
    [string]$srcPath = Join-Path $projectPath "src"
    [int]$buildParallel = 16
    [string]$sceneFileurl = "http://10.76.1.181/courses/graphics/2024/files/example-scenes-cg24.zip"
    
    if (-not (Test-Path $srcPath))
    {
        $logger.Error("找不到 $srcPath 目录.")
        return
    }

    if (-not (Test-Path $thirdPartyPath))
    {
        New-Item $thirdPartyPath -ItemType Directory -Force | Out-Null
    } 
    $logger.Info("正在克隆仓库...")


    [bool]$success =   $(Git-Clone "https://github.com/g-truc/glm.git" $(Join-Path $thirdPartyPath "glm")) `
                         -and $(Git-Clone "https://github.com/glfw/glfw.git" $(Join-Path $thirdPartyPath "glfw")) `
                         -and $(Git-Clone "https://github.com/nothings/stb.git" $(Join-Path $thirdPartyPath "stb")) `
                         -and $(Git-Clone "https://github.com/tinyobjloader/tinyobjloader.git" $(Join-Path $thirdPartyPath "tinyobjloader")) `
                         -and $(Git-Clone "https://github.com/leethomason/tinyxml2.git" $(Join-Path $thirdPartyPath "tinyxml2"))

    if ($success)
    {
        $logger.Info("仓库 clone 完成！")
    } else {
        $logger.Info("仓库 clone 失败.")
        return
    }

    if (-not (Get-Command cmake -ErrorAction SilentlyContinue))
    {
        $logger.Error("系统中未找到 cmake.")
        return
    } else {
        $logger.Info($("找到 cmake: " + (Get-Command cmake).Source))
    }


    $logger.Info("构建路径: $buildPath.")

    if (Get-Command cl -ErrorAction SilentlyContinue)
    {
        $logger.Info($("编译器:" + (Get-Command cl).Source))
        $logger.Info("使用 MSVC 构建项目。")
        cmake -G "Visual Studio 17 2022" -A x64 -B $buildPath -S $projectPath 
        cmake --build $buildPath --config Release --parallel $buildParallel
    } elseif (
        (Get-Command gcc -ErrorAction SilentlyContinue) -and 
        (Get-Command make -ErrorAction SilentlyContinue)
    ) {
        $logger.Info($("编译器:" + (Get-Command gcc).Source))
        $logger.Info("使用 GNU Make 构建项目。")
        cmake -G "Unix Makefiles" -B $buildPath -S $projectPath -DCMAKE_BUILD_TYPE=RELEASE
        cmake --build $buildPath --parallel $buildParallel
    } else {
        $logger.Error("找不到编译器.")
        return
    }

    $sceneFile = Join-Path -Path $projectPath -ChildPath "example-scenes-cg24.zip"
    $sceneFileFolder = Join-Path -Path $projectPath -ChildPath "example-scenes-cg24"
    if ( 
        (-not (Test-Path $sceneFile -PathType Leaf)) -and 
        (-not (Test-Path $sceneFileFolder -PathType Container))
    ) {
        
        $logger.Info("正在下载 $sceneFile")
        try {
            Invoke-WebRequest -Uri $sceneFileurl -OutFile $sceneFile
        }
        catch {
            # 下载失败时报告错误  
            $logger.Error("文件下载失败: $_")
            return
        }
        Write-Host "文件下载成功: $sceneFile."
    }

    if (
        (-not (Test-Path $sceneFileFolder -PathType Container)) -or 
        (-not (Test-Path $(Join-Path $sceneFileFolder "*") <# 空目录 #> ) )
    )
    {
        $logger.Info("解压 $sceneFile.")
        New-Item $sceneFileFolder -ItemType Directory -Force  | Out-Null
        try {        
            Expand-Archive -Path $sceneFile -DestinationPath $projectPath
        } catch {
            $logger.Error("文件解压失败: $_")
            return
        }
    }

    <#  由于程序读取 .xml 文件的逻辑与 example-scenes-cg24.zip 中给定 .xml 配置文件有所不同
        因此用 example_configs 中的 .xml 文件覆盖  #>
    $configPath = Join-Path $projectPath "example_configs"
    if ( (Test-Path $configPath) -and (Test-Path $(Join-Path $sceneFileFolder "*") ) )
    {
        $srcFile = Join-Path $configPath "bathroom2.xml"
        if (Test-Path $srcFile)
        {
            $dstFile = Join-Path  -Path $sceneFileFolder -ChildPath "bathroom2/bathroom2.xml"
            $logger.Info("用 $srcFile 替换 $dstFile .")
            Copy-Item  -Path $srcFile -Destination $dstFile -Force
        }
        $srcFile = Join-Path $configPath "cornell-box.xml"
        if (Test-Path $srcFile)
        {
            $dstFile = Join-Path  -Path $sceneFileFolder -ChildPath "cornell-box/cornell-box.xml"
            $logger.Info("用 $srcFile 替换 $dstFile .")
            Copy-Item  -Path $srcFile -Destination $dstFile -Force
        }
        $srcFile = Join-Path $configPath "veach-mis.xml"
        if (Test-Path $srcFile)
        {
            $dstFile = Join-Path  -Path $sceneFileFolder -ChildPath "veach-mis/veach-mis.xml"
            $logger.Info("用 $srcFile 替换 $dstFile .")
            Copy-Item  -Path $srcFile -Destination $dstFile -Force
        }
    }

    [string]$exePath = (Get-ChildItem -Path ".\build\" -Filter "project.exe" -File -Recurse).FullName
    if ($exePath)
    {
        $logger.Info("项目构建成功: 请运行 $exePath .")
    }
    return
}

Main

