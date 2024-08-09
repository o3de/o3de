#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

get_property(miniaudio_gem_root GLOBAL PROPERTY "@GEMROOT:MiniAudio@")
ly_add_external_target(
    NAME miniaudio 
    3RDPARTY_ROOT_DIRECTORY "${miniaudio_gem_root}/3rdParty/miniaudio"
    VERSION
)

ly_add_external_target(
    NAME stb_vorbis 
    3RDPARTY_ROOT_DIRECTORY "${miniaudio_gem_root}/3rdParty/stb_vorbis"
    VERSION
)

