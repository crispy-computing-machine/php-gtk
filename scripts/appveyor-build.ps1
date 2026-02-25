param(
    [switch]$PrepareOnly
)

$ErrorActionPreference = 'Stop'

Write-Host "Preparing AppVeyor build for php-gtk (PHP $env:PHP_VERSION, $env:ARCH)"

if (-not (Get-Command php -ErrorAction SilentlyContinue)) {
    choco install php --version=8.4.18 -y
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
