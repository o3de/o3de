#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# this file makes sure that the 3rd party components miniaudio and stb_vorbis are available to use.

# for licenses of the sub-parts, please see their repos in github, or, licenses.txt in this folder.

include(FetchContent)

FetchContent_Declare(
    stb_vorbis
    URL          https://raw.githubusercontent.com/nothings/stb/5736b15f7ea0ffb08dd38af21067c314d6a3aae9/stb_vorbis.c
    URL_HASH     MD5=36713ac98e445271e29547cc2d90b01f # from the main branch @ Jan 29 2023
    DOWNLOAD_NO_EXTRACT TRUE
    DOWNLOAD_NO_PROGRESS TRUE
    TLS_VERIFY TRUE
)

FetchContent_Declare(
    miniaudio
    URL         https://raw.githubusercontent.com/mackron/miniaudio/0.11.17/miniaudio.h
    URL_HASH    MD5=7233fe13f5d42c4df37acff873c72bf5 # from tag 0.11.17
    DOWNLOAD_NO_EXTRACT TRUE
    DOWNLOAD_NO_PROGRESS TRUE
    TLS_VERIFY TRUE
)

FetchContent_MakeAvailable(stb_vorbis miniaudio)

FetchContent_GetProperties(stb_vorbis SOURCE_DIR STB_VORBIS_SOURCE_DIR)
FetchContent_GetProperties(miniaudio SOURCE_DIR MINIAUDIO_SOURCE_DIR)

add_library(3rdParty::miniaudio IMPORTED INTERFACE GLOBAL ${MINIAUDIO_SOURCE_DIR}/miniaudio.h)
target_include_directories(3rdParty::miniaudio SYSTEM INTERFACE ${MINIAUDIO_SOURCE_DIR})

add_library(3rdParty::stb_vorbis IMPORTED INTERFACE GLOBAL ${STB_VORBIS_SOURCE_DIR}/stb_vorbis.c)
target_include_directories(3rdParty::stb_vorbis SYSTEM INTERFACE ${STB_VORBIS_SOURCE_DIR})

message(STATUS "Using miniaudio ${MINIAUDIO_SOURCE_DIR}/miniaudio.h - see ${CMAKE_CURRENT_LIST_DIR}/miniaudio/LICENSE.TXT")
message(STATUS "Using stb_vorbis ${STB_VORBIS_SOURCE_DIR}/stb_vorbis.c - see ${CMAKE_CURRENT_LIST_DIR}/stb_vorbis/LICENSE.TXT")
