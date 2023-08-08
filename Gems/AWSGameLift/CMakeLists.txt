#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (NOT gem_name)
    set(gem_name "AWSGameLift")
endif()

# Fallback to using the current source CMakeLists.txt directory as the gem root path
if (NOT gem_path)
    set(gem_path ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set(gem_json ${gem_path}/gem.json)

o3de_restricted_path(${gem_json} gem_restricted_path gem_parent_relative_path)

add_subdirectory(Code)

ly_install_directory(DIRECTORIES cdk)
