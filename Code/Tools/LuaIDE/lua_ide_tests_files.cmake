#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    tests/main.cpp
    tests/FlatElementsTests.cpp
    # The elements of the flattened GUI, which lua_ide_files.cmake builds into the application too: the tests reach them here
    Source/FlatElements.h
    Source/FlatElements.cpp
)
