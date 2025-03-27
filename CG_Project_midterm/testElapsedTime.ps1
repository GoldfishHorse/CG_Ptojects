param (
    [Parameter(Position = 0)] 
    [string]$objFilePath = (Join-Path -Path $PSScriptRoot -ChildPath 'Model\teapot.obj'), 
    [Parameter(Position = 1)] 
    [string]$outputPath = (Join-Path -Path $PSScriptRoot -ChildPath 'output'),
    [Parameter(Position = 2)]
    [bool]$echo = $true
)
$exePath = Join-Path -Path $PSScriptRoot -ChildPath 'ZBufferCMD.exe'

$result = @()

# foreach ($zBufferChoice in @('No', 'Naive', 'ScanLine', 'Hierarchical', 'KDTree')) {
#     $command = "$exePath --file $objFilePath --output $(Join-Path $outputPath $($zBufferChoice + 'ZBuffer.bmp')) --zbuffer $($zBufferChoice.ToLower())"
#     Write-Host $command
#     $output = & Invoke-Expression $command | Out-String
#     Write-Host $output 
#     $output -match "Elapsed:\s*([0-9]*\.?[0-9]+)\s*seconds" | Out-Null
#     $result += [PSCustomObject]@{
#         'Algorithm' = $zBufferChoice;
#         'Time' = $matches[1]
#     }

# }
#

# 增加了 --all 参数, 无需反复加载 model. 
$command = "$exePath --file `"$objFilePath`" --output `"$outputPath`" --all"
if ($echo) { Write-Host $command }
$output = & Invoke-Expression $command | Out-String
if ($echo) { Write-Host $output }

$output -split "`n" | ForEach-Object { 
    if ($_  -match "Elapsed: (\d+\.\d+) seconds for (\w+)\.") { 
        $time = "{0:N3}" -f [double]$matches[1] # 保留三位小数 
        $algorithm = $matches[2] 
        $result += [PSCustomObject]@{ Algorithm = $algorithm; Time = $time } 
    } 
}

$result