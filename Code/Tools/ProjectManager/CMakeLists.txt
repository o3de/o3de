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

# This will set python_package_name to whatever the package 'Python' is associated with
ly_get_package_association(Python python_package_name)
if (NOT python_package_name)
    set(python_package_name "python-no-package-assocation-found")
    message(WARNING "Python was not found in the package assocation list.  Did someone call ly_associate_package(xxxxxxx Python) ?")
endif()

ly_add_target(
    NAME ProjectManager APPLICATION
    OUTPUT_NAME o3de
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    AUTORCC
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
        PUBLIC
            .
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
            AZ::AzToolsFramework
            AZ::AzQtComponents
)