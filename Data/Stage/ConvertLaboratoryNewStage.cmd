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
"%converter%" --input "Data\Stage\LaboratoryNewStage.tmj" --output "Data\Stage\LaboratoryNewStage.stage.json" --pixels-per-unit 32
if errorlevel 1 (
 echo Conversion failed. The current game stage was not changed.
 popd
 pause
 exit /b 1
)
if exist "Data\Stage\ConvertedStage.stage.json" (
 copy /y "Data\Stage\ConvertedStage.stage.json" "Data\Stage\ConvertedStage.before-new-stage.stage.json" >nul
 if errorlevel 1 (
  echo Backup failed. The current game stage was not changed.
  popd
  pause
  exit /b 1
 )
)
copy /y "Data\Stage\LaboratoryNewStage.stage.json" "Data\Stage\ConvertedStage.stage.json" >nul
if errorlevel 1 (echo Failed to install the new stage.) else (echo Converted successfully. Restart the game to load the new stage.)
popd
pause
