#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)
ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME LmbrCentral.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        lmbrcentral_headers_files.cmake
        lmbrcentral_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_dir}
        PUBLIC
            include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            AZ::AzFramework
)

ly_add_target(
    NAME LmbrCentral.API HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        lmbrcentral_headers_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            include
)

ly_add_target(
    NAME LmbrCentral ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        lmbrcentral_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_dir}
        PUBLIC
            include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            AZ::AzFramework
            Gem::LmbrCentral.Static
)

# by default, load the above "Gem::LmbrCentral" module in Client and Server
ly_create_alias(NAME LmbrCentral.Clients NAMESPACE Gem TARGETS Gem::LmbrCentral)
ly_create_alias(NAME LmbrCentral.Servers NAMESPACE Gem TARGETS Gem::LmbrCentral)


if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME LmbrCentral.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            lmbrcentral_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                ${pal_dir}
            PUBLIC
                include
        COMPILE_DEFINITIONS
            PUBLIC
                LMBR_CENTRAL_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                ${additional_dependencies}
            PUBLIC
                3rdParty::Qt::Widgets
                Legacy::CryCommon
                Legacy::Editor.Headers
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
    )

    ly_add_target(
        NAME LmbrCentral.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            lmbrcentral_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                ${pal_dir}
            PUBLIC
                include
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Gui
                AZ::AzCore
                Gem::LmbrCentral.Static
                Gem::LmbrCentral.Editor.Static
                ${additional_dependencies}
    )

    include(${pal_dir}/lrelease_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

    ly_add_target_files(
        TARGETS LmbrCentral.Editor
        FILES ${lrelease_files}
    )

    # by default, load the above "Gem::LmbrCentral.Editor" module in dev tools
    ly_create_alias(NAME LmbrCentral.Builders NAMESPACE Gem TARGETS Gem::LmbrCentral.Editor)
    ly_create_alias(NAME LmbrCentral.Tools NAMESPACE Gem TARGETS Gem::LmbrCentral.Editor)

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME LmbrCentral.Mocks HEADERONLY
        NAMESPACE Gem
        FILES_CMAKE
            lmbrcentral_mocks_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Mocks
    )
    
    ly_add_target(
        NAME LmbrCentral.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            lmbrcentral_tests_files.cmake
            lmbrcentral_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzTestShared
                Legacy::CryCommon
                AZ::AzFramework
                Gem::LmbrCentral.Static
                Gem::LmbrCentral.Mocks
    )
    ly_add_googletest(
        NAME Gem::LmbrCentral.Tests
    )

    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME LmbrCentral.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                lmbrcentral_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
                    Source
                    Tests
            COMPILE_DEFINITIONS
                PRIVATE
                    LMBR_CENTRAL_EDITOR
            BUILD_DEPENDENCIES
                PRIVATE
                    3rdParty::Qt::Gui
                    3rdParty::Qt::Widgets
                    Legacy::CryCommon
                    Legacy::Editor.Headers
                    AZ::AzTest
                    AZ::AzCore
                    AZ::AzTestShared
                    AZ::AzToolsFramework
                    AZ::AzToolsFrameworkTestCommon
                    AZ::AssetBuilderSDK
                    AZ::AzManipulatorTestFramework.Static
                    Gem::LmbrCentral.Static
                    Gem::LmbrCentral.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::LmbrCentral.Editor.Tests
        )
    endif()
endif()
