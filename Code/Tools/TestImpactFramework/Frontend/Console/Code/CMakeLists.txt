#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME TestImpact.Frontend.Console EXECUTABLE
    NAMESPACE AZ
    FILES_CMAKE
        testimpactframework_frontend_console_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::TestImpact.Runtime.Static
)
