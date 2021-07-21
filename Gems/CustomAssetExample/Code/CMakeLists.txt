#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME CustomAssetExample ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        customassetexample_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
)

# clients and servers use the above module.
ly_create_alias(NAME CustomAssetExample.Clients NAMESPACE Gem TARGETS CustomAssetExample)
ly_create_alias(NAME CustomAssetExample.Servers NAMESPACE Gem TARGETS CustomAssetExample)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME CustomAssetExample.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            customassetexample_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AssetBuilderSDK
    )

    # other tools use the above tools module:
    ly_create_alias(NAME CustomAssetExample.Builders NAMESPACE Gem TARGETS CustomAssetExample.Editor)
    ly_create_alias(NAME CustomAssetExample.Tools    NAMESPACE Gem TARGETS CustomAssetExample.Editor)

endif()
