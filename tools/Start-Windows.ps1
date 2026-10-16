# SPDX-License-Identifier: GPL-3.0-or-later
# Windows PowerShell 5.1 compatible. Does not change the default distribution.
[CmdletBinding()]
param([switch]$InstallEnvironment)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$Distribution = 'Ubuntu-24.04'
$SourceRoot = Split-Path -Parent $PSScriptRoot
try {
    if (-not [Environment]::Is64BitOperatingSystem -or $env:PROCESSOR_ARCHITECTURE -eq 'ARM64') {
        throw 'This build currently requires an x64 Windows PC.'
    }
    if ([Environment]::OSVersion.Version.Build -lt 19041) { throw 'Use Windows 10 build 19041 or later, or Windows 11.' }
    $Wsl = Join-Path $env:WINDIR 'System32\wsl.exe'
    if (-not [Environment]::Is64BitProcess) { $Wsl = Join-Path $env:WINDIR 'Sysnative\wsl.exe' }
    if (-not (Test-Path -LiteralPath $Wsl)) { throw 'Windows WSL component is unavailable. See https://learn.microsoft.com/windows/wsl/install' }
    if ($InstallEnvironment) {
        & $Wsl --install --distribution $Distribution --no-launch
        if ($LASTEXITCODE -notin @(0,3010)) { throw 'Windows did not complete WSL setup. Read its message above.' }
        Write-Host 'Restart Windows if requested, then run Start-Windows.cmd again.'
        exit 0
    }
    $Installed = @(& $Wsl --list --quiet 2>$null) -replace "`0", ''
    if ($LASTEXITCODE -ne 0 -or $Installed -notcontains $Distribution) {
        Write-Host 'DKR-R builds locally using Ubuntu 24.04 in WSL2. No Windows Python is needed.'
        $Choice = Read-Host 'Install this Microsoft build environment now? [y/N]'
        if ($Choice -ne 'y') { Write-Host 'Nothing installed.'; exit 0 }
        # Elevate only the WSL installation, never the game build.
        $Arguments = '-NoLogo -NoProfile -ExecutionPolicy Bypass -File "{0}" -InstallEnvironment' -f $PSCommandPath
        $Process = Start-Process -FilePath 'powershell.exe' -Verb RunAs -ArgumentList $Arguments -Wait -PassThru
        if ($Process.ExitCode -ne 0) { throw 'The environment installation was cancelled or failed.' }
        Write-Host 'Run Start-Windows.cmd again after any requested Windows restart.'
        exit 0
    }
    Write-Host 'Opening the Ubuntu build environment. On its first launch, finish the Linux user setup.'
    & $Wsl --distribution $Distribution --exec true
    if ($LASTEXITCODE -ne 0) { throw 'Ubuntu is not ready. Open Ubuntu 24.04 from Start, finish its setup, then try again.' }
    $Kernel = (& $Wsl --distribution $Distribution --exec uname -r | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or $Kernel -notmatch '(?i)WSL2') { throw 'Ubuntu must use WSL2. Run: wsl --set-version Ubuntu-24.04 2' }
    $UserId = (& $Wsl --distribution $Distribution --exec id -u | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or $UserId -eq '0') { throw 'Finish Ubuntu setup with an ordinary Linux user before building. Do not build as root.' }
    $LinuxRoot = (& $Wsl --distribution $Distribution --exec wslpath -a -u $SourceRoot | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $LinuxRoot.StartsWith('/')) { throw 'Could not locate the extracted source folder in WSL.' }
    & $Wsl --distribution $Distribution --exec bash ($LinuxRoot + '/tools/start_build_assistant.sh') --stage-windows
    if ($LASTEXITCODE -ne 0) { throw 'The build assistant stopped. See the error above.' }
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
