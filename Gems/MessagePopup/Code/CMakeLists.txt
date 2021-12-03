#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME MessagePopup.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        messagepopup_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
            Gem::LyShine
)

ly_add_target(
    NAME MessagePopup ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        messagepopup_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::MessagePopup.Static
)

# MessagePopup is used only in client applications
ly_create_alias(NAME MessagePopup.Clients NAMESPACE Gem TARGETS Gem::MessagePopup)

