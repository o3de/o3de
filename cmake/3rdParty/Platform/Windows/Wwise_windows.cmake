#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(WWISE_WINDOWS_LIB_NAMES
    AkAutobahn
    SFLib
)

set(WWISE_VS_VER "vc160")

set(WWISE_LIB_PATH ${BASE_PATH}/SDK/x64_${WWISE_VS_VER}/$<IF:$<CONFIG:Debug>,Debug,$<IF:$<CONFIG:Profile>,Profile,Release>>/lib)

set(WWISE_LIBS)

foreach(libName IN LISTS WWISE_COMMON_LIB_NAMES WWISE_CODEC_LIB_NAMES WWISE_WINDOWS_LIB_NAMES WWISE_ADDITIONAL_LIB_NAMES)
    list(APPEND WWISE_LIBS ${WWISE_LIB_PATH}/${libName}.lib)
endforeach()

foreach(nonReleaseLibName IN LISTS WWISE_NON_RELEASE_LIB_NAMES)
    list(APPEND WWISE_LIBS $<$<NOT:$<CONFIG:Release>>:${WWISE_LIB_PATH}/${nonReleaseLibName}.lib>)
endforeach()

# windows OS libs
list(APPEND WWISE_LIBS
    dxguid.lib
    DInput8.lib
    $<$<NOT:$<CONFIG:Release>>:ws2_32.lib>
)
