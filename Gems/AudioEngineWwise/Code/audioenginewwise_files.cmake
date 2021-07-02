#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Source/AudioEngineWwiseGemSystemComponent.cpp
    Source/AudioEngineWwiseGemSystemComponent.h
    Source/Engine/ATLEntities_wwise.h
    Source/Engine/AudioSourceManager.h
    Source/Engine/AudioSystemImpl_wwise.h
    Source/Engine/AudioSystemImplCVars.h
    Source/Engine/Common_wwise.h
    Source/Engine/Config_wwise.h
    Source/Engine/FileIOHandler_wwise.h
    Source/Engine/PluginRegistration_wwise.h
    Source/Engine/AudioSourceManager.cpp
    Source/Engine/AudioSystemImpl_wwise.cpp
    Source/Engine/AudioSystemImplCVars.cpp
    Source/Engine/Common_wwise.cpp
    Source/Engine/Config_wwise.cpp
    Source/Engine/FileIOHandler_wwise.cpp
    Source/Engine/AudioInput/AudioInputFile.cpp
    Source/Engine/AudioInput/AudioInputFile.h
    Source/Engine/AudioInput/AudioInputMicrophone.cpp
    Source/Engine/AudioInput/AudioInputMicrophone.h
    Source/Engine/AudioInput/AudioInputStream.cpp
    Source/Engine/AudioInput/AudioInputStream.h
    Source/Engine/AudioInput/WavParser.cpp
    Source/Engine/AudioInput/WavParser.h
)
   
# Skip the following file that is also used in the editor shared target so the compiler will recognize its the same symbol
set(SKIP_UNITY_BUILD_INCLUSION_FILES
    Source/AudioEngineWwiseGemSystemComponent.cpp
)

