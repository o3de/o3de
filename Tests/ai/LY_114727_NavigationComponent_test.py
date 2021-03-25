"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""


"""
ly102242: Runs the ly-102242 level in the Editor which reproduces the appropriate steps for 
bug ly-102242 to crash when spawning an invalid touchbending asset.  The touchbending asset 
can be made invalid by including multiple meshes - one with skinning data and one without.
NOTE:  In the bugfixed case, a crash will not occur, but touchbending will not occur and errors
will be printed to the console every time a new asset gets "touched" (spawned in the physics system).
"""
import pytest
pytest.importorskip('ly_test_tools')
import logging
import os

from ..ly_shared import hydra_lytt_test_utils as hydra_utils

logger = logging.getLogger(__name__)
test_directory = os.path.join(os.path.dirname(__file__), 'EditorScripts')
editor_timeout = 30

@pytest.mark.parametrize('platform', ['win_x64_vs2017'])
@pytest.mark.parametrize('configuration', ['profile'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('spec', ['all'])
@pytest.mark.parametrize('level', ['AI/NavigationComponentTest'])
class TestNavigationComponent(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request):
        def teardown():
            if hasattr(self, 'cfg_file_name'):
                hydra_utils.cleanup_cfg_file(self.cfg_file_name)

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)

    # entities with Transform, Physics and Custom movement methods navigate to the goal 
    def test_NavigationComponent(self, request, legacy_editor, level):

        cfg_args = [level]

        expected_lines = [
            "OnActivate NavigationAgentCustom",
            "OnActivate NavigationAgentPhysics",
            "OnActivate NavigationAgentTransform",

            "OnTraversalComplete NavigationAgentCustom",
            "OnTraversalComplete NavigationAgentPhysics",
            "OnTraversalComplete NavigationAgentTransform",
        ]

        unexpected_lines = [
            "OnTraversalCanceled NavigationAgentCustom",
            "OnTraversalCanceled NavigationAgentPhysics",
            "OnTraversalCanceled NavigationAgentTransform",
        ]

        hydra_utils.launch_and_validate_results(request, test_directory, legacy_editor,
                                                'LY_114727_NavigationComponent_MovementMethods.py',
                                                expected_lines, unexpected_lines, timeout=editor_timeout, cfg_args=cfg_args)

