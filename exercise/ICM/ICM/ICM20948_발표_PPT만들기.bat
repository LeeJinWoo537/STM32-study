@echo off
chcp 65001 >nul
cd /d "%~dp0"

echo PPT 파일 만든 뒤 열기...
python make_pptx_no_lib.py
if exist "ICM20948_학습_발표.pptx" (
    start "" "ICM20948_학습_발표.pptx"
    echo PowerPoint에서 열었습니다.
) else (
    echo 생성 실패. Python이 설치되어 있는지 확인해 주세요.
)
pause
