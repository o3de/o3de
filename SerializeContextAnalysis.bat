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

pushd
cd "%MYBATCHFILEDIRECTORY%"

set OUTPUTFOLDER=SerializeContextAnalysis

REM Attempt to determine the best BinFolder for rc.exe and AssetProcessorBatch.exe
call "%MYBATCHFILEDIRECTORY%\DetermineRCandAP.bat" SILENT

REM If a bin folder was registered, validate the presence of the binfolder/rc.exe
IF ERRORLEVEL 1 (
    ECHO Unable to locate an appropriate bin folder. Make sure binaries are copied or compiled from source.
    EXIT /b 1
)
ECHO Detected binary folder at %MYBATCHFILEDIRECTORY%%BINFOLDER%
ECHO Extracting data...
mkdir "%MYBATCHFILEDIRECTORY%%OUTPUTFOLDER%"
.\%BINFOLDER%\SerializeContextTools.exe dumpsc -output=../%OUTPUTFOLDER%/SerializeContext.json
.\%BINFOLDER%\SerializeContextTools.exe dumpfiles -files=*.slice;*.uicanvas;config/Editor.xml;config/Game.xml -output=../%OUTPUTFOLDER%/Dump/

ECHO Converting data to documentation.
rem This used to bring in jinja2 with pip, but this needs to instead be hash-checked with
rem requirements.txt instead.
call .\python\python.cmd .\Tools\SerializeContextAnalyzer\SerializeContextAnalyzer.py --source "./%OUTPUTFOLDER%/SerializeContext.json" --output "./%OUTPUTFOLDER%/Confluence/Serialize Context.md" --template ".\Tools\SerializeContextAnalyzer\ConfluenceWiki_SerializeContext.jinja" --split
call .\python\python.cmd .\Tools\SerializeContextAnalyzer\SerializeContextAnalyzer.py --source "./%OUTPUTFOLDER%/SerializeContext.json" --output "./%OUTPUTFOLDER%/Confluence/SystemComponents.md" --template ".\Tools\SerializeContextAnalyzer\ConfluenceWiki_SystemComponents.jinja" --issplit true --parentdoc "Serialize Context"
call .\python\python.cmd .\Tools\SerializeContextAnalyzer\SerializeContextAnalyzer.py --source "./%OUTPUTFOLDER%/SerializeContext.json" --output "./%OUTPUTFOLDER%/Confluence/EntityComponents.md" --template ".\Tools\SerializeContextAnalyzer\ConfluenceWiki_EntityComponents.jinja" --issplit true --parentdoc "Serialize Context"
call .\python\python.cmd .\Tools\SerializeContextAnalyzer\SerializeContextAnalyzer.py --source "./%OUTPUTFOLDER%/SerializeContext.json" --output "./%OUTPUTFOLDER%/SystemComponents.txt" --template ".\Tools\SerializeContextAnalyzer\Text_SystemComponents.jinja"
call .\python\python.cmd .\Tools\SerializeContextAnalyzer\SerializeContextAnalyzer.py --source "./%OUTPUTFOLDER%/SerializeContext.json" --output "./%OUTPUTFOLDER%/EntityComponents.txt" --template ".\Tools\SerializeContextAnalyzer\Text_EntityComponents.jinja"
call .\python\python.cmd .\Tools\SerializeContextAnalyzer\SerializeContextAnalyzer.py --source "./%OUTPUTFOLDER%/SerializeContext.json" --output "./%OUTPUTFOLDER%/Types.txt" --template ".\Tools\SerializeContextAnalyzer\Text_Types.jinja"
call .\python\python.cmd .\Tools\SerializeContextAnalyzer\SerializeContextAnalyzer.py --source "./%OUTPUTFOLDER%/SerializeContext.json" --output "./%OUTPUTFOLDER%/SerializeContext.txt" --template ".\Tools\SerializeContextAnalyzer\Text_SerializeContext.jinja"

popd
ECHO Done
