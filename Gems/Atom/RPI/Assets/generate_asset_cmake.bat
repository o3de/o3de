@ECHO off
setlocal enabledelayedexpansion

:: TAB equals 4 spaces
set TAB=    

:: Directory in which this batch file lives
set CURRENT_DIRECTORY=%~dp0

:: Destination file we're writing too
set OUTPUT_FILE=atom_rpi_asset_files.cmake

:: Write copyright header to top of file
echo # > %OUTPUT_FILE%
echo # All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or >> %OUTPUT_FILE%
echo # its licensors. >> %OUTPUT_FILE%
echo # >> %OUTPUT_FILE%
echo # For complete copyright and license terms please see the LICENSE at the root of this >> %OUTPUT_FILE%
echo # distribution (the "License"). All use of this software is governed by the License, >> %OUTPUT_FILE%
echo # or, if provided, by the license below or the license accompanying this file. Do not >> %OUTPUT_FILE%
echo # remove or modify any license notices. This file is distributed on an "AS IS" BASIS, >> %OUTPUT_FILE%
echo # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. >> %OUTPUT_FILE%
echo # >> %OUTPUT_FILE%
echo.>> %OUTPUT_FILE%

:: .cmake file first line should be "set(FILES"
echo set(FILES>> %OUTPUT_FILE%

:: Here we append the list of files
(
    :: Recursive for loop for every file in directory and subdirectories
    for /R %%f in (*.*) do (
    
        :: Get full file path and name
        set filePath=%%f
        
        :: Remove the current directory from the file path. This gives us the local path
        set relativeFilePath=!filePath:%CURRENT_DIRECTORY%=!

        :: Turns "\my\asset\folder" into "/my/asset/folder" for cmake
        set relativeFilePath=!relativeFilePath:\=/!
        
        :: Filter only relevant file types
        if !relativeFilePath:~-5!  == .pass         echo %TAB%!relativeFilePath!
        if !relativeFilePath:~-5!  == .azsl         echo %TAB%!relativeFilePath!
        if !relativeFilePath:~-6!  == .azsli        echo %TAB%!relativeFilePath!
        if !relativeFilePath:~-7!  == .shader       echo %TAB%!relativeFilePath!
        if !relativeFilePath:~-13! == .materialtype echo %TAB%!relativeFilePath!
    )
) >> %OUTPUT_FILE%

@echo ) >> %OUTPUT_FILE%