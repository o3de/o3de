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
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME AutomatedTesting::PrefabTests
        TEST_SUITE main
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/prefab/TestSuite_Main.py
        TIMEOUT 1500
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
    )
endif()

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
