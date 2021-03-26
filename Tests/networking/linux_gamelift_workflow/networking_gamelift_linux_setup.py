"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

PRE-REQUISITES:
-machine is configured with AWS credentials (run aws configure)
-MultiplayerSample is built for the given platforms
-Dedicated servers are also built for the respective platforms
-Have LYTestTools installed - run in dev 'lmbr_test pysetup install ly_test_tools'

Part 1 of 3 for Linux Gamelift Automation:
Prepares a Linux Gamelift package and uploads it to S3
"""
import os
import pytest
import logging
import configparser
pytest.importorskip('boto3')
import boto3

pytest.importorskip("ly_test_tools")
# ly_test_tools dependencies
import ly_test_tools.builtin.helpers as helpers

# test level dependencies
from ...ly_shared import s3_utils, gamelift_utils, file_utils
from ..test_lib import networking_constants

logger = logging.getLogger(__name__)
AWS_PROFILE = networking_constants.DEFAULT_PROFILE
REGION = networking_constants.DEFAULT_REGION
CONFIG_SECTION = networking_constants.LINUX_GAMELIFT_WORKFLOW_CONFIG_SECTION
CONFIG_FILENAME = networking_constants.LINUX_GAMELIFT_WORKFLOW_CONFIG_FILENAME
CONFIG_TARFILE_KEY = networking_constants.LINUX_GAMELIFT_WORKFLOW_CONFIG_TARFILE_KEY
CONFIG_DIR = os.path.dirname(__file__)  # assumes CONFIG_FILENAME is a sibling to this file
CONFIG_FILE = os.path.join(CONFIG_DIR, CONFIG_FILENAME)
BOTO3_SESSION = boto3.session.Session(profile_name=AWS_PROFILE, region_name=REGION)
S3_UTIL = s3_utils.S3Utils(BOTO3_SESSION)


@pytest.mark.parametrize('platform', ['win_x64_vs2017'])
@pytest.mark.parametrize('configuration', ['profile'])
@pytest.mark.parametrize('project', ['MultiplayerSample'])
@pytest.mark.parametrize('spec', ['all'])
class TestGameliftLinuxSetup(object):
    """
    This test will create a Linux Gamelift package and upload it to s3
    """
    def test_Gamelift_PrepareLinuxPackage_Workflow(self, request, workspace):
        def teardown():
            # Delete the locally generated tar file
            if os.path.exists(tar_filepath):
                os.remove(tar_filepath)

        # Required to use custom teardown()
        request.addfinalizer(teardown)

        # Run paks script
        gamelift_utils.GameliftUtils.run_multiplayer_sample_paks_pc_dedicated(workspace.paths.dev())

        # Run linux packer script
        tar_filepath = gamelift_utils.GameliftUtils.run_multiplayer_sample_linux_packer(workspace.paths.dev())
        tar_filename = os.path.basename(tar_filepath)

        # Upload to s3
        S3_UTIL.upload_to_bucket(networking_constants.LINUX_TAR_BUCKET, tar_filepath)

        # Write filename to .ini file to be utilized in next script
        file_utils.clear_out_file(CONFIG_FILE)
        config = configparser.ConfigParser()
        config[CONFIG_SECTION] = {}
        config[CONFIG_SECTION][CONFIG_TARFILE_KEY] = tar_filename

        with open(CONFIG_FILE, 'w') as config_to_write:
            config.write(config_to_write)
