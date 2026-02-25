param(
    [switch]$PrepareOnly
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

if (Get-Variable PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
    $global:PSNativeCommandUseErrorActionPreference = $false
}

function Invoke-CheckedProcess {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $false)][string[]]$ArgumentList = @(),
        [Parameter(Mandatory = $false)][string]$FailMessage = 'External command failed.'
    )

    $proc = Start-Process -FilePath $FilePath -ArgumentList $ArgumentList -Wait -PassThru -NoNewWindow
    if ($proc.ExitCode -ne 0) {
        throw "$FailMessage ExitCode=$($proc.ExitCode)"
    }
}

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

    $needsClone = $true
    if (Test-Path $phpSdkDir) {
        if (Test-Path (Join-Path $phpSdkDir '.git')) {
            try {
                Invoke-CheckedProcess -FilePath 'git' -ArgumentList @('-C', $phpSdkDir, 'rev-parse', '--is-inside-work-tree') -FailMessage 'php-sdk-binary-tools directory exists but is not a valid git repository.'
                $needsClone = $false
                Write-Host 'Reusing existing php-sdk-binary-tools checkout.'
            } catch {
                Write-Host 'Existing php-sdk-binary-tools checkout is invalid; removing and recloning...'
                Remove-Item -Recurse -Force $phpSdkDir
            }
        } else {
            Write-Host 'Found stale PHP SDK tools directory; removing it before clone...'
            Remove-Item -Recurse -Force $phpSdkDir
        }
    }

    if ($needsClone) {
        Write-Host 'PHP SDK binary tools not found; cloning php-sdk-binary-tools...'
        Invoke-CheckedProcess -FilePath 'git' -ArgumentList @('clone', '--depth', '1', 'https://github.com/php/php-sdk-binary-tools.git', $phpSdkDir) -FailMessage 'Failed to clone php-sdk-binary-tools.'

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
        Invoke-CheckedProcess -FilePath 'cmd' -ArgumentList @('/c', 'buildconf', '--force') -FailMessage 'buildconf failed to generate configure.js.'
    }
}

if (-not (Test-Path configure.js)) {
    throw 'configure.js is still missing after running buildconf. Ensure config.w32 is present and PHP SDK binary tools are installed.'
}

Write-Host 'configure.js found, attempting Windows extension build...'
Invoke-CheckedProcess -FilePath 'cscript' -ArgumentList @('/nologo', 'configure.js', '--enable-gtk') -FailMessage 'configure.js failed.'
Invoke-CheckedProcess -FilePath 'nmake' -ArgumentList @() -FailMessage 'nmake failed.'

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
