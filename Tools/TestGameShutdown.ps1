param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug'
)

$projectRoot = Split-Path -Parent $PSScriptRoot
$gameExecutable = Join-Path $projectRoot "bin/x64/$Configuration/Game.exe"
if (!(Test-Path -LiteralPath $gameExecutable))
{
    throw "Build the game first: $gameExecutable"
}

$gameProcess = Start-Process -FilePath $gameExecutable -ArgumentList '--profile-loading-auto' `
    -WorkingDirectory $projectRoot -WindowStyle Hidden -PassThru

if (!$gameProcess.WaitForExit(60000))
{
    Stop-Process -Id $gameProcess.Id -Force
    throw 'Game startup/shutdown test timed out.'
}
if ($gameProcess.ExitCode -ne 0)
{
    throw "Game shutdown failed with exit code $($gameProcess.ExitCode)."
}

Write-Output 'PASS: game startup, first game frame, scene destruction and shutdown (exit code 0).'
