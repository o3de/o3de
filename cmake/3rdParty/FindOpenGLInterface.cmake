#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

find_package(OpenGL)

ly_add_external_target(
    NAME OpenGLInterface
    VERSION ""
    BUILD_DEPENDENCIES
        OpenGL::GL
)