@echo off
setlocal

set DFP=C:\Program Files\Microchip\MPLABX\v6.30\packs\Microchip\PIC18F-Q_DFP\1.28.451\xc8
set XC8=C:\Program Files\Microchip\xc8\v3.10\bin\xc8-cc.exe
set SRC=main.c config.c ioc.c
set OUT=IOC_Single.elf

cd /d "%~dp0"

echo Building IOC_Single for PIC18F47Q43...
"%XC8%" -mcpu=18F47Q43 "-mdfp=%DFP%" -I. -o %OUT% %SRC%

if %ERRORLEVEL% == 0 (
    echo Build successful: %OUT%
) else (
    echo Build FAILED with error %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
