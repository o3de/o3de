
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: ly_get_list_relative_pal_filename will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/MaterialEditor/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/MaterialEditor/Code
ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${o3de_gem_restricted_path} ${o3de_gem_path} ${o3de_gem_name})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Add the MaterialEditor.Static target
# Note: We include the common files and the platform specific files which are set in materialeditor_common_files.cmake
# and in ${pal_dir}/materialeditor_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME MaterialEditor.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        materialeditor_files.cmake
        ${pal_dir}/materialeditor_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
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

# Here add MaterialEditor target, it depends on the MaterialEditor.Static
ly_add_target(
    NAME MaterialEditor ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        materialeditor_shared_files.cmake
        ${pal_dir}/materialeditor_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::MaterialEditor.Static
)

# By default, we will specify that the above target MaterialEditor would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME MaterialEditor.Clients NAMESPACE Gem TARGETS Gem::MaterialEditor)
ly_create_alias(NAME MaterialEditor.Servers NAMESPACE Gem TARGETS Gem::MaterialEditor)

# If we are on a host platform, we want to add the host tools targets like the MaterialEditor.Editor target which
# will also depend on MaterialEditor.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME MaterialEditor.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTORCC
        FILES_CMAKE
            materialeditor_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::MaterialEditor.Static
    )

    ly_add_target(
        NAME MaterialEditor.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            materialeditor_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::MaterialEditor.Editor.Static
    )

    # By default, we will specify that the above target MaterialEditor would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME MaterialEditor.Tools    NAMESPACE Gem TARGETS Gem::MaterialEditor.Editor)
    ly_create_alias(NAME MaterialEditor.Builders NAMESPACE Gem TARGETS Gem::MaterialEditor.Editor)


endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for MaterialEditor.Static
    if(PAL_TRAIT_MATERIALEDITOR_TEST_SUPPORTED)
        # We support MaterialEditor.Tests on this platform, add MaterialEditor.Tests target which depends on MaterialEditor.Static
        ly_add_target(
            NAME MaterialEditor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                materialeditor_files.cmake
                materialeditor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::MaterialEditor.Static
        )

        # Add MaterialEditor.Tests to googletest
        ly_add_googletest(
            NAME Gem::MaterialEditor.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_MATERIALEDITOR_EDITOR_TEST_SUPPORTED)
            # We support MaterialEditor.Editor.Tests on this platform, add MaterialEditor.Editor.Tests target which depends on MaterialEditor.Editor
            ly_add_target(
                NAME MaterialEditor.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    materialeditor_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::MaterialEditor.Editor
            )

            # Add MaterialEditor.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::MaterialEditor.Editor.Tests
            )
        endif()
    endif()
endif()
