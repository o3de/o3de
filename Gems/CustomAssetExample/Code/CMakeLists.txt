#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
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

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/CustomAssetExample/CustomAssetExampleModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# clients and servers use the above module.
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS ${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS ${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS ${gem_name})

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE

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

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Source/CustomAssetExample/CustomAssetExampleEditorModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    # other tools use the above tools module:
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS ${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS ${gem_name}.Editor)

endif()
