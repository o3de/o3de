
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: ly_get_list_relative_pal_filename will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/PythonCoverage/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/PythonCoverage/Code
ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${o3de_gem_restricted_path} ${o3de_gem_path} ${o3de_gem_name})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Add the PythonCoverage.Static target
# Note: We include the common files and the platform specific files which are set in pythoncoverage_common_files.cmake
# and in ${pal_dir}/pythoncoverage_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME PythonCoverage.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        pythoncoverage_files.cmake
        ${pal_dir}/pythoncoverage_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

# Here add PythonCoverage target, it depends on the PythonCoverage.Static
ly_add_target(
    NAME PythonCoverage ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        pythoncoverage_shared_files.cmake
        ${pal_dir}/pythoncoverage_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::PythonCoverage.Static
)

# By default, we will specify that the above target PythonCoverage would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME PythonCoverage.Clients NAMESPACE Gem TARGETS Gem::PythonCoverage)
ly_create_alias(NAME PythonCoverage.Servers NAMESPACE Gem TARGETS Gem::PythonCoverage)

# If we are on a host platform, we want to add the host tools targets like the PythonCoverage.Editor target which
# will also depend on PythonCoverage.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME PythonCoverage.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            pythoncoverage_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PUBLIC
                PYTHON_COVERAGE_EDITOR
            PRIVATE
                LY_TEST_IMPACT_DEFAULT_CONFIG_FILE=\"\"
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::PythonCoverage.Static
    )

    ly_add_target(
        NAME PythonCoverage.Editor MODULE
        NAMESPACE Gem
        AUTOMOC
        OUTPUT_NAME Gem.PythonCoverage.Editor
        FILES_CMAKE
            pythoncoverage_editor_shared_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                PYTHON_COVERAGE_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::PythonCoverage.Editor.Static
    )

    # By default, we will specify that the above target PythonCoverage would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME PythonCoverage.Tools    NAMESPACE Gem TARGETS Gem::PythonCoverage.Editor)
    ly_create_alias(NAME PythonCoverage.Builders NAMESPACE Gem TARGETS Gem::PythonCoverage.Editor)


endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for PythonCoverage.Static
    if(PAL_TRAIT_PYTHONCOVERAGE_TEST_SUPPORTED)
        # We support PythonCoverage.Tests on this platform, add PythonCoverage.Tests target which depends on PythonCoverage.Static
        ly_add_target(
            NAME PythonCoverage.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                pythoncoverage_files.cmake
                pythoncoverage_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::PythonCoverage.Static
        )

        # Add PythonCoverage.Tests to googletest
        ly_add_googletest(
            NAME Gem::PythonCoverage.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_PYTHONCOVERAGE_EDITOR_TEST_SUPPORTED)
            # We support PythonCoverage.Editor.Tests on this platform, add PythonCoverage.Editor.Tests target which depends on PythonCoverage.Editor
            ly_add_target(
                NAME PythonCoverage.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    pythoncoverage_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::PythonCoverage.Editor
            )

            # Add PythonCoverage.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::PythonCoverage.Editor.Tests
            )
        endif()
    endif()
endif()
