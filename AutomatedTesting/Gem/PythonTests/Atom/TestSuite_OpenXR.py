"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools import HOST_OS_EDITOR
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorTestSuite


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", [HOST_OS_EDITOR])
class TestOpenXR(EditorTestSuite):
    class OpenXRAssetsAndComponents(EditorSingleTest):
        from Atom.tests import OpenXRAssetsAndComponents as test_module
