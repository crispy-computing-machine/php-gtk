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


function Import-VsBuildEnvironment {
    if (Get-Command nmake -ErrorAction SilentlyContinue) {
        return
    }

    $vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
    $vsDevCmd = $null

    if (Test-Path $vswhere) {
        $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($LASTEXITCODE -eq 0 -and $installPath) {
            $candidate = Join-Path $installPath 'Common7\Tools\VsDevCmd.bat'
            if (Test-Path $candidate) {
                $vsDevCmd = $candidate
            }
        }
    }

    if (-not $vsDevCmd) {
        $fallback = 'C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat'
        if (Test-Path $fallback) {
            $vsDevCmd = $fallback
        }
    }

    if (-not $vsDevCmd) {
        throw 'Could not locate VsDevCmd.bat to initialize MSVC tools. Ensure Visual Studio Build Tools are installed in AppVeyor.'
    }

    Write-Host "Initializing MSVC build environment via: $vsDevCmd"
    $envDump = cmd /s /c "`"$vsDevCmd`" -arch=x64 -host_arch=x64 >nul && set"
    if ($LASTEXITCODE -ne 0) {
        throw 'Failed to initialize Visual Studio developer command prompt environment.'
    }

    foreach ($line in $envDump) {
        $idx = $line.IndexOf('=')
        if ($idx -gt 0) {
            $name = $line.Substring(0, $idx)
            $value = $line.Substring($idx + 1)
            Set-Item -Path "Env:$name" -Value $value
        }
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

Import-VsBuildEnvironment

if (-not (Get-Command nmake -ErrorAction SilentlyContinue)) {
    throw 'nmake is still missing after loading VsDevCmd. Ensure Visual Studio Build Tools (VC++) are installed in this AppVeyor image.'
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
