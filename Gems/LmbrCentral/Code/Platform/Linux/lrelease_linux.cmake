#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

add_custom_command(TARGET LmbrCentral.Editor POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -P "${LY_ROOT_FOLDER}/cmake/Platform/Linux/RPathChange.cmake"
            "$<TARGET_FILE_DIR:LmbrCentral.Editor>/lrelease"
            "$ORIGIN/../lib"
            "$ORIGIN"
    COMMENT "Patching lrelease..."
    VERBATIM
)

set(lrelease_files
    ${QT_LRELEASE_EXECUTABLE}
)
