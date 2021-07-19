"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Sandbox suite tests for the Atom renderer.
"""

import pytest


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("level", ["auto_test"])
class TestAtomEditorComponentsSandbox(object):

    # It requires at least one test
    def test_Dummy(self, request, editor, level, workspace, project, launcher_platform):
        pass
