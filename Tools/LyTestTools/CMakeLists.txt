#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# SPEC-1417 will investigate and fix this
if(NOT PAL_PLATFORM_NAME STREQUAL "Mac")
    add_subdirectory(tests)
endif()

ly_install_directory(DIRECTORIES .)
