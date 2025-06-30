@ECHO OFF
REM "Usage: pre_build.bat clean existing fw_ncpu.bin and fw_ncpu_enc.bin"

SET UTILS_PATH=..\..\..\..\utils
SET BIN_OUT=fw_ncpu.bin
SET ENC_BIN_OUT=fw_ncpu_enc.bin

echo Deleting old binary file...

echo Deleting %ENC_BIN_OUT% 
echo Deleting %BIN_OUT%
echo Deleting %UTILS_PATH%\bin_gen\flash_bin\%BIN_OUT%
echo Deleting %UTILS_PATH%\bin_gen\flash_bin\%ENC_BIN_OUT%
echo Deleting %UTILS_PATH%\JLink_programmer\bin\%BIN_OUT%
echo Deleting %UTILS_PATH%\JLink_programmer\bin\%ENC_BIN_OUT%

IF EXIST %ENC_BIN_OUT% (del %ENC_BIN_OUT%)
IF EXIST %BIN_OUT% (del %BIN_OUT%)
IF EXIST %UTILS_PATH%\bin_gen\flash_bin\%BIN_OUT% (del %UTILS_PATH%\bin_gen\flash_bin\%BIN_OUT%)
IF EXIST %UTILS_PATH%\bin_gen\flash_bin\%ENC_BIN_OUT% (del %UTILS_PATH%\bin_gen\flash_bin\%ENC_BIN_OUT%)
IF EXIST %UTILS_PATH%\JLink_programmer\bin\%BIN_OUT% (del %UTILS_PATH%\JLink_programmer\bin\%BIN_OUT%)
IF EXIST %UTILS_PATH%\JLink_programmer\bin\%ENC_BIN_OUT% (del %UTILS_PATH%\JLink_programmer\bin\%ENC_BIN_OUT%)

ECHO End pre-build.bat