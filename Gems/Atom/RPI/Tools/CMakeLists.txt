#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_pip_install_local_package_editable(${CMAKE_CURRENT_LIST_DIR} atom_rpi_tools)

    if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        ly_add_pytest(
            NAME RPI::atom_rpi_tools_tests
            PATH ${CMAKE_CURRENT_LIST_DIR}/atom_rpi_tools/tests/
            TIMEOUT 30
            LABELS REQUIRES_tiaf
        )
    endif()
endif()

ly_install_directory(DIRECTORIES .
    EXCLUDE_PATTERNS tests
)
