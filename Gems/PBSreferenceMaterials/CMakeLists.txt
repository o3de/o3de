#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This will export its "SourcePaths" to the generated "cmake_dependencies.<project>.assetbuilder.setreg"
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME PBSreferenceMaterials.Builders NAMESPACE Gem)
endif()
