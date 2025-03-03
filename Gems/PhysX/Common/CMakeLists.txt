#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

add_subdirectory(Code)

o3de_gem_setup("PhysXCommon")

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem)
endif()

add_subdirectory(3rdParty)
ly_add_external_target_path(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty)