"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Main suite tests for the Shader Build Pipeline.
"""
import pytest
from ly_test_tools import LAUNCHERS
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSingleTest

@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestShaderBuildPipelineMain(EditorTestSuite):
    """Holds tests for Shader Build Pipeline validation"""

    class ShaderAssetBuilder_RecompilesShaderAsChainOfDependenciesChanges(EditorSingleTest):
        from .tests import hydra_ShaderAssetBuilder_RecompilesShaderAsChainOfDependenciesChanges as test_module