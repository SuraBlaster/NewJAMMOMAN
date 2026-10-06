param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [switch]$Manual
)
$projectRoot = Split-Path $PSScriptRoot -Parent
$executable = Join-Path $projectRoot "bin/x64/$Configuration/Game.exe"
if (!(Test-Path -LiteralPath $executable)) { throw "Build $Configuration first: $executable" }
$argument = if ($Manual) { '--profile-loading' } else { '--profile-loading-auto' }
# Keep the window visible for a user-initiated measurement.
$process = Start-Process -FilePath $executable -ArgumentList $argument -WorkingDirectory $projectRoot -PassThru
$process.WaitForExit()
Write-Output "Exit code: $($process.ExitCode)"
Write-Output "Log: $(Join-Path $projectRoot 'loading-profile.log')"
