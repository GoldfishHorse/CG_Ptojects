Add-Type -AssemblyName System.Windows.Forms

$form = New-Object System.Windows.Forms.Form
$form.Text = "User Run"
$form.Size = New-Object System.Drawing.Size(450, 380)  

$buttonSelectFile = New-Object System.Windows.Forms.Button
$buttonSelectFile.Text = "Select .obj"
$buttonSelectFile.Location = New-Object System.Drawing.Point(10, 20)
$buttonSelectFile.Width = 120 
$form.Controls.Add($buttonSelectFile)

$textBoxFilePath = New-Object System.Windows.Forms.TextBox
$textBoxFilePath.Location = New-Object System.Drawing.Point(150, 20)
$textBoxFilePath.Width = 270 
$form.Controls.Add($textBoxFilePath)

$labelYaw = New-Object System.Windows.Forms.Label
$labelYaw.Text = "Yaw (radian):"
$labelYaw.Location = New-Object System.Drawing.Point(10, 60)
$form.Controls.Add($labelYaw)

$textBoxYaw = New-Object System.Windows.Forms.TextBox
$textBoxYaw.Location = New-Object System.Drawing.Point(150, 60)
$textBoxYaw.Text = "0.0"
$textBoxYaw.Width = 270 
$form.Controls.Add($textBoxYaw)

$labelPitch = New-Object System.Windows.Forms.Label
$labelPitch.Text = "Pitch (radian):"
$labelPitch.Location = New-Object System.Drawing.Point(10, 100)
$form.Controls.Add($labelPitch)

$textBoxPitch = New-Object System.Windows.Forms.TextBox
$textBoxPitch.Location = New-Object System.Drawing.Point(150, 100)
$textBoxPitch.Text = "0.0"
$textBoxPitch.Width = 270 
$form.Controls.Add($textBoxPitch)

$labelZBuffer = New-Object System.Windows.Forms.Label
$labelZBuffer.Text = "ZBuffer:"
$labelZBuffer.Location = New-Object System.Drawing.Point(10, 140)
$form.Controls.Add($labelZBuffer)

$comboBoxZBuffer = New-Object System.Windows.Forms.ComboBox
$comboBoxZBuffer.Location = New-Object System.Drawing.Point(150, 140)
$comboBoxZBuffer.Width = 270 
$comboBoxZBuffer.Items.AddRange(@("No", "Naive", "Scanline", "Hierarchical", "Kdtree Hierarchical"))
$comboBoxZBuffer.SelectedItem = "Naive"
$form.Controls.Add($comboBoxZBuffer)

$buttonSelectOutput = New-Object System.Windows.Forms.Button
$buttonSelectOutput.Text = "Output:"
$buttonSelectOutput.Location = New-Object System.Drawing.Point(10, 180)
$buttonSelectOutput.Width = 120
$form.Controls.Add($buttonSelectOutput)


$textBoxOutputPath = New-Object System.Windows.Forms.TextBox
$textBoxOutputPath.Location = New-Object System.Drawing.Point(150, 180)
$textBoxOutputPath.Width = 270 
$textBoxOutputPath.Text = ".\output.bmp"
$form.Controls.Add($textBoxOutputPath)

$buttonRun = New-Object System.Windows.Forms.Button
$buttonRun.Text = "Run ZBufferCMD"
$buttonRun.Location = New-Object System.Drawing.Point(200, 220) 
$buttonRun.Width = 120 
$form.Controls.Add($buttonRun)

$buttonCancel = New-Object System.Windows.Forms.Button
$buttonCancel.Text = "Cancel"
$buttonCancel.Location = New-Object System.Drawing.Point(330, 220) 
$buttonCancel.Width = 100 
$form.Controls.Add($buttonCancel)

$buttonSelectFile.Add_Click({
    $openFileDialog = New-Object System.Windows.Forms.OpenFileDialog
    $openFileDialog.Filter = "OBJ Files|*.obj"
    if ($openFileDialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
        $textBoxFilePath.Text = $openFileDialog.FileName
    }
})

$buttonSelectOutput.Add_Click({
    $saveFileDialog = New-Object System.Windows.Forms.SaveFileDialog
    $saveFileDialog.Filter = "BMP Files|*.bmp"
    if ($saveFileDialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
        $textBoxOutputPath.Text = $saveFileDialog.FileName
    }
})

$buttonRun.Add_Click({
    $filePath = $textBoxFilePath.Text
    $yaw = $textBoxYaw.Text
    $pitch = $textBoxPitch.Text
    $outputPath = $textBoxOutputPath.Text
    $zbuffer = $comboBoxZBuffer.SelectedItem

    $arguments = "--file $filePath --yaw $yaw --pitch $pitch --output $outputPath --zbuffer $zbuffer"
    Start-Process -FilePath "ZBufferCMD.exe" -ArgumentList $arguments
})

$buttonCancel.Add_Click({
    $form.Close()
})

$form.ShowDialog()
