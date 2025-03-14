
# Include this file in your Project/Gem/CMakeLists.txt using "include(Tests/custom_google_test.cmake)" at the end.

# Add a cache variable or set -DDISABLE_GEM_TESTS=ON to disable tests for this gem
# PAL_TRAIT_TEST_GOOGLE_TEST_SUPPORTED is defined in the O3DE CMake code and will be
# FALSE if the current platform (like IOS for example) does not support Google Test
if (NOT DISABLE_GEM_TESTS AND PAL_TRAIT_TEST_GOOGLE_TEST_SUPPORTED)
    # add the test cpp code (add new files in the _test_files.cmake file)
    ly_add_target(
        NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            ${CMAKE_CURRENT_LIST_DIR}/test_files.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
    )

    # Register the test so that it runs when you run ctest
    # eg, ctest -P <build-config> --test-dir <path-to-your-build-dir>
    # eg, ctest -P profile --test-dir build/windows
    # eg, ctest -P debug --test-dir build\linux
    ly_add_googletest(
            NAME Gem::${gem_name}.Tests
        # optional parameters: 
        # TEST_SUITE "main" or "sandbox" or "smoke"
        # TARGET name of ly_target to run, if it does not match NAME of this test
    )
endif()

