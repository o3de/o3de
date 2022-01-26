
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/MotionMatching/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/MotionMatching/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${o3de_gem_restricted_path} ${o3de_gem_path} ${o3de_gem_name})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Add the MotionMatching.Static target
# Note: We include the common files and the platform specific files which are set in motionmatching_common_files.cmake
# and in ${pal_dir}/motionmatching_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME MotionMatching.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        motionmatching_files.cmake
        ${pal_dir}/motionmatching_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::EMotionFXStaticLib
            Gem::ImguiAtom.Static
)

# Here add MotionMatching target, it depends on the MotionMatching.Static
ly_add_target(
    NAME MotionMatching ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        motionmatching_shared_files.cmake
        ${pal_dir}/motionmatching_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::MotionMatching.Static
            Gem::ImGui.Static
            Gem::ImGui.ImGuiLYUtils
)

# By default, we will specify that the above target MotionMatching would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME MotionMatching.Clients NAMESPACE Gem TARGETS Gem::MotionMatching)
ly_create_alias(NAME MotionMatching.Servers NAMESPACE Gem TARGETS Gem::MotionMatching)

# If we are on a host platform, we want to add the host tools targets like the MotionMatching.Editor target which
# will also depend on MotionMatching.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME MotionMatching.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            motionmatching_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::MotionMatching.Static
    )

    ly_add_target(
        NAME MotionMatching.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        OUTPUT_NAME Gem.MotionMatching.Editor
        FILES_CMAKE
            motionmatching_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::MotionMatching.Editor.Static
    )

    # By default, we will specify that the above target MotionMatching would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME MotionMatching.Tools    NAMESPACE Gem TARGETS Gem::MotionMatching.Editor)
    ly_create_alias(NAME MotionMatching.Builders NAMESPACE Gem TARGETS Gem::MotionMatching.Editor)


endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for MotionMatching.Static
    if(PAL_TRAIT_MOTIONMATCHING_TEST_SUPPORTED)
        # We support MotionMatching.Tests on this platform, add MotionMatching.Tests target which depends on MotionMatching.Static
        ly_add_target(
            NAME MotionMatching.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                motionmatching_files.cmake
                motionmatching_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::EMotionFX.Tests.Static
                    Gem::MotionMatching.Static
        )

        # Add MotionMatching.Tests to googletest
        ly_add_googletest(
            NAME Gem::MotionMatching.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_MOTIONMATCHING_EDITOR_TEST_SUPPORTED)
            # We support MotionMatching.Editor.Tests on this platform, add MotionMatching.Editor.Tests target which depends on MotionMatching.Editor
            ly_add_target(
                NAME MotionMatching.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    motionmatching_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::MotionMatching.Editor
            )

            # Add MotionMatching.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::MotionMatching.Editor.Tests
            )
        endif()
    endif()
endif()
