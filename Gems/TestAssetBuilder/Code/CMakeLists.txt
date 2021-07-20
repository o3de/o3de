#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME TestAssetBuilder.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        testassetbuilder_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AssetBuilderSDK
)

ly_add_target(
    NAME TestAssetBuilder.Editor GEM_MODULE

    NAMESPACE Gem
    FILES_CMAKE
        testassetbuilder_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::TestAssetBuilder.Static
)

# the above module is for use in builders only
ly_create_alias(NAME TestAssetBuilder.Builders NAMESPACE Gem TARGETS Gem::TestAssetBuilder.Editor)
