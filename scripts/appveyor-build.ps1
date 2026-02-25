param(
    [switch]$PrepareOnly
)

$ErrorActionPreference = 'Stop'

Write-Host "Preparing AppVeyor build for php-gtk (PHP $env:PHP_VERSION, $env:ARCH)"

if (-not (Get-Command php -ErrorAction SilentlyContinue)) {
    choco feature disable --name=showDownloadProgress
    choco install php --version=8.4.18 --no-progress -y
    $env:Path += ';C:\tools\php84'
}

php -v

if ($PrepareOnly) {
    Write-Host 'PrepareOnly set; skipping native build steps.'
    exit 0
}

if (Test-Path configure.js) {
    Write-Host 'configure.js found, attempting Windows extension build...'
    if (Get-Command buildconf -ErrorAction SilentlyContinue) {
        buildconf
    }

    cscript /nologo configure.js --enable-gtk
    nmake
} else {
    Write-Host 'configure.js missing; skipping native build. This repo currently focuses on *nix config.m4.'
}

$artifactDir = 'artifacts'
$zipPath = Join-Path $artifactDir 'php-gtk-build.zip'

if (-not (Test-Path $artifactDir)) {
    New-Item -ItemType Directory -Path $artifactDir | Out-Null
}

$itemsToZip = @()
if (Test-Path 'x64\Release\php_gtk.dll') {
    $itemsToZip += 'x64\Release\php_gtk.dll'
}
if (Test-Path 'README.md') {
    $itemsToZip += 'README.md'
}
if (Test-Path 'examples\hello.php') {
    $itemsToZip += 'examples\hello.php'
}
if (Test-Path 'php-wrapper\Gtk') {
    $itemsToZip += 'php-wrapper\Gtk'
}

if ($itemsToZip.Count -gt 0) {
    if (Test-Path $zipPath) {
        Remove-Item $zipPath -Force
    }
    Compress-Archive -Path $itemsToZip -DestinationPath $zipPath -CompressionLevel Optimal
    Write-Host "Created AppVeyor artifact zip: $zipPath"
}
