@echo off

REM 
REM All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
REM its licensors.
REM
REM For complete copyright and license terms please see the LICENSE at the root of this
REM distribution (the "License"). All use of this software is governed by the License,
REM or, if provided, by the license below or the license accompanying this file. Do not
REM remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM

setlocal enabledelayedexpansion 

set ORIGINALDIRECTORY=%cd%
set MYBATCHFILEDIRECTORY=%~dp0
cd "%MYBATCHFILEDIRECTORY%"

REM Parse command line arguments for OPTIONAL parameters. Looking for --recompress or --use_fastest.
set RECOMPRESS_NAME=--recompress
set /A RECOMPRESS_VALUE=0
set USE_FASTEST_NAME=--use_fastest
set /A USE_FASTEST_VALUE=0
set GAME_NAME_CMD=--game
set "BINFOLDER_HINT_NAME=--binfolder-hint"
set "BINFOLDER_HINT="
:CmdLineArgumentsParseLoop
if "%1"=="" goto CmdLineArgumentsParsingDone
    if "%1"=="%RECOMPRESS_NAME%" (
        set /A RECOMPRESS_VALUE=1
        goto GotValidArgument
    )
    if "%1"=="%USE_FASTEST_NAME%" (
        set /A USE_FASTEST_VALUE=1
        goto GotValidArgument
    )
    if "%1"=="%GAME_NAME_CMD%" (
        set GAME_NAME=%2
        shift
        set GAME_NAME
        CALL :lowercase GAME_NAME GAME_NAME_LOWER
        set GAME_NAME_LOWER
        goto GotValidArgument
    )
    if "%1"=="%BINFOLDER_HINT_NAME%" (
        REM The next parameter is the hint directory
        set BINFOLDER_HINT=%2%
        shift
        goto GotValidArgument
    )
:InvalidCommandArgument
    REM If we are here, the user gave us an unexpected argument. Let them know and quit.
    echo "%1" is an invalid argument, "%GAME_NAME_CMD%" is required, optional arguments are "%RECOMPRESS_NAME%", "%USE_FASTEST_NAME%" or "%BINFOLDER_HINT_NAME%"
    echo --game: The name of the game to generate the auxiliary content for.
    echo --recompress: If present, the ResourceCompiler (RC.exe) will decompress and compress back each
    echo               PAK file found as they are transferred from the cache folder to the game_pc_pak folder.
    echo --use_fastest: As each file is being added to its PAK file, they will be compressed across all
    echo                available codecs (ZLIB, ZSTD and LZ4) and the one with the fastest decompression time
    echo                will be chosen. The default is to always use ZLIB.
    echo %BINFOLDER_HINT_NAME%^=^<folder_name^>: A hint to indicate the folder name to use for finding the windows binaries i.e Bin64vc142
    exit /b 1
:GotValidArgument
    shift
goto CmdLineArgumentsParseLoop
:CmdLineArgumentsParsingDone

if "%GAME_NAME%"=="" goto InvalidCommandArgument

REM Attempt to determine the best BinFolder for rc.exe and AssetProcessorBatch.exe
call "%MYBATCHFILEDIRECTORY%\DetermineRCandAP.bat" SILENT %BINFOLDER_HINT%

REM If a bin folder was registered, validate the presence of the binfolder/rc.exe
IF ERRORLEVEL 1 (
    ECHO unable to determine the locations of AssetProcessor and rc.exe.  Make sure that they are available or rebuild from source
    EXIT /b 1
)
ECHO Detected binary folder at %MYBATCHFILEDIRECTORY%%BINFOLDER%

echo ----- Processing Assets Using Asset Processor Batch ----
.\%BINFOLDER%\AssetProcessorBatch.exe /gamefolder=%GAME_NAME% /platforms=pc
IF ERRORLEVEL 1 GOTO AssetProcessingFailed

echo ----- Creating Packages ----
rem lowercase is intentional, since cache folders are lowercase on some platforms
.\%BINFOLDER%\rc.exe /job=%MYBATCHFILEDIRECTORY%Code\Tools\RC\Config\rc\RCJob_Generic_MakeAuxiliaryContent.xml /p=pc /game=%GAME_NAME_LOWER% /recompress=%RECOMPRESS_VALUE% /use_fastest=%USE_FASTEST_VALUE%
IF ERRORLEVEL 1 GOTO RCFailed
echo ----- Done -----
cd "%ORIGINALDIRECTORY%"
exit /b 0

:RCFailed
echo ---- RC PAK failed ----
cd "%ORIGINALDIRECTORY%"
exit /b 1

:AssetProcessingFailed
echo ---- ASSET PROCESSING FAILED ----
cd "%ORIGINALDIRECTORY%"
exit /b 1

:lowercase
SET _UCase=A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
SET _LCase=a b c d e f g h i j k l m n o p q r s t u v w x y z
SET _Lib_UCase_Tmp=!%1!
SET _Abet=%_LCase%
FOR %%Z IN (%_Abet%) DO SET _Lib_UCase_Tmp=!_Lib_UCase_Tmp:%%Z=%%Z!
SET %2=%_Lib_UCase_Tmp%
GOTO:EOF
