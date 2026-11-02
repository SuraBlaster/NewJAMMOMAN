@echo off
setlocal
pushd "%~dp0..\.."
set "converter=StageConverter.exe"

if not exist "%converter%" (
 echo Build StageConverter first.
 popd
 pause
 exit /b 1
)
copy /y "Data\Stage\LaboratoryCombinedCourse.stage.json" "Data\Stage\LaboratoryCombinedCourse.before-tiled.stage.json" >nul
if errorlevel 1 (
 echo Backup failed. Current game stage unchanged.
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
echo Converted successfully. Restart the game to load LaboratoryCombinedCourse.stage.json.
popd
pause
