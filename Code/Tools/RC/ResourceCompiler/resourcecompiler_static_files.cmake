#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    IRCLog.h
    IConfig.cpp
    IConvertor.h
    platform_implRC.h
    platform_implRC.cpp
    ZipEncryptor.cpp
    ZipEncryptor.h
    ICfgFile.h
    IConfig.h
    IConvertor.h
    IMultiplatformConfig.h
    IRCLog.h
    IResCompiler.h  
    PakHelpers.cpp
    PakManager.cpp
    PakHelpers.h
    PakManager.h
    CfgFile.cpp
    Config.cpp
    DependencyList.cpp
    ExcelExport.cpp
    ExcelReport.cpp
    ExtensionManager.cpp
    ListFile.cpp
    PropertyVars.cpp
    ResourceCompiler_precompiled.cpp
    ResourceCompiler_precompiled.h
    TextFileReader.cpp
    CfgFile.h
    Config.h
    ConvertContext.h
    DebugLog.h
    DependencyList.h
    ExcelExport.h
    ExcelReport.h
    ExtensionManager.h
    ListFile.h
    MultiplatformConfig.h
    PropertyVars.h
    RcFile.h
    resource.h
    TextFileReader.h
    AssetFileInfo.h
    UpToDateFileHelpers.h
    NameConvertor.h
    SimpleString.h
    IProgress.h
    ResourceCompiler.rc
    FunctionThread.h
    ResourceCompiler.cpp
    ResourceCompiler.h
)

set(SKIP_UNITY_BUILD_INCLUSION_FILES
    IConfig.cpp
    platform_implRC.cpp
)
