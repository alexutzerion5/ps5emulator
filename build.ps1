# PS5 Emulator Build Script for Windows

Write-Host "Building PS5 Emulator..." -ForegroundColor Green

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build"
}

Set-Location build

# Configure with CMake
cmake .. -G "Visual Studio 16 2019" -A x64

# Build
cmake --build . --config Release

Write-Host "Build complete!" -ForegroundColor Green
Write-Host "Run with: .\Release\ps5_emulator.exe" -ForegroundColor Green