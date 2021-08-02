# {BEGIN_LICENSE}
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# {END_LICENSE}

# This will export the path to the directory containing the gem.json
# to the "SourcePaths" entry within the "cmake_dependencies.<project>.assetbuilder.setreg"
# which is generated when cmake is run
# This path is the gem root directory
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME ${Name}.Builders NAMESPACE Gem)
endif()
