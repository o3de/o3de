"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

"""
import pytest
import ly_test_tools

import os
import sys
import shutil
import subprocess
import glob
from ly_test_tools.builtin.helpers import *
from ly_test_tools.environment.process_utils import *
from ly_test_tools.lumberyard.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

logger = logging.getLogger(__name__)

project_list = ['AutomatedTesting']

@pytest.mark.system
@pytest.mark.SUITE_periodic
class TestAuxiliaryContent:
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, project):
        path_to_dev = workspace.paths.dev()
        os.chdir(path_to_dev)
        auxiliaryContentDirName = str.lower(project) + f"_{ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]}_paks"
        self.auxiliaryContentPath = os.path.join(path_to_dev, auxiliaryContentDirName)

        def teardown():
            if (os.path.exists(self.auxiliaryContentPath)):
                shutil.rmtree(self.auxiliaryContentPath)

        request.addfinalizer(teardown)

    @staticmethod
    def scanForLevelPak(path_toscan):
        files = glob.glob('{0}/**/level.pak'.format(path_toscan), recursive=True)
        return len(files)

    @pytest.mark.parametrize('level', ['alldependencies'])
    @pytest.mark.parametrize('project', project_list)
    def test_CreateAuxiliaryContent_DontSkipLevelPaks(self, workspace, level):
        """
        This test ensure that Auxiliary Content contain level.pak files
        """

        path_to_dev = workspace.paths.dev()
        bin_path = workspace.paths.build_directory()

        auxiliaryContentScriptPath = os.path.join(path_to_dev, 'BuildReleaseAuxiliaryContent.py')
        subprocess.check_call(['python', auxiliaryContentScriptPath,
                               "--buildFolder={0}".format(bin_path),
                               "--platforms=pc",
                               f"--project={workspace.project}"])

        assert os.path.exists(self.auxiliaryContentPath)
        assert not self.scanForLevelPak(self.auxiliaryContentPath) == 0

    @pytest.mark.parametrize('level', ['alldependencies'])
    @pytest.mark.parametrize('project', project_list)
    def test_CreateAuxiliaryContent_SkipLevelPaks(self, workspace, level):
        """
        This test ensure that Auxiliary Content contain no level.pak file
        """

        path_to_dev = workspace.paths.dev()
        bin_path = workspace.paths.build_directory()

        auxiliaryContentScriptPath = os.path.join(path_to_dev, 'BuildReleaseAuxiliaryContent.py')
        subprocess.check_call(['python', auxiliaryContentScriptPath,
                               "--buildFolder={0}".format(bin_path),
                               "--platforms=pc",
                               "--skiplevelPaks",
                               f"--project={workspace.project}"])
        assert os.path.exists(self.auxiliaryContentPath)

        assert self.scanForLevelPak(self.auxiliaryContentPath) == 0
