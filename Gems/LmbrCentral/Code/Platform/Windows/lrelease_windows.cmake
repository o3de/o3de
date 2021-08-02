#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(lrelease_files
    ${QT_LRELEASE_EXECUTABLE}
    ${QT_PATH}/bin/Qt5Core.dll # this is a dependency of lrelease. Even in debug we use the release version
)
