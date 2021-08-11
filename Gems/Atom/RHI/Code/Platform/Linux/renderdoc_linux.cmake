#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Prevent bundling the renderdoc dll with a packaged title
if(NOT LY_MONOLITHIC_GAME)
    if(DEFINED ENV{"ATOM_RENDERDOC_PATH"})
        set(RENDERDOC_PATH ENV{"ATOM_RENDERDOC_PATH"})
    endif()

    if(RENDERDOC_PATH)
        # Normalize file path
        file(TO_CMAKE_PATH "${RENDERDOC_PATH}" RENDERDOC_PATH)

        if(EXISTS "${RENDERDOC_PATH}/librenderdoc.so")
            ly_add_external_target(
                NAME renderdoc
                VERSION
                3RDPARTY_ROOT_DIRECTORY ${RENDERDOC_PATH}
                INCLUDE_DIRECTORIES "."
                RUNTIME_DEPENDENCIES "${RENDERDOC_PATH}/librenderdoc.so"
            )
        endif()
    endif()
endif()