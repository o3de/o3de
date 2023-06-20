#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Query the gem name from the gem.json file if possible
# otherwise fallback to using Atom_TestData
o3de_find_ancestor_gem_root(gem_root_path gem_name "${CMAKE_CURRENT_SOURCE_DIR}")
if (NOT gem_name)
    set(gem_name "Atom_TestData")
endif()

# This will export the path to the directory containing the gem.json
# to the "SourcePaths" entry within the "cmake_dependencies.<project>.assetbuilder.setreg"
# which is generated when cmake is run
# This path is the gem root directory
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem)
    # Allows Tool applications to have this gem be active as well
    # This allows @gemroot:${gem_name}@ alias to be used to resolve the path to this gem
    ly_create_alias(NAME ${gem_name}.Tools NAMESPACE Gem)
endif()

