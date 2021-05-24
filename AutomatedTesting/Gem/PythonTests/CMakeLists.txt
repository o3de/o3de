#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
# Enable when AWS Gems work on Linux and Android.
# add_subdirectory(AWS)
