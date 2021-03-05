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

ly_add_target(
    NAME CrashSupport STATIC
    NAMESPACE AZ
    FILES_CMAKE
        crash_handler_support_files.cmake
        Platform/${PAL_PLATFORM_NAME}/crash_handler_support_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
)
