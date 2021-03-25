REM 
REM 
REM  All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
REM  its licensors.
REM 
REM  For complete copyright and license terms please see the LICENSE at the root of this
REM  distribution (the "License"). All use of this software is governed by the License,
REM  or, if provided, by the license below or the license accompanying this file. Do not
REM  remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
REM  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM 

TITLE asset import test

SETLOCAL EnableExtensions
set EXE=AssetProcessor_tmp.exe
FOR /F %%x IN ('tasklist /NH /FI "IMAGENAME eq %EXE%"') DO IF %%x == %EXE% goto FOUND
echo Make sure asset processor is running before run this script.
goto FIN
:FOUND

SET F="..\..\Cache\SamplesProject\pc\samplesproject\objects"
 
IF EXIST %F% (
    RMDIR /S /Q %F%
    ECHO Detected folder at %F%
    ECHO Make sure there's no fail / crash in asset processor after all job finished.
) ELSE (ECHO folder %F% NOT FOUND)

:FIN
PAUSE