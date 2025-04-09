# 
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
# 
# 

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/DccScriptingInterface/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/DccScriptingInterface/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)


# If we are on a host platform, we want to add the host tools targets like the ${gem_name}.Editor target which
# will also depend on ${gem_name}.Editor.API target
if(PAL_TRAIT_BUILD_HOST_TOOLS)
     # The ${gem_name}.Editor.API target can be used by other gems that want to interact with the ${gem_name}.Editor module
    ly_add_target(
        NAME ${gem_name}.Editor.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            dccscriptinginterface_editor_api_files.cmake
            ${pal_dir}/dccscriptinginterface_editor_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
    )

    # The ${gem_name}.Editor.Private.Object target is an internal target
    # which is only to be used by this gems CMakeLists.txt and any subdirectories
    # Other gems should not use this target
    ly_add_target(
        NAME ${gem_name}.Editor.Private.Object STATIC
        NAMESPACE Gem
        AUTORCC
        FILES_CMAKE
            dccscriptinginterface_editor_private_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
    )

    if (PAL_TRAIT_HAS_QTFORPYTHON)
        ly_add_target(
            NAME ${gem_name}.Editor GEM_MODULE
            NAMESPACE Gem
            AUTOMOC
            FILES_CMAKE
                dccscriptinginterface_editor_shared_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                PUBLIC
                    Include
            BUILD_DEPENDENCIES
                PUBLIC
                    Gem::${gem_name}.Editor.API
                PRIVATE
                    Gem::${gem_name}.Editor.Private.Object
            RUNTIME_DEPENDENCIES
                Gem::QtForPython.Editor
        )

        # Inject the gem name into the Module source file
        ly_add_source_properties(
            SOURCES
                Source/Tools/DccScriptingInterfaceEditorModule.cpp
            PROPERTY COMPILE_DEFINITIONS
                VALUES
                    O3DE_GEM_NAME=${gem_name}
                    O3DE_GEM_VERSION=${gem_version})

        # By default, we will specify that the above target ${gem_name} would be used by
        # Tool and Builder type targets when this gem is enabled.  If you don't want it
        # active in Tools or Builders by default, delete one of both of the following lines:
        ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
        ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
        endif()

    # For the Tools and Builders variants of ${gem_name} Gem, an alias to the ${gem_name}.Editor API target will be made
    ly_create_alias(NAME ${gem_name}.Tools.API NAMESPACE Gem TARGETS Gem::${gem_name}.Editor.API)
    ly_create_alias(NAME ${gem_name}.Builders.API NAMESPACE Gem TARGETS Gem::${gem_name}.Editor.API)

endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for ${gem_name}.Editor.Tests

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
    endif()
endif()
