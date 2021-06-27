# 
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
# 

# If we are on a host platform, we want to add the host tools targets like the PrefabDependencyViewer.Editor target which
# will also depend on PrefabDependencyViewer.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME PrefabDependencyViewer.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            prefabdependencyviewer_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
            PRIVATE
                Legacy::Editor.Headers
    )

    ly_add_target(
        NAME PrefabDependencyViewer.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        OUTPUT_NAME Gem.PrefabDependencyViewer.Editor
        FILES_CMAKE
            prefabdependencyviewer_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::PrefabDependencyViewer.Editor.Static
    )

    # By default, we will specify that the above target PrefabDependencyViewer would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME PrefabDependencyViewer.Tools    NAMESPACE Gem TARGETS Gem::PrefabDependencyViewer.Editor)
endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for PrefabDependencyViewer.Static

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        # We support PrefabDependencyViewer.Editor.Tests on this platform, add PrefabDependencyViewer.Editor.Tests
        # target which depends on PrefabDependencyViewer.Editor
        ly_add_target(
            NAME PrefabDependencyViewer.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                prefabdependencyviewer_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Gem::PrefabDependencyViewer.Editor
        )

        # Add PrefabDependencyViewer.Editor.Tests to googletest
        ly_add_googletest(
            NAME Gem::PrefabDependencyViewer.Editor.Tests
        )
    endif()
endif()
