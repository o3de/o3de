# """
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.

# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

# BaseViewer image comparison tests on windows
# """

# import Atom.image_comparison_utils as image_comparison_utils
# import test_tools.builtin.fixtures as fixtures
# import subprocess
# import pytest
# import os
# import logging
# import time
# import datetime
# from test_tools import WINDOWS
# from test_tools.shared.process_utils import kill_processes_named
# from test_tools.shared.waiter import wait_for




####################################
# Commented out due to need to shift to new LyTestTools, Python3 and new screenshot workflow
# Don't merge to Mainline
####################################




# workspace = fixtures.use_fixture(fixtures.builtin_workspace_fixture, scope='function')
# logger = logging.getLogger(__name__)

# @pytest.fixture(scope='session', autouse=True)
# def closing_ap(request):
    # """
    # Fixture to call once per all tests to teardown AP at the end
    # :param request: pytest request
    # """
    # def teardown():
        # kill_processes_named('AssetProcessor_tmp', ignore_extensions=True)
        # kill_processes_named('AssetProcessor', ignore_extensions=True)
        # kill_processes_named('AssetProcessorBatch', ignore_extensions=True)
        # kill_processes_named('AssetBuilder', ignore_extensions=True)
        # kill_processes_named('rc', ignore_extensions=True)
    # request.addfinalizer(teardown)

# @pytest.fixture()
# def screenshots_setup(request, workspace, sample, graphics_vendor, upload_results_to_s3):
    # """
    # Fixture for setting up workspace needed for screenshot comparison test
    # :param request: pytest request
    # :param workspace: pythontesttools workspace object
    # :param sample: name of BaseViwer sample
    # :return final_path: path to folder where output screenshots will be stored
    # """
    # # Creating output folder
    # tests_path = os.path.dirname(os.path.realpath(__file__))
    # dir_name = "{}_screenshot_tests_{}_{}_{}_{}".format(datetime.datetime.now().strftime("%Y-%m-%d_%H_%M_%S_%f"), sample.replace('/', ""), workspace.release.platform, workspace.release.configuration,
                                                        # graphics_vendor)
    # dir_name = dir_name.replace(":", '_')
    # final_path = os.path.join(tests_path, dir_name)
    # os.mkdir(final_path)

    # # Teardown to clean up Cache from .dds screenshots that are produced by BaseViewer.exe
    # def teardown():
        # cache = os.path.join(workspace.release.paths.dev(), "Cache", "BaseViewer", "pc", "baseviewer")
        # files = os.listdir(cache)
        # for file in files:
            # name, file_extension = os.path.splitext(file) 
            # if 'screenshot' in name and file_extension == '.dds':
                # screen_to_remove = os.path.join(cache, file)
                # logger.info('Deleting temp screenshot file {}.'.format(screen_to_remove))
                # os.remove(screen_to_remove)
        # kill_processes_named('BaseViewer', ignore_extensions=True)
        # # uploading screenshots to s3
        # if upload_results_to_s3:
            # image_comparison_utils.upload_screenshots_to_s3(final_path, dir_name)
    # request.addfinalizer(teardown)
    # return final_path


# # Commenting out debug due to ATOM-1677
# @pytest.mark.parametrize("platform,configuration,project,spec,sample", [
        # pytest.param("win_x64_vs2017", "profile", "BaseViewer", "all", "RPI/BistroBenchmark",
                     # marks=pytest.mark.skipif(not WINDOWS, reason="Only supported on Windows hosts")),
        # #pytest.param("win_x64_vs2017", "debug", "BaseViewer", "all", "RPI/BistroBenchmark",
        # #             marks=pytest.mark.skipif(not WINDOWS, reason="Only supported on Windows hosts")),
    # ])
# class TestBaseViewerScreenshots(object):
    # def test_BistroBenchmarkSample_CompareScreenshots(self, request, workspace, sample, screenshots_setup, graphics_vendor):
        # """
        # Launches BaseViewer.exe RPI/BistoBenchmark, taking screenshot and comparing on certain frames.
        # """
        # base_path = workspace.release.paths.dev()
        # # Generating frames list parameter
        # frames_list = range(1000,10000,1000)
        # frames_param = ""
        # screenshot_names = []
        # for parameter in frames_list:
            # frames_param += '{},'.format(parameter)
            # screenshot_names.append('screenshot_bistro_{}.dds'.format(parameter))
        # frames_param = frames_param[:-1]
        # # Loading BaseViewer
        # self.load_baseviewer_directly(workspace, sample, frames_param, timeout=100)

        # logger.info('Comparing screenshots to golden images')
        # taken_screens_path = os.path.join(base_path, "Cache", "BaseViewer", "pc", "baseviewer")
        # golden_screens_path = os.path.join(base_path, "Tests", "Atom", "GoldenImages", "Windows", graphics_vendor, "Baseviewer", "BistroBenchmark")
        # failed_screenshots = []
        # for screen in screenshot_names:
            # taken_image = os.path.join(taken_screens_path, screen)
            # golden_image = os.path.join(golden_screens_path, screen)
            # if not image_comparison_utils.compare_screenshot_to_golden_image(taken_image, golden_image, screenshots_setup):
                # failed_screenshots.append(screen)
        # if len(failed_screenshots) > 0:
            # assert False, "A failure has been found during image comparison for the following images: {}".format(failed_screenshots)


    # def load_baseviewer_directly(self, workspace, sample, frames, timeout):
        # """
        # Launch directly Baseviewer without using the launcher (since Atom is not yet integrated in Lumberyard)
        # :param workspace: pythontesttools workspace object
        # :param sample: name of the sample from BaseViewer
        # :param frames: list of frames to take screenshots at
        # :param timeout: time in seconds to wait BaseViewer to take screenshots
        # """
        # base_path = workspace.release.paths.dev()
        # bin_dir = workspace.release.paths.bin_dir()
        # cmd_path = os.path.join(base_path, bin_dir)
        # os.chdir(cmd_path)
        # p = subprocess.Popen(['BaseViewer.exe', '-sample', sample, '-screenshot', frames])
        # # Wait for BaseViewer to run and take screenshot
        # last_screenshot = frames.split(',')[-1]
        # screenshot_file = os.path.join(base_path, "Cache", "BaseViewer", "pc", "baseviewer", "screenshot_bistro_{}.dds".format(last_screenshot))
        # wait_for(lambda: os.path.exists(screenshot_file), timeout)

