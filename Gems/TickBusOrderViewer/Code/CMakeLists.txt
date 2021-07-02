#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME TickBusOrderViewer.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        tickbusorderviewer_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
)

ly_add_target(
    NAME TickBusOrderViewer ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        tickbusorderviewer_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::TickBusOrderViewer.Static
)


# the above module is for use in all application types except builders
ly_create_alias(NAME TickBusOrderViewer.Servers  NAMESPACE Gem TARGETS Gem::TickBusOrderViewer)
ly_create_alias(NAME TickBusOrderViewer.Clients  NAMESPACE Gem TARGETS Gem::TickBusOrderViewer)
ly_create_alias(NAME TickBusOrderViewer.Tools    NAMESPACE Gem TARGETS Gem::TickBusOrderViewer)
