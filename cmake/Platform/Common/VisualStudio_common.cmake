#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(CMAKE_GENERATOR MATCHES "Visual Studio 16")
    configure_file("${CMAKE_CURRENT_LIST_DIR}/Directory.Build.props" "${CMAKE_BINARY_DIR}/Directory.Build.props" COPYONLY)
endif()