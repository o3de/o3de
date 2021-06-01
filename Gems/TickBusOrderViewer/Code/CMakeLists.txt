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
