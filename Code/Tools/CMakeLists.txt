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

add_subdirectory(SceneAPI) # Needs to go before AssetProcessor since it provides a builder
add_subdirectory(AssetProcessor)
add_subdirectory(AWSNativeSDKInit)
add_subdirectory(AzTestRunner)
add_subdirectory(CryCommonTools)
add_subdirectory(CryXML)
add_subdirectory(HLSLCrossCompiler)
add_subdirectory(HLSLCrossCompilerMETAL)
add_subdirectory(News)
add_subdirectory(PythonBindingsExample)
add_subdirectory(RC)
add_subdirectory(RemoteConsole)
add_subdirectory(CrashHandler)
add_subdirectory(ShaderCacheGen)
add_subdirectory(DeltaCataloger)
add_subdirectory(SerializeContextTools)
add_subdirectory(AssetBundler)
add_subdirectory(GridHub)
add_subdirectory(Standalone)
add_subdirectory(TestImpactFramework)
add_subdirectory(ProjectManager)
