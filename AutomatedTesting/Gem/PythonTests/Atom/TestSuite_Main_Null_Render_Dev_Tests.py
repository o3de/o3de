"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os
import pytest

from ly_test_tools.o3de.editor_test import EditorBatchedTest, EditorTestSuite

logger = logging.getLogger(__name__)
TEST_DIRECTORY = os.path.join(os.path.dirname(__file__), "tests")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestDevAutomation(EditorTestSuite):

    class ShaderAssetBuilder_RecompilesShaderAsChainOfDependenciesChanges(EditorBatchedTest):
        from Atom.tests import hydra_ShaderAssetBuilder_RecompilesShaderAsChainOfDependenciesChanges as test_module
