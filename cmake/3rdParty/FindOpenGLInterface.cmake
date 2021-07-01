#
# Copyright (c) Contributors to the Open 3D Engine Project
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