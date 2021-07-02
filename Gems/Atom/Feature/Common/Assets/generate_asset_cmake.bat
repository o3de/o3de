@ECHO off
REM Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM 
REM SPDX-License-Identifier: Apache-2.0 OR MIT

setlocal enabledelayedexpansion

:: TAB equals 4 spaces
set TAB=    

:: Directory in which this batch file lives
set CURRENT_DIRECTORY=%~dp0

:: Destination file we're writing too
set OUTPUT_FILE=atom_feature_common_asset_files.cmake

:: Write copyright header to top of file
echo # > %OUTPUT_FILE%
echo # Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution. >> %OUTPUT_FILE%
echo # >> %OUTPUT_FILE%
echo # SPDX-License-Identifier: Apache-2.0 OR MIT >> %OUTPUT_FILE%
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
        if !relativeFilePath:~-4!  == .lua          echo %TAB%!relativeFilePath!
        if !relativeFilePath:~-5!  == .pass         echo %TAB%!relativeFilePath!
        if !relativeFilePath:~-5!  == .azsl         echo %TAB%!relativeFilePath!
        if !relativeFilePath:~-6!  == .azsli        echo %TAB%!relativeFilePath!
        if !relativeFilePath:~-7!  == .shader       echo %TAB%!relativeFilePath!
        if !relativeFilePath:~-13! == .materialtype echo %TAB%!relativeFilePath!
    )
) >> %OUTPUT_FILE%

@echo ) >> %OUTPUT_FILE%
