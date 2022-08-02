#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/ONNX/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/ONNX/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

find_package(Onnx REQUIRED)

# The ONNX.API target declares the common interface that users of this gem should depend on in their targets
ly_add_target(
    NAME ONNX.API INTERFACE
    NAMESPACE Gem
    FILES_CMAKE
        onnx_api_files.cmake
        ${pal_dir}/onnx_api_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
           AZ::AzCore
           Gem::ImguiAtom.Static
           Gem::ImGui.Static
           Gem::ImGui.ImGuiLYUtils
)

# The ONNX.Private.Object target is an internal target
# It should not be used outside of this Gems CMakeLists.txt
ly_add_target(
    NAME ONNX.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        onnx_private_files.cmake
        ${pal_dir}/onnx_private_files.cmake
    TARGET_PROPERTIES
        O3DE_PRIVATE_TARGET TRUE
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
            Assets/rsz_twoV2.png
            ${Onnx_INCLUDE_DIR}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            "${Onnx_LIBS_DIR}/onnxruntime.lib"
            Gem::ImguiAtom.Static
            Gem::ImGui.Static
            Gem::ImGui.ImGuiLYUtils
)

# Here add ONNX target, it depends on the Private Object library and Public API interface
ly_add_target(
    NAME ONNX ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        onnx_shared_files.cmake
        ${pal_dir}/onnx_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::ONNX.API
            "${Onnx_LIBS_DIR}/onnxruntime.lib"
        PRIVATE
            Gem::ONNX.Private.Object
        INTERFACE
            Gem::ImguiAtom.Static
            Gem::ImGui.Static
            Gem::ImGui.ImGuiLYUtils
)

# By default, we will specify that the above target ONNX would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME ONNX.Clients NAMESPACE Gem TARGETS Gem::ONNX)
ly_create_alias(NAME ONNX.Servers NAMESPACE Gem TARGETS Gem::ONNX)

# For the Client and Server variants of ONNX Gem, an alias to the ONNX.API target will be made
ly_create_alias(NAME ONNX.Clients.API NAMESPACE Gem TARGETS Gem::ONNX.API)
ly_create_alias(NAME ONNX.Servers.API NAMESPACE Gem TARGETS Gem::ONNX.API)

# If we are on a host platform, we want to add the host tools targets like the ONNX.Editor MODULE target
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    # The ONNX.Editor.API target can be used by other gems that want to interact with the ONNX.Editor module
    ly_add_target(
        NAME ONNX.Editor.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            onnx_editor_api_files.cmake
            ${pal_dir}/onnx_editor_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
                Gem::ImguiAtom.Static
                Gem::ImGui.Static
                Gem::ImGui.ImGuiLYUtils
    )

    # The ONNX.Editor.Private.Object target is an internal target
    # which is only to be used by this gems CMakeLists.txt and any subdirectories
    # Other gems should not use this target
    ly_add_target(
        NAME ONNX.Editor.Private.Object STATIC
        NAMESPACE Gem
        FILES_CMAKE
            onnx_editor_private_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                $<TARGET_OBJECTS:Gem::ONNX.Private.Object>
                "${Onnx_LIBS_DIR}/onnxruntime.lib"
                Gem::ImGui.Static
                Gem::ImGui.ImGuiLYUtils
            INTERFACE
                Gem::ImguiAtom.Static
    )

    ly_add_target(
        NAME ONNX.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            onnx_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::ONNX.Editor.API
                "${Onnx_LIBS_DIR}/onnxruntime.lib"
            PRIVATE
                Gem::ONNX.Editor.Private.Object
            INTERFACE
                Gem::ImguiAtom.Static
                Gem::ImGui.Static
                Gem::ImGui.ImGuiLYUtils
    )

    # By default, we will specify that the above target ONNX would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME ONNX.Tools    NAMESPACE Gem TARGETS Gem::ONNX.Editor)
    ly_create_alias(NAME ONNX.Builders NAMESPACE Gem TARGETS Gem::ONNX.Editor)

    # For the Tools and Builders variants of ONNX Gem, an alias to the ONNX.Editor API target will be made
    ly_create_alias(NAME ONNX.Tools.API NAMESPACE Gem TARGETS Gem::ONNX.Editor.API)
    ly_create_alias(NAME ONNX.Builders.API NAMESPACE Gem TARGETS Gem::ONNX.Editor.API)

endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for ONNX.Tests
    if(PAL_TRAIT_ONNX_TEST_SUPPORTED)
        # We support ONNX.Tests on this platform, add dependency on the Private Object target
        ly_add_target(
            NAME ONNX.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                onnx_private_files.cmake
                onnx_tests_files.cmake
            TARGET_PROPERTIES
                O3DE_PRIVATE_TARGET TRUE
            INCLUDE_DIRECTORIES
                PUBLIC
                    Tests
                    Source
                    Source/Clients
                    ${Onnx_INCLUDE_DIR}
                    Include
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::ONNX.Private.Object
                PUBLIC
                    "${Onnx_LIBS_DIR}/onnxruntime.lib"
                    Gem::ONNX.API
        )

        # Add ONNX.Tests to googletest
        ly_add_googletest(
            NAME Gem::ONNX.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_ONNX_EDITOR_TEST_SUPPORTED)
            # We support ONNX.Editor.Tests on this platform, add ONNX.Editor.Tests target which depends on
            # private ONNX.Editor.Private.Object target
            ly_add_target(
                NAME ONNX.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    onnx_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::ONNX.Private.Object
                    PUBLIC
                        "${Onnx_LIBS_DIR}/onnxruntime.lib"
            )

            # Add ONNX.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::ONNX.Editor.Tests
            )
        endif()
    endif()
endif()
