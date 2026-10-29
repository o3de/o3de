#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    tests/applicationManagerTests.cpp
    tests/tests_main.cpp
    tests/main.h
    tests/UtilsTests.cpp
    tests/FlatElementsTests.cpp
    # The elements of the GUI's sheet, which assetbundlergui_files.cmake builds too: the tests reach them here
    source/ui/FlatElements.h
    source/ui/FlatElements.cpp
)
