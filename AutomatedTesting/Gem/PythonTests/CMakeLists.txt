#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

################################################################################
# Automated Tests
################################################################################

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

include(${pal_dir}/PAL_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

## Asset pipeline ##
add_subdirectory(assetpipeline)

## Atom Renderer ##
add_subdirectory(Atom)

## Physics ##
add_subdirectory(Physics)

## ScriptCanvas ##
add_subdirectory(scripting)

## White Box ##
add_subdirectory(WhiteBox)

## NvCloth ##
add_subdirectory(NvCloth)

## Prefab ##
add_subdirectory(Prefab)

## Editor Python Bindings ##
add_subdirectory(EditorPythonBindings)

## Python Asset Builder ##
add_subdirectory(PythonAssetBuilder)

## Large Worlds ##
add_subdirectory(largeworlds)

## Editor ##
add_subdirectory(editor)

## Streaming ##
add_subdirectory(streaming)

## Smoke ##
add_subdirectory(smoke)

## Terrain ##
add_subdirectory(Terrain)

## Multiplayer ##
add_subdirectory(Multiplayer)

## Integration tests for editor testing framework ##
add_subdirectory(editor_test_testing)

## Performance ##
add_subdirectory(Performance)
