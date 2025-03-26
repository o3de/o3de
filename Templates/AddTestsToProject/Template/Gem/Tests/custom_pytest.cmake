# {BEGIN_LICENSE}
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# {END_LICENSE}

# Include this file in your Project/Gem/CMakeLists.txt using "include(Tests/custom_pytest.cmake)" at the end.

# Add a cache variable or set -DDISABLE_GEM_TESTS=ON to disable tests for this gem
# PAL_TRAIT_TEST_PYTEST_SUPPORTED is defined in the O3DE CMake code and will be
# FALSE if the current platform (like IOS for example) does not support python tests
if (DISABLE_GEM_TESTS OR NOT PAL_TRAIT_TEST_PYTEST_SUPPORTED)
    return()
endif()

# Register the test so that it runs when you run ctest
# eg, ctest -P <build-config> --test-dir <path-to-your-build-dir-you-used-when-you-ran-cmake>
# eg, ctest -P profile --test-dir build/windows
# eg, ctest -P debug --test-dir build\linux
# Unit tests.
ly_add_pytest(
    NAME PyTestMain_main_no_gpu  
        # the name of the test also has extra optional tags on the end of it, eg
        # optional addition of "_main", "_sandbox", "_periodic", "_smoke" puts it in that suite.  Unspecified = main
        # "_no_gpu" / "_requires_gpu" = it does not require a gpu to run, or it does.   Used to filter tests out on headless. 
    PATH ${CMAKE_CURRENT_LIST_DIR} # this can be a specific .pyfile or a directory to be scanned for files matching "*test*.py"
)
