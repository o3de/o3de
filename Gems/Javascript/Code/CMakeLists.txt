
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: ly_get_list_relative_pal_filename will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/Javascript/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/Javascript/Code
ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${o3de_gem_restricted_path} ${o3de_gem_path} ${o3de_gem_name})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# This library is the 3rdParty Duktape that is in the Gem's External
ly_add_target(
    NAME Duktape.duktapelib ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        duktape_lib_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            ../External/duktape-2.6.0/src
        PUBLIC_HEADER
            Include
)

# Add the Javascript.Static target
# Note: We include the common files and the platform specific files which are set in javascript_common_files.cmake
# and in ${pal_dir}/javascript_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME Javascript.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        javascript_files.cmake
        ${pal_dir}/javascript_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::Duktape.duktapelib
        RUNTIME_DEPENDENCIES
            Gem::Duktape.duktapelib
)

# Here add Javascript target, it depends on the Javascript.Static
ly_add_target(
    NAME Javascript ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        javascript_shared_files.cmake
        ${pal_dir}/javascript_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Javascript.Static
)

# By default, we will specify that the above target Javascript would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME Javascript.Clients NAMESPACE Gem TARGETS Gem::Javascript)
ly_create_alias(NAME Javascript.Servers NAMESPACE Gem TARGETS Gem::Javascript)

# If we are on a host platform, we want to add the host tools targets like the Javascript.Editor target which
# will also depend on Javascript.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME Javascript.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            javascript_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::Javascript.Static
    )

    ly_add_target(
        NAME Javascript.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        OUTPUT_NAME Gem.Javascript.Editor
        FILES_CMAKE
            javascript_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::Javascript.Editor.Static
    )

    # By default, we will specify that the above target Javascript would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME Javascript.Tools    NAMESPACE Gem TARGETS Gem::Javascript.Editor)
    ly_create_alias(NAME Javascript.Builders NAMESPACE Gem TARGETS Gem::Javascript.Editor)


endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for Javascript.Static
    if(PAL_TRAIT_JAVASCRIPT_TEST_SUPPORTED)
        # We support Javascript.Tests on this platform, add Javascript.Tests target which depends on Javascript.Static
        ly_add_target(
            NAME Javascript.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                javascript_files.cmake
                javascript_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::Javascript.Static
        )

        # Add Javascript.Tests to googletest
        ly_add_googletest(
            NAME Gem::Javascript.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_JAVASCRIPT_EDITOR_TEST_SUPPORTED)
            # We support Javascript.Editor.Tests on this platform, add Javascript.Editor.Tests target which depends on Javascript.Editor
            ly_add_target(
                NAME Javascript.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    javascript_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::Javascript.Editor
            )

            # Add Javascript.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::Javascript.Editor.Tests
            )
        endif()
    endif()
endif()
