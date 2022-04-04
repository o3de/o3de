
# Add the Streamer.Static target
ly_add_target(
    NAME StreamerProfiler.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        streamerprofiler_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Code
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            Gem::ImGui.imguilib
)

# Here add Streamer target, it depends on the Streamer.Static
ly_add_target(
    NAME StreamerProfiler ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        streamerprofiler_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Code
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::StreamerProfiler.Static
)

# By default, we will specify that the above target Streamer would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME StreamerProfiler.Clients NAMESPACE Gem TARGETS Gem::StreamerProfiler)
ly_create_alias(NAME StreamerProfiler.Servers NAMESPACE Gem TARGETS Gem::StreamerProfiler)

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We support Streamer.Tests on this platform, add Streamer.Tests target which depends on Streamer.Static
    ly_add_target(
        NAME StreamerProfiler.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            streamerprofiler_files.cmake
            streamerprofiler_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Code
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFramework
                Gem::StreamerProfiler.Static
    )

    # Add Streamer.Tests to googletest
    ly_add_googletest(
        NAME Gem::StreamerProfiler.Tests
    )
endif()
