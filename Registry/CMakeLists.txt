#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(LY_MONOLITHIC_GAME)
    return()
endif()

ly_install_directory(DIRECTORIES .)

ly_install_directory(DIRECTORIES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/Registry
    DESTINATION ${runtime_output_directory}
)
