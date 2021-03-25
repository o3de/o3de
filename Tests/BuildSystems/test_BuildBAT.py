"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

BuildSystems BAT to automate building on packages for Windows
"""
import logging
import pytest
import os

pytest.importorskip('ly_test_tools')

import ly_test_tools.builtin.helpers as helpers
from .test_lib import build_helper

logger = logging.getLogger(__name__)


@pytest.mark.BAT
@pytest.mark.parametrize('spec', ['all'])
@pytest.mark.parametrize('project', ['SamplesProject'])

class TestWindowsBuildConfig(object):
    """
    Automated tests for all the build configurations for Windows.
    Test cases live in Repository/Build System/Lumberyard Builds/Configurations
    """
    @pytest.mark.test_case_id('C15723869')
    @pytest.mark.parametrize('platform', ['win_x64_vs2017'])
    @pytest.mark.parametrize('configuration', ['profile'])
    @pytest.mark.build
    def test_build_win_x64_vs2017_profile(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15723870')
    @pytest.mark.parametrize('platform', ['win_x64_vs2017'])
    @pytest.mark.parametrize('configuration', ['profile_dedicated'])
    @pytest.mark.build
    def test_build_win_x64_vs2017_profile_dedicated(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15723871')
    @pytest.mark.parametrize('platform', ['win_x64_vs2017'])
    @pytest.mark.parametrize('configuration', ['profile_test'])
    @pytest.mark.build
    def test_build_win_x64_vs2017_profile_test(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15723872')
    @pytest.mark.parametrize('platform', ['win_x64_vs2017'])
    @pytest.mark.parametrize('configuration', ['profile_test_dedicated'])
    @pytest.mark.build
    def test_build_win_x64_vs2017_profile_test_dedicated(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15716369')
    @pytest.mark.parametrize('platform', ['win_x64_vs2017'])
    @pytest.mark.parametrize('configuration', ['debug'])
    @pytest.mark.build
    def test_build_win_x64_vs2017_debug(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15716370')
    @pytest.mark.parametrize('platform', ['win_x64_vs2017'])
    @pytest.mark.parametrize('configuration', ['debug_dedicated'])
    @pytest.mark.build
    def test_build_win_x64_vs2017_debug_dedicated(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15716371')
    @pytest.mark.parametrize('platform', ['win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['debug_test'])
    @pytest.mark.build
    def test_build_win_x64_vs2019_debug_test(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15716372')
    @pytest.mark.parametrize('platform', ['win_x64_vs2017'])
    @pytest.mark.parametrize('configuration', ['debug_test_dedicated'])
    @pytest.mark.build
    def test_build_win_x64_vs2017_debug_test_dedicated(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15723889')
    @pytest.mark.parametrize('platform', ['win_x64_vs2017'])
    @pytest.mark.parametrize('configuration', ['release'])
    @pytest.mark.build
    def test_build_win_x64_vs2017_release(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15723890')
    @pytest.mark.parametrize('platform', ['win_x64_vs2017'])
    @pytest.mark.parametrize('configuration', ['release_dedicated'])
    @pytest.mark.build
    def test_build_win_x64_vs2017_release_dedicated(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15815180')
    @pytest.mark.parametrize('platform', ['win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['profile'])
    @pytest.mark.build
    def test_build_win_x64_vs2019_profile(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15815181')
    @pytest.mark.parametrize('platform', ['win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['profile_dedicated'])
    @pytest.mark.build
    def test_build_win_x64_vs2019_profile_dedicated(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15815182')
    @pytest.mark.parametrize('platform', ['win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['profile_test'])
    @pytest.mark.build
    def test_build_win_x64_vs2019_profile_test(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15815183')
    @pytest.mark.parametrize('platform', ['win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['profile_test_dedicated'])
    @pytest.mark.build
    def test_build_win_x64_vs2019_profile_test_dedicated(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15815174')
    @pytest.mark.parametrize('platform', ['win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['debug'])
    @pytest.mark.build
    def test_build_win_x64_vs2019_debug(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15815175')
    @pytest.mark.parametrize('platform', ['win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['debug_dedicated'])
    @pytest.mark.build
    def test_build_win_x64_vs2019_debug_dedicated(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15815176')
    @pytest.mark.parametrize('platform', ['win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['debug_test'])
    @pytest.mark.build
    def test_build_win_x64_vs2019_debug_test(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15815177')
    @pytest.mark.parametrize('platform', ['win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['debug_test_dedicated'])
    @pytest.mark.build
    def test_build_win_x64_vs2019_debug_test_dedicated(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15815190')
    @pytest.mark.parametrize('platform', ['win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['release'])
    @pytest.mark.build
    def test_build_win_x64_vs2019_release(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)

    @pytest.mark.test_case_id('C15815191')
    @pytest.mark.parametrize('platform', ['win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['release_dedicated'])
    @pytest.mark.build
    def test_build_win_x64_vs2019_release_dedicated(self, workspace, platform, configuration, project, spec):
        workspace.build()
        build_log = os.path.join(workspace.artifact_manager.dest_path, 'waf_build.log')
        assert build_helper.verify_build_log(build_log, platform, configuration)