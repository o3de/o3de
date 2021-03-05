#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

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

# DEPRECATED: this target was marked as deprecated in the VS filter
ly_add_target(
    NAME AmazonStyle APPLICATION
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    FILES_CMAKE
        AzQtComponents/azqtcomponents_style_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            AzQtComponents
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Widgets
            AZ::AzQtComponents
)

ly_add_target(
    NAME AmazonQtControlGallery APPLICATION
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
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzQtComponents
                AZ::AzTest
    )
    ly_add_googletest(
        NAME AZ::AzQtComponents.Tests
    )
endif()