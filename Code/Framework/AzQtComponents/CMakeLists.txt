#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})

ly_add_target(
    NAME AzQtComponents SHARED
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        AzQtComponents/azqtcomponents_files.cmake
        AzQtComponents/Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            AzQtComponents
        PUBLIC
            .
            ${pal_dir}
    COMPILE_DEFINITIONS
        PRIVATE
            AZ_QT_COMPONENTS_EXPORT_SYMBOLS
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            AZ::AzFramework
        PUBLIC
            3rdParty::Qt::Widgets
            3rdParty::Qt::Svg
            3rdParty::Qt::Xml
            AZ::AzCore
)

ly_add_target(
    NAME O3DEQtControlGallery APPLICATION
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    FILES_CMAKE
        AzQtComponents/azqtcomponents_gallery_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            AzQtComponents
            AzQtComponents/Gallery
            ${pal_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Svg
            3rdParty::Qt::Widgets
            AZ::AzQtComponents
            AZ::AzToolsFramework
)

ly_add_target(
    NAME ReflectedPropertyEditorStandalone APPLICATION
    NAMESPACE AZ
    FILES_CMAKE
        AzQtComponents/azqtcomponents_rpestandalone_files.cmake
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Widgets
            AZ::AzToolsFramework
)

ly_add_target(
    NAME DPEDebugViewStandalone APPLICATION
    NAMESPACE AZ
    FILES_CMAKE
        AzQtComponents/azqtcomponents_debugviewstandalone_files.cmake
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Widgets
            AZ::AzToolsFramework
            AZ::AzQtComponents
        PUBLIC
            AUTOMOC
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME AzQtComponents.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            AzQtComponents/azqtcomponents_testing_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                AzQtComponents
                ${pal_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzQtComponents
                AZ::AzTest
    )
    ly_add_googletest(
        NAME AZ::AzQtComponents.Tests
        LABELS REQUIRES_tiaf
    )
endif()
