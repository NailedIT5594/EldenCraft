param (
    [string]$Configuration = "Release",
    [string]$Platform = "x64"
)

$projectPath = "path\to\your\project.sln"
$msbuildPath = "path\to\msbuild.exe"
$outputPath = "path\to\your\output\EldenCraft.dll"
$destinationPath = "C:\Users\CW\Downloads\ModEngine-2.1.0.0-win64\ModEngine-2.1.0.0-win64\EldenCraft.dll"

& $msbuildPath $projectPath /p:Configuration=$Configuration /p:Platform=$Platform /p:PlatformToolset=v142 /p:RuntimeLibrary=MultiThreadedDLL

if (Test-Path $outputPath) {
    Copy-Item $outputPath $destinationPath -Force
    Write-Output "EldenCraft.dll copied to $destinationPath"
} else {
    Write-Output "EldenCraft.dll not found at $outputPath"
}
