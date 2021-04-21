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
# Atom Renderer: Automated Tests
# Runs EditorPythonBindings (hydra) scripts inside the Editor to verify test results for the Atom renderer.
# Utilizes a combination of screenshot comparisons and log files to verify test results.
################################################################################

if(PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_BUILD_TESTS_SUPPORTED AND AutomatedTesting IN_LIST LY_PROJECTS)
    ly_add_pytest(
        NAME AtomRenderer::HydraTestsMain
        TEST_REQUIRES gpu
        TEST_SUITE main
        PATH ${CMAKE_CURRENT_LIST_DIR}/test_Atom_MainSuite.py
        TEST_SERIAL
        TIMEOUT 1200
        RUNTIME_DEPENDENCIES
            AssetProcessor
            AtomTest.Assets
            Editor
    )
endif()
