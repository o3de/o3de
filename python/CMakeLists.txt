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

# common files to all platforms
ly_install_files(
    FILES
        get_python.cmake
        get_python_package_hash.cmake
        readme.md
        requirements.txt
    DESTINATION python
)

# platform specific files (they are PROGRAMS)
include(Platform/${PAL_PLATFORM_NAME}/install_files_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
ly_install_files(
    FILES ${install_files}
    PROGRAMS
    DESTINATION python
)
