@echo off
:: Get current date parts
for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (
    set mm=%%a
    set dd=%%b
    set yyyy=%%c
)

:: Remove leading zeros if present
set /a mm=1%mm%-100
set /a dd=1%dd%-100

:: Ask user for folder name suffix
set /p suffix=Enter folder name suffix: 

:: Build folder name
set foldername=%mm%-%dd%-%yyyy%_%suffix%

:: Create the folder
mkdir "%foldername%"

echo Folder "%foldername%" created.
pause
