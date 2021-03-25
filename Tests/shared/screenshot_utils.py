"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import os
import botocore.exceptions
import string
import boto3

from test_tools.shared.file_utils import move_file

import test_tools.launchers.phase as phase
import shared.s3_utils as s3_utils
from test_tools.shared.remote_console_commands import get_screenshot_command
from test_tools.shared.waiter import wait_for
from test_tools import HOST_PLATFORM
from test_tools.shared.images.qssim import qssim as compare_screenshots
from test_tools.shared.launcher_testlib import retry_console_command


def take_screenshot(remote_console_instance, launcher, screenshot_name):
    """
    Takes an in game screenshot using the remote console instance passed in, validates that the screenshot exists
    and then renames that screenshot to something defined by the user of this function.
    :param remote_console_instance: Remote console instance that is attached to a specific launcher instance
    :param launcher: Launcher instance so we can use the file exists functionality provided by test_tools
    :param screenshot_name: Name of the screenshot
    :return: None
    """
    get_screenshot_command(remote_console_instance)
    screenshot_path = os.path.join(launcher.workspace.release.paths.platform_cache(), 'user', 'screenshots')
    launcher.run(phase.FileExistsPhase(os.path.join(screenshot_path, 'screenshot0000.jpg')))
    wait_for(lambda: rename_screenshot(screenshot_path, screenshot_name),
             timeout=120,
             exc=AssertionError('Screenshot at path:{} and with name:{} is still in use.'.format(screenshot_path, screenshot_name)))


def rename_screenshot(screenshot_path, screenshot_name):
    """
    Tries to rename the screenshot when the file is done being written to
    :param screenshot_path: Path to the Screenshot folder
    :param screenshot_name: Name we wish to change the screenshot to
    :return: True when operation is completed, False if the file is still in use
    """
    try:
        print 'Trying to rename {} to {}.'.format(os.path.join(screenshot_path, 'screenshot0000.jpg'), os.path.join(screenshot_path, '{}.jpg'.format(screenshot_name)))
        os.rename(os.path.join(screenshot_path, 'screenshot0000.jpg'),
                  os.path.join(screenshot_path, '{}.jpg'.format(screenshot_name)))
        return True
    except OSError as e:
        print ('Found error {0} when trying to rename screenshot. {1}'.format(str(e), str(e.message)))
        return False


def move_screenshots(screenshot_path, file_type, logs_path):
    """
    Moves screenshots of a specific file type to the flume location so we can gather all of the screenshots we took.
    :param screenshot_path: Path to the screenshot folder
    :param file_type: Types of Files to look for.  IE .jpg, .tif, etc
    :param logs_path: Path where flume gathers logs to be upload
    """
    for file_name in os.listdir(screenshot_path):
        if file_name.endswith(file_type):
            move_file(screenshot_path, logs_path, file_name)


def screenshot_command(remote_console_instance, command_to_run, expected_log_line):
    """
    This is just a helper function to help send and validate against screenshot console commands.
    :param remote_console_instance: Remote console instance
    :param command_to_run: The Screenshot command that you wish to run
    :param expected_log_line:  The console log line to expect in order to set the event to true
    :return:
    """
    return retry_console_command(remote_console_instance, command_to_run, expected_log_line)


def get_screenshot_command_with_retries(remote_console_instance):
    """
    Used for an in-game screenshot
    :param remote_console_instance: Remote console instance
    :return: None
    """
    if (HOST_PLATFORM == 'win_x64'):
        command = 'Screenshot: @user@\screenshots/'
    else:
        command = 'Screenshot: @user@/screenshots/'

    wait_for(lambda: screenshot_command(remote_console_instance, 'r_GetScreenShot 1', command), timeout=240,
             exc=AssertionError('Screenshot command failed'))


def take_screenshot_with_retries(remote_console_instance, launcher, screenshot_name):
    """
    Takes an in game screenshot using the remote console instance passed in, validates that the screenshot exists
    and then renames that screenshot to something defined by the user of this function.
    :param remote_console_instance: Remote console instance that is attached to a specific launcher instance
    :param launcher: Launcher instance so we can use the file exists functionality provided by test_tools
    :param screenshot_name: Name of the screenshot
    :return: None
    """
    get_screenshot_command_with_retries(remote_console_instance)
    screenshot_path = os.path.join(launcher.workspace.release.paths.platform_cache(), 'user', 'screenshots')
    launcher.run(phase.FileExistsPhase(os.path.join(screenshot_path, 'screenshot0000.jpg')))
    wait_for(lambda: rename_screenshot(screenshot_path, screenshot_name),
             timeout=120,
             exc=AssertionError('Screenshot taken is still in use'))


def compare_golden_image(similarity_threshold, screenshot, screenshot_path, golden_image_name,
                         golden_image_path=None):
    """
    This function assumes that your golden image filename contains the same base screenshot name and the word "golden"
    ex. pc_gamelobby_golden

    :param similarity_threshold: A float from 0.0 - 1.0 that determines how similar images must be or an asserts
    :param screenshot: A string that is the full name of the screenshot (ex. 'gamelobby_host.jpg')
    :param screenshot_path: A string that contains the path to the screenshots
    :param golden_image_path: A string that contains the path to the golden images, defaults to the screenshot_path
    :return:
    """
    if golden_image_path is None:
        golden_image_path = screenshot_path

    mean_similarity = compare_screenshots('{}\{}'.format(screenshot_path, screenshot),
                                          '{}\{}'.format(golden_image_path, golden_image_name))
    assert mean_similarity > similarity_threshold, \
        '{} screenshot comparison failed! Mean similarity value is: {}'\
        .format(screenshot, mean_similarity)


def take_screenshot_and_compare(remote_console, launcher, screenshot, similarity_threshold,
                                golden_image_name, screenshot_path=None, file_type='.jpg'):
    """
    Takes a screenshot and compares it with its golden image. This utilizes the take_screenshot_with_retries function
    which is only used for PC. There are some assumptions with the golden image naming convention that is explained in
    the compare_golden_image function. This also assumes it is a jpg file.

    This function enforces a naming convention such that the golden image and screenshot share part of the same name.
    Also, the screenshot will be appended with the screenshot_key as shown below.

    The screenshot name will be screenshot_base + screenshot_key + filetype
        (ex. 'gamelobby_host.jpg', 'MultiplayerSampleClient_1.png')

    :param screenshot_base: A string that is the base name of the screenshot (ex. 'gamelobby')
    :param screenshot_key: A string that acts as a key identifier for the screenshot.
    :param similarity_threshold: A float from 0.0 - 1.0 that determines how similar images must be or it asserts
    :param screenshot_path: A string for the screenshot path. Defaults to user/screenshots
    :param file_type: A string for the screenshot filetype. Defaults to '.jpg'
    :return:
    """
    if screenshot_path is None:
        screenshot_path = r'{}\user\screenshots'.format(launcher.workspace.release.paths.platform_cache())

    take_screenshot_with_retries(remote_console, launcher, screenshot)
    compare_golden_image(similarity_threshold, '{}{}'.format(screenshot, file_type),
                         screenshot_path, golden_image_name)


def download_qa_golden_images(project_name, destination_dir, platform):
    """
    Downloads the golden images for a specified project from s3. The project_name, platform, and filetype are used to
    filter which images will be downloaded as the golden images.

    https://s3.console.aws.amazon.com/s3/buckets/ly-qae-jenkins-configs/golden-images/?region=us-west-1&tab=overview

    :param project_name: a string of the project name of the folder in s3. ex: 'MultiplayerSample'
    :param destination_dir: a string of where the images will be downloaded to
    :param platform: a string for the platform type ('pc', 'android', 'ios', 'darwin')
    :param filetype: a string for the file type. ex: '.jpg', '.png'
    :return:
    """
    bucket_name = 'ly-qae-jenkins-configs'
    path = 'golden-images/{}/{}/'.format(project_name, platform)

    if not s3_utils.key_exists_in_bucket(bucket_name, path):
        raise s3_utils.KeyDoesNotExistError("Key '{}' does not exist in S3 bucket {}".format(path, bucket_name))
    for image in s3_utils.s3.Bucket(bucket_name).objects.filter(Prefix=path):
        file_name = string.replace(image.key, path, '')
        if file_name != '':
            s3_utils.download_from_bucket(bucket_name, image.key, destination_dir, file_name)


def prepare_for_screenshot_compare(remote_console_instance):
    """
    Prepares launcher for screenshot comparison. Removes any debug text and antialiasing that may result in interference
    with the comparison.

    :param remote_console_instance: Remote console instance that is attached to a specific launcher instance
    :return:
    """
    wait_for(lambda: retry_console_command(remote_console_instance, 'r_displayinfo 0',
                                           '$3r_DisplayInfo = $60 $5[DUMPTODISK, RESTRICTEDMODE]$4'), timeout=120)
    wait_for(lambda: retry_console_command(remote_console_instance, 'r_antialiasingmode 0',
                                           '$3r_AntialiasingMode = $60 $5[]$4'), timeout=120)

