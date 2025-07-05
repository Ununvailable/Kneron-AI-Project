@echo off
setlocal enabledelayedexpansion
goto :main

:help_message
    echo.
    echo Usage: mptool2.bat [-sd] [-i {index}] [-l {number}] [-h]
    echo -sd        : Show device
    echo -d         : Test M.2 dual chip
    echo -i index   : Test the specified device by index
    echo ^            optional, default is -1: all devices
    echo -l number  : Set loop count
    echo ^            optional, default is 1
    echo -h         : Show this help message
    exit /b

:main
REM Initialize variables
set loop=1
set index=-1
set chip_count=1

REM Process command-line arguments
:process_args
if "%~1"=="" goto :execute

if "%1"=="-sd" (
    echo [info] Executing scan_device.exe to show devices
    res\bin\scan_devices.exe
    goto :end
) else if "%~1"=="-d" (
    set chip_count=2
    shift
    goto :process_args
) else if "%~1"=="-i" (
    set index=%~2
    shift
    shift
    goto :process_args
) else if "%~1"=="-l" (
    set loop=%~2
    shift
    shift
    goto :process_args
) else if "%1"=="/?" (
    goto :help_message
) else if "%1"=="-h" (
    goto :help_message
REM :help_message
REM     echo Help message:
REM     echo.
REM     echo Usage: mptool2.bat [-sd] [-i {index}] [-l {number}] [-h]
REM     echo -sd            : Show device
REM     echo -d             : Test Dual chip M.2
REM     echo -i {index}     : Test the specified device by index
REM     echo ^                optional, default is -1: all devices
REM     echo -l {number}    : Set loop count
REM     echo ^                optional, default is 1
REM     echo -h | /?        : Show this help message
REM     exit /b
) else (
    echo Invalid arguments. Use /? for help.
    exit /b 1
)

:execute
REM Default behavior if no parameters or only "-i" specified
if "%index%"=="-1" (
    echo [info] No device index specified, using default index -1^(all devices^)
    if %chip_count% equ 1 (
        set sidx_opt=-sidx 0
    )
) else (
    set sidx_opt=-sidx %index%
)

REM Execute test based on specified index and loop count
echo [info] Executing generic_inference.exe with index=%index% loop=%loop%
pushd .
cd res\bin
set model=..\..\res\models\KL720\YoloV5s_640_640_3\models_720.nef
generic_inference.exe -target KL720 %sidx_opt% -model %model% -l %loop% > temp.txt
popd

REM Check if stdout contains "connect 1 device(s) ... OK"
findstr /C:"connect %chip_count% device(s) ... OK" res\bin\temp.txt > nul
if %errorlevel% equ 0 (
    set device_connection_check_pass=1
) else (
    echo FAIL ^>^> Device connection check failed
    set device_connection_check_pass=0
    set log_dir=test_log\fail
    goto :check_done
)

REM Check if stdout contains the string in golden.txt
set /p golden=<"res\golden.txt"
findstr /C:"%golden%" res\bin\temp.txt > nul
echo.
if %errorlevel% equ 0 (
    if %device_connection_check_pass% equ 1 (
        echo PASS ^>^> Golden patten found in the output
        set log_dir=test_log\pass
    )
) else (
    echo FAIL ^>^> Golden patten not found in the output
    set log_dir=test_log\fail
)
:check_done

REM Get the current date and time in the format YYYYMMDD_HHMMSS
for /f "skip=1 tokens=1-6" %%G in ('wmic path Win32_LocalTime get Day^,Hour^,Minute^,Month^,Second^,Year /format:table') do (
    IF "%%~L"=="" goto :s_done
        set /a "day=10000*%%L+100*%%J+%%G"
        set /a "time=3600*24*%%I+60*%%H+%%K"
)
:s_done

REM Format the date and time into a string
set "date_time=!day!_!time!"
REM echo Date and time: %date_time%

REM Run the program and save its output to a temporary file
pushd .
cd res\bin
generic_command.exe -target KL720 %sidx_opt% > gen.txt

REM Use findstr to extract the kn_number value
for /f "tokens=2" %%a in ('type gen.txt ^| findstr "kn_number"') do (
    set "kn_number=%%a"
)
del /Q gen.txt
popd

if %chip_count% equ 2 (
    set log_file=dual_%kn_number%_%date_time%.txt
) else (
    set log_file=%kn_number%_%date_time%.txt
)

REM echo log_file: %kn_number%_%date_time%.txt

REM Save stdout to appropriate log directory
mkdir %log_dir% 2>nul
move /Y res\bin\temp.txt %log_dir%\%log_file% > nul

:end
REM Clean up
REM del /Q temp.txt
echo.
endlocal

