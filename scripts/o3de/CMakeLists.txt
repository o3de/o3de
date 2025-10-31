#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

add_subdirectory(tests)

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${O3DE_ENGINE_RESTRICTED_PATH} ${LY_ROOT_FOLDER})
include(${pal_dir}/o3de_install_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

ly_install_files(FILES ../o3de.py 
    DESTINATION scripts
)

ly_install_files(FILES setup.py PROGRAMS
    DESTINATION scripts/o3de
)
ly_install_files(FILES README.txt
    DESTINATION scripts/o3de
)

ly_install_directory(
    DIRECTORIES
        o3de.egg-info
    VERBATIM
)

ly_install_directory(DIRECTORIES .
    EXCLUDE_PATTERNS
        tests
        Platform
)
