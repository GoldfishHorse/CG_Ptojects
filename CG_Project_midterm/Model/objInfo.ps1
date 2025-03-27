function Get-OBJFileInfo {
    param (
        [Parameter(Position = 0)]
        [string]$folderPath = $PSScriptRoot
    )

    if (-Not (Test-Path $folderPath)) {
        Write-Error "The specified folder path does not exist."
        return
    }

    $objFiles = Get-ChildItem -Path $folderPath -Filter *.obj
    if ($objFiles.Count -eq 0) {
        Write-Host "There are no .obj files in the folder."
        return
    }

    $table = @()

    foreach ($file in $objFiles) {
        $size = [math]::round((Get-Item $file.FullName).Length / 1KB, 2)

        # Get-Content 太慢了, 改用 Select-String 实现
        # $vertexCount = 0
        # $faceCount = 0
        # Get-Content $file.FullName | ForEach-Object {
        #     if ($_ -match "^v\s+") { $vertexCount++ }
        #     if ($_ -match "^f\s+") { $faceCount++ }
        # }
        $vertexCount = (Select-String -Path $file.FullName -Pattern "^v\s+").Count 
        $faceCount = (Select-String -Path $file.FullName -Pattern "^f\s+").Count

        $table += [PSCustomObject]@{
            'Name'      = $file.Name
            'Size (KB)' = $size
            'Vertices'  = $vertexCount
            'Faces'     = $faceCount
        }
    }

    # $table #| Format-Table -AutoSize
    return $table
}

Get-OBJFileInfo
# Write-Host "Press Any Key to Continue..." 
# $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown") | Out-Null