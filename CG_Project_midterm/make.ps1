param (
    [string]$target = "All"
)

$CC = "g++"
$CFLAGS = "-std=c++17 -Wall -I./src -I./src/ZBuffer -O2"
$LDFLAGS = "-ld3d9 -ld3dx9 -lcomdlg32"

$TARGET_UI = "ZBufferUI.exe"
$TARGET_CMD = "ZBufferCMD.exe"

$SOURCES = Get-ChildItem -Path '.\src' -Recurse -File -Filter *.cpp
$OBJECTS = $SOURCES -replace '\.cpp$', '.o'

$UI_OBJECTS = $OBJECTS | Where-Object { $_ -notmatch 'mainCMD\.o$' -and $_ -notmatch 'mainUI\.o$' }
$CMD_OBJECTS = $UI_OBJECTS + "src\mainCMD.o"
$UI_OBJECTS += "src\mainUI.o"


function EchoAndRun {
    param ( [string]$cmd )
    Write-Host $cmd 
    Invoke-Expression -Command $cmd
}

function All { 
    $OBJECTS | ForEach-Object { CompileSource $_ }
    EchoAndRun "$CC -o $TARGET_UI $($UI_OBJECTS -join ' ') $LDFLAGS"
    EchoAndRun "$CC -o $TARGET_CMD $($CMD_OBJECTS -join ' ') $LDFLAGS"
}


function CompileSource {
    param ($source)
    EchoAndRun "$CC -c -o $source $($source -replace '\.o$', '.cpp') $CFLAGS"
}

function Clean {
    Remove-Item -Force  $UI_OBJECTS
    Remove-Item -Force  $CMD_OBJECTS
    # Remove-Item -Force  $TARGET_UI
    # Remove-Item -Force  $TARGET_CMD
}


& $target
