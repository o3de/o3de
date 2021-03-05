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

set(PATH_TO_LIBS ${BASE_PATH}/lib/vc15win64-cmake)
set(PATH_TO_DLLS ${BASE_PATH}/bin/vc15win64-cmake)

# if(LY_MONOLITHIC_GAME)
set(BLAST_LIBS
    ${PATH_TO_LIBS}/NvBlast$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
    ${PATH_TO_LIBS}/NvBlastExtAssetUtils$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
    ${PATH_TO_LIBS}/NvBlastExtAuthoring$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
    ${PATH_TO_LIBS}/NvBlastExtExporter$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
    ${PATH_TO_LIBS}/NvBlastExtImport$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
    ${PATH_TO_LIBS}/NvBlastExtPhysX$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
    ${PATH_TO_LIBS}/NvBlastExtPxSerialization$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
    ${PATH_TO_LIBS}/NvBlastExtSerialization$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
    ${PATH_TO_LIBS}/NvBlastExtShaders$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
    ${PATH_TO_LIBS}/NvBlastExtStress$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
    ${PATH_TO_LIBS}/NvBlastExtTkSerialization$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
    ${PATH_TO_LIBS}/NvBlastGlobals$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
    ${PATH_TO_LIBS}/NvBlastTk$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.lib
)
# else()
set(BLAST_RUNTIME_DEPENDENCIES
    ${PATH_TO_DLLS}/NvBlast$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.dll
    ${PATH_TO_DLLS}/NvBlastExtAssetUtils$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.dll
    ${PATH_TO_DLLS}/NvBlastExtAuthoring$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.dll
    ${PATH_TO_DLLS}/NvBlastExtExporter$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.dll
    ${PATH_TO_DLLS}/NvBlastExtPhysX$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.dll
    ${PATH_TO_DLLS}/NvBlastExtPxSerialization$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.dll
    ${PATH_TO_DLLS}/NvBlastExtSerialization$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.dll
    ${PATH_TO_DLLS}/NvBlastExtShaders$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.dll
    ${PATH_TO_DLLS}/NvBlastExtStress$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.dll
    ${PATH_TO_DLLS}/NvBlastExtTkSerialization$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.dll
    ${PATH_TO_DLLS}/NvBlastGlobals$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.dll
    ${PATH_TO_DLLS}/NvBlastTk$<$<NOT:$<CONFIG:Release>>:$<UPPER_CASE:$<CONFIG>>>_x64.dll
)
# endif()
