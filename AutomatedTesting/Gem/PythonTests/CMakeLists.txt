#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

################################################################################
# Automated Tests
################################################################################

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

include(${pal_dir}/PAL_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

## Asset pipeline ##
add_subdirectory(assetpipeline)

## Atom Renderer ##
add_subdirectory(atom_renderer)

## Physics ##
add_subdirectory(physics)

## ScriptCanvas ##
add_subdirectory(scripting)

## White Box ##
add_subdirectory(WhiteBox)

## NvCloth ##
add_subdirectory(NvCloth)

## Prefab ##
add_subdirectory(prefab)

## Editor Python Bindings ##
add_subdirectory(EditorPythonBindings)

## Python Asset Builder ##
add_subdirectory(PythonAssetBuilder)

## Blast ##
add_subdirectory(Blast)

## Large Worlds ##
add_subdirectory(largeworlds)

## Editor ##
add_subdirectory(editor)

## Streaming ##
add_subdirectory(streaming)

## Smoke ##
add_subdirectory(smoke)

## AWS ##
add_subdirectory(AWS)
