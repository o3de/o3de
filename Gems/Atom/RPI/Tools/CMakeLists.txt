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

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_pip_install_local_package_editable(${CMAKE_CURRENT_LIST_DIR} atom_rpi_tools)

    if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        ly_add_pytest(
            NAME RPI::atom_rpi_tools_tests
            PATH ${CMAKE_CURRENT_LIST_DIR}/atom_rpi_tools/tests/
            TIMEOUT 30
        )
    endif()
endif()

