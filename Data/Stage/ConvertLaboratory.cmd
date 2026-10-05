@echo off
setlocal
pushd "%~dp0..\.."
set "converter=bin\x64\Release\StageConverter.exe"
if not exist "%converter%" set "converter=bin\x64\Debug\StageConverter.exe"
if not exist "%converter%" (
 echo Build StageConverter first.
 popd
 pause
 exit /b 1
)
copy /y "Data\Stage\ConvertedStage.stage.json" "Data\Stage\ConvertedStage.before-tiled.stage.json" >nul
start /wait "" "%converter%" --input "Data\Stage\LaboratoryCourse.tmj" --output "Data\Stage\ConvertedStage.stage.json" --pixels-per-unit 32
if errorlevel 1 (echo Conversion failed. Check the TMJ map.) else (echo Converted successfully. Restart the game to load the stage.)
popd
pause
