
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/MaterialEditor/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/MaterialEditor/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${o3de_gem_restricted_path} ${o3de_gem_path} ${o3de_gem_name})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# If we are on a host platform, we want to add the host tools targets like the O3DEMaterialEditor.Editor target which
# will also depend on O3DEMaterialEditor.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME O3DEMaterialEditor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTORCC
        FILES_CMAKE
            materialeditor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
            PRIVATE
                AZ::AzQtComponents
    )

    ly_add_target(
        NAME O3DEMaterialEditor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            materialeditor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                Include
            PRIVATE
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzCore
            PRIVATE
                Gem::O3DEMaterialEditor.Static
    )

    # By default, we will specify that the above target O3DEMaterialEditor would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME O3DEMaterialEditor.Tools    NAMESPACE Gem TARGETS Gem::O3DEMaterialEditor)
    ly_create_alias(NAME O3DEMaterialEditor.Builders NAMESPACE Gem TARGETS Gem::O3DEMaterialEditor)

endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # Add O3DEMaterialEditor.Tests target which depends on O3DEMaterialEditor
        ly_add_target(
            NAME O3DEMaterialEditor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                materialeditor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Gem::O3DEMaterialEditor
        )

        # Add O3DEMaterialEditor.Editor.Tests to googletest
        ly_add_googletest(
            NAME Gem::O3DEMaterialEditor.Tests
        )
    endif()
endif()
