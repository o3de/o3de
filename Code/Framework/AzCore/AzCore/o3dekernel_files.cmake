#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Module/Environment.cpp
    Module/Environment.h
    Debug/ITrace.cpp
)

if(NOT LY_MONOLITHIC_GAME)
    list(APPEND FILES 
        std/hash.cpp
    )
endif()
