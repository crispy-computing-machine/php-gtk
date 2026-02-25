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

$phpSdkDir = 'C:\tools\php-sdk-binary-tools'
if (-not (Get-Command buildconf -ErrorAction SilentlyContinue)) {
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        choco install git --no-progress -y
    }

    if (-not (Test-Path 'C:\tools')) {
        New-Item -ItemType Directory -Path 'C:\tools' | Out-Null
    }

    if (Test-Path $phpSdkDir) {
        if (-not (Test-Path (Join-Path $phpSdkDir '.git'))) {
            Write-Host 'Found stale PHP SDK tools directory; removing it before clone...'
            Remove-Item -Recurse -Force $phpSdkDir
        }
    }

    if (-not (Test-Path $phpSdkDir)) {
        Write-Host 'PHP SDK binary tools not found; cloning php-sdk-binary-tools...'
        cmd /c "git clone --depth 1 https://github.com/php/php-sdk-binary-tools.git $phpSdkDir"

        if (-not (Test-Path $phpSdkDir)) {
            throw 'Failed to clone php-sdk-binary-tools into C:\tools. Verify git/network access in AppVeyor.'
        }
    }

    $env:Path += ';C:\tools\php-sdk-binary-tools;C:\tools\php-sdk-binary-tools\bin'
}

php -v

if ($PrepareOnly) {
    Write-Host 'PrepareOnly set; skipping native build steps.'
    exit 0
}

if (-not (Get-Command nmake -ErrorAction SilentlyContinue)) {
    throw 'nmake is missing. Ensure Visual Studio Build Tools are available in this AppVeyor image.'
}

if (-not (Test-Path configure.js)) {
    if (Get-Command buildconf -ErrorAction SilentlyContinue) {
        Write-Host 'Generating configure.js via buildconf...'
        cmd /c buildconf --force
    }
}

if (-not (Test-Path configure.js)) {
    throw 'configure.js is still missing after running buildconf. Ensure config.w32 is present and PHP SDK binary tools are installed.'
}

Write-Host 'configure.js found, attempting Windows extension build...'
cscript /nologo configure.js --enable-gtk
nmake

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
