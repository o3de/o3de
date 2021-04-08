"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

A simple sanity test.
"""
import os
import pytest

import ly_test_tools.builtin.helpers


class TestSanity:

    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, legacy_workspace):
        def teardown():
            # Per-test cleanup goes here
            pass
        request.addfinalizer(teardown)  # adds teardown to pytest important to hook before setup in case setup fails

        # Per-test setup goes here

        #    builtin_empty_workspace_fixture doesn't configure lumberyard to run, it leaves all the configuration to
        #    the test.
        #    To run lumberayrd you need to at least setup 3rdParty and run setup assistant using the following lines.
        #    Alternatively, you can use 'builtin_workspace_fixture'.
        #    This sanity test only checks if the framework is sane, so there is no need to configure LY.
        # workspace.run_waf_configure()
        # workspace.setup_assistant.enable_default_capabilities()

    @pytest.mark.bvt
    # Example of dynamically parametrized test, these parameters are consumed by the workspace fixture:
    # TODO LY-109331 @pytest.mark.parametrize("platform", ["win_x64_vs2017", "win_x64_vs2019", "darwin_x64"])
    @pytest.mark.parametrize("platform", ["win_x64_vs2017", "win_x64_vs2019"])
    @pytest.mark.parametrize("configuration", ["profile"])
    @pytest.mark.parametrize("project", ["AutomatedTesting"])
    @pytest.mark.parametrize("spec", ["all"])
    def test_Paths_DevPathExists_PathItsADirectory(self, legacy_workspace, platform, configuration, project, spec):
        # type: (WorkspaceManager, str, str, str, str) -> None

        # Test code goes here
        # os.makedirs(workspace.paths.dev())

        # These asserts verify that the parameters were correctly received by the workspace fixture
        assert legacy_workspace.platform == platform, "Platform does not match parameters"
        assert legacy_workspace.configuration == configuration, "Configuration does not match parameters"
        assert legacy_workspace.project == project, "Project does not match parameters"
        assert legacy_workspace.spec == spec, "Spec does not match parameters"

        # Verify that the dev folder exists
        assert os.path.isdir(legacy_workspace.paths.dev())
