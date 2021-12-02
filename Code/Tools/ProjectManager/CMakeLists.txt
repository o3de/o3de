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

# This will set python_package_name to whatever the package 'Python' is associated with
ly_get_package_association(Python python_package_name)
if (NOT python_package_name)
    set(python_package_name "python-no-package-assocation-found")
    message(WARNING "Python was not found in the package assocation list.  Did someone call ly_associate_package(xxxxxxx Python) ?")
endif()


ly_add_target(
    NAME ProjectManager.Static STATIC
    NAMESPACE AZ
    AUTOMOC
    FILES_CMAKE
        project_manager_files.cmake
        Platform/${PAL_PLATFORM_NAME}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        Platform/${PAL_PLATFORM_NAME}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        Platform/Common/${PAL_TRAIT_COMPILER_ID}/projectmanager_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            PY_PACKAGE="${python_package_name}" 
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            Platform/${PAL_PLATFORM_NAME}
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Concurrent
            3rdParty::Qt::Widgets
            3rdParty::Python
            3rdParty::pybind11
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
)

ly_add_target(
    NAME ProjectManager APPLICATION
    OUTPUT_NAME o3de
    NAMESPACE AZ
    AUTORCC
    FILES_CMAKE
        project_manager_app_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Qt::Core
            3rdParty::Qt::Concurrent
            3rdParty::Qt::Widgets
            3rdParty::Python
            3rdParty::pybind11
            AZ::AzCore
            AZ::AzFramework
            AZ::AzQtComponents
            AZ::ProjectManager.Static
)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ProjectManager.Tests EXECUTABLE
        NAMESPACE AZ
        AUTORCC
        FILES_CMAKE
            project_manager_tests_files.cmake
            Platform/${PAL_PLATFORM_NAME}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Platform/${PAL_PLATFORM_NAME}
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Core
                3rdParty::Qt::Concurrent
                3rdParty::Qt::Widgets
                3rdParty::Python
                3rdParty::pybind11
                AZ::AzTest
                AZ::AzFramework
                AZ::AzFrameworkTestShared
                AZ::ProjectManager.Static
    )

    ly_add_googletest(
        NAME AZ::ProjectManager.Tests
        TEST_COMMAND $<TARGET_FILE:AZ::ProjectManager.Tests> --unittest
    )

endif()
