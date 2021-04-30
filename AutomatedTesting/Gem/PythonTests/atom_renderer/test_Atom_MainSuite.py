"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Main suite tests for the Atom renderer.
"""
import logging
import os

import pytest

import editor_python_test_tools.hydra_test_utils as hydra

logger = logging.getLogger(__name__)
EDITOR_TIMEOUT = 120
TEST_DIRECTORY = os.path.join(os.path.dirname(__file__), "atom_hydra_scripts")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("level", ["auto_test"])
class TestAtomEditorComponentsMain(object):

    # It requires at least one test
    def test_Dummy(self, request, editor, level, workspace, project, launcher_platform):
        pass
