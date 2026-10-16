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
"%converter%" --input "Data\Stage\LaboratoryCombinedCourse.tmj" --output "Data\Stage\LaboratoryCombinedCourse.stage.json" --pixels-per-unit 32
if errorlevel 1 (
 echo Conversion failed. Current game stage unchanged.
 popd
 pause
 exit /b 1
)
copy /y "Data\Stage\ConvertedStage.stage.json" "Data\Stage\ConvertedStage.before-combined.stage.json" >nul
if errorlevel 1 (
 echo Backup failed. Current game stage unchanged.
 popd
 pause
 exit /b 1
)
copy /y "Data\Stage\LaboratoryCombinedCourse.stage.json" "Data\Stage\ConvertedStage.stage.json" >nul
if errorlevel 1 (echo Installation failed.) else (echo Converted successfully. Restart the game.)
popd
pause
