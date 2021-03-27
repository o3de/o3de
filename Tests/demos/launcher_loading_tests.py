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
import shutil
import subprocess
from ly_test_tools.builtin.helpers import *
import tempfile
import time
# The following imports are used to detect capabilities in the current system

from ly_test_tools.environment.process_utils import *
import Tests.shared.asset_processor_utils as aputil

# Built-in fixture: provides a ready to use workspace.
from Tests.shared import substring
from ly_test_tools.environment.waiter import wait_for

LAUNCHER_TIMEOUT = 120


@pytest.mark.system
class TestProjectLauncher:
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, launcher):
        path_to_cache = workspace.paths.asset_cache()
        # Remove previous artifacts from cache
        if os.path.isdir(os.path.join(path_to_cache, "user")):
            shutil.rmtree(os.path.join(path_to_cache, "user"))

        user_cfg_path = os.path.join(workspace.paths.dev(), "user.cfg")
        cfg_staging_path = None

        # Backup user.cfg if one exists
        if os.path.exists(user_cfg_path):
            cfg_staging_path = os.path.join(tempfile.gettempdir(), "user.cfg")
            shutil.move(user_cfg_path, cfg_staging_path)

        # Configure the headless client
        with open(user_cfg_path, "w") as user_cfg:
            user_cfg.write("r_driver=NULL\n")
            user_cfg.write("sys_audio_disable=1\n")
            user_cfg.write("sys_skip_input=1\n")

        def teardown():
            launcher.kill()

            aputil.kill_asset_processor()

            # Restore previous user.cfg if one existed and unconfigure the headless client
            if cfg_staging_path:
                shutil.move(os.path.join(tempfile.gettempdir(), "user.cfg"), user_cfg_path)
            elif os.path.exists(user_cfg_path):
                os.remove(user_cfg_path)

            # save logs,screenshots
            if os.path.exists(workspace.paths.project_log()):
                workspace.artifact_manager.save_artifact(workspace.paths.project_log())
            if os.path.isdir(workspace.paths.project_screenshots()):
                workspace.artifact_manager.save_artifact(workspace.paths.project_screenshots())

        request.addfinalizer(teardown)

    @pytest.mark.parametrize('level', ['Samples/Fur_Technical_Sample', 'Samples/Advanced_RinLocomotion', 'UI/UiFeatures','Samples/Simple_JackLocomotion',
                                       'Samples/ScriptedEntityTweenerSample/SampleFullscreenAnimation', 'UI/UiMainMenuLuaSample', 'Samples/Metastream_Sample',
                                       'Samples/ScriptCanvas_Sample/ScriptCanvas_Basic_Sample', 'UI/UiIn3DWorld', 'Samples/Audio_Sample'])
    @pytest.mark.test_case_id('C1698289,C1698287,C1698294,C1698280,C1698295,C1698293,C1698289,C1698290,C1698292,C1698288')
    @pytest.mark.parametrize('platform', ['win_x64_vs2017', 'win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['profile'])
    @pytest.mark.parametrize('project', ['SamplesProject'])
    @pytest.mark.parametrize('spec', ['all'])
    def test_LaunchAndWait_LoadsLevelAndQuit_NoCrash(self, workspace, level, launcher):
        """
        Launch the Project Launcher and sets the specified level.
        Performs the console steps by passing in args.
        Loads the specified level and quit's the launcher. 
        """
        # Fast fail if the level doesn't exist
        assert os.path.exists(os.path.join(workspace.paths.project(), "Levels", level)) and os.path.isdir(
            os.path.join(workspace.paths.project(), "Levels", level)), "Level Doesn't Exist"

        launcher.args = ["+map", level]
        launcher.launch()

        pattern_string = "Loading level " + level
        test = os.path.join(workspace.paths.project_log(), "Game.log")
        wait_for(lambda: os.path.exists(os.path.join(workspace.paths.project_log(), "Game.log")))
        wait_for(lambda: substring.in_file(
            os.path.join(workspace.paths.project_log(), "Game.log"), pattern_string), LAUNCHER_TIMEOUT)
        assert not os.path.exists(
            os.path.join(workspace.paths.project_log(), "error.log")), "Launcher Crashed Unexpectedly"

        # This is a convenient place to also verify LY-90255 for free here 
        # (as well as any other text that must appear in the log).
        # writing a separate test to launch the launcher and examine the log would just waste time as the 
        # above test already launches the launcher, and waits for it to finish anyway.

        assert substring.in_file(os.path.join(workspace.paths.project_log(), "Game.log"), "Initializing CryFont done, MemUsage")



    @pytest.mark.parametrize('level',['UI/UiFeatures','Samples/Metastream_Sample'])
    @pytest.mark.parametrize('platform', ['win_x64_vs2017', 'win_x64_vs2019'])
    @pytest.mark.parametrize('configuration', ['profile'])
    @pytest.mark.parametrize('project', ['SamplesProject'])
    @pytest.mark.parametrize('spec', ['all'])
    def test_LaunchAndWait_LoadsLevelFromPakAndQuit_NoCrash(self, workspace, level, launcher):
        """
        This test ensure that the Launcher can load levels from inside paks
        """

        if workspace.platform == 'win_x64_vs2017':
            binDirPath = os.path.join(workspace.paths._dev_path, 'Bin64vc141')
        if workspace.platform == 'win_x64_vs2019':
            binDirPath = os.path.join(workspace.paths._dev_path, 'Bin64vc142')

        # Launch APBatch so that it can process all assets and quit
        subprocess.check_call(
            [os.path.join(binDirPath, 'AssetProcessorBatch'), "/gamefolder=SamplesProject"])

        lowercaseProjectName = workspace.project.lower()
        cacheLevelDir = os.path.join(workspace.paths.platform_cache(), lowercaseProjectName, "levels")
        cacheTempLevelDir = os.path.join(workspace.paths.platform_cache(), lowercaseProjectName, "templevels")

        # Rename levels dir so that runtime cannot load levels from it
        os.rename(cacheLevelDir, cacheTempLevelDir)
        # Make an empty levels folder
        os.mkdir(cacheLevelDir)

        # make an archive of all the levels
        outputLevelsArchive = os.path.join(cacheLevelDir, "templevels")
        shutil.make_archive(outputLevelsArchive, 'zip', cacheTempLevelDir)
        # make archive will make a zip file
        outputLevelsArchive = outputLevelsArchive + ".zip"

        # change extension from zip to pak
        baseFileName = os.path.splitext(outputLevelsArchive)[0]
        os.rename(outputLevelsArchive, baseFileName + ".pak")

        # Launching AP again for SamplesProject gameproject
        subprocess.Popen([os.path.join(binDirPath, 'AssetProcessor'), "/gamefolder=SamplesProject", "--zeroAnalysisMode"])

        # Waiting to give AP time some time to start the listening thread otherwise the launcher will try to launch another instance of AP
        time.sleep(1)

        # ensure that in the cache level does not exist on disk
        assert  not os.path.exists(os.path.join(cacheLevelDir, level))

        # load the levels
        launcher.args = ["+map", level]
        launcher.launch()

        pattern_string = "Loading level " + level
        wait_for(lambda: os.path.exists(os.path.join(workspace.paths.project_log(), "Game.log")))
        wait_for(lambda: substring.in_file(
            os.path.join(workspace.paths.project_log(), "Game.log"), pattern_string), LAUNCHER_TIMEOUT)
        assert not os.path.exists(
            os.path.join(workspace.paths.project_log(), "error.log")), "Launcher Crashed Unexpectedly"

        loaded_level_pattern = "Level " + level + " loaded"
        wait_for(lambda: substring.in_file(
            os.path.join(workspace.paths.project_log(), "Game.log"), loaded_level_pattern), LAUNCHER_TIMEOUT)

        launcher.stop()

        aputil.kill_asset_processor()

        shutil.rmtree(cacheLevelDir)
        os.rename(cacheTempLevelDir, cacheLevelDir)



