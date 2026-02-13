@echo off
cd /d "%~dp0"
if exist "presentation.html" (
    start "" "presentation.html"
) else if exist "ICM20948_학습_발표.html" (
    start "" "ICM20948_학습_발표.html"
) else (
    echo presentation.html 파일이 없습니다.
    pause
)
