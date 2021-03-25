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

@ECHO WARNING This batch file, %0, is deprecated.  See ant script build.xml
EXIT 0

REM TBD: once the build.xml script gets past CB5 QA, remove the below lines
REM Until then, keep this to compare the build.xml behavior in CB5 to the CB4 behavior below
@echo #1
IF EXIST .\Bin64vc141\rc\rc.exe (
    SET BINFOLDER=Bin64vc141
) ELSE (
    ECHO Cannot find rc.exe
    EXIT /b 1
)

.\%BINFOLDER%\rc\rc.exe /job=.\%BINFOLDER%\rc\RCJob_Build_RPGSample_paks.xml > BuildRPGSamplePaks.log
del TempRC\RPGsample /s /q
del Build\RPGSample /s /q

@echo #2
@echo Move (not copy) these files into another folder, zip it up so it retains the same folder structure. That way someone could just extract the .zip file and have everything go to the right place
xcopy RPGSample\*.dds SourceAssets\RPGSample /s /i
xcopy RPGSample\*.tif SourceAssets\RPGSample /s /i
xcopy RPGSample\*.psd SourceAssets\RPGSample /s /i

del RPGSample\*.dds /s /q /f
del RPGSample\*.tif /s /q /f
del RPGSample\*.psd /s /q /f


@echo #3 We'll deliver the packaged engine and the assets that were moved in #2 separately, so they can choose to download the source art or not (an extra 15GB or so)

@echo If they choose to extract the source art, they'll want to run 
@echo .\%BINFOLDER%\rc\rc.exe /job=.\Bin64\rc\RCJob_Compile_RPGSample_Textures.xml