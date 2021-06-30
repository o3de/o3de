#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME CrySystem.XMLBinary STATIC
    NAMESPACE Legacy
    FILES_CMAKE
        crysystem_xmlbinary_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
)
