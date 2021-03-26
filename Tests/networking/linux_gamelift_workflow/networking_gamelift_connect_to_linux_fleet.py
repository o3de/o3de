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
-AWS default output format is set to 'json'
-MultiplayerSample is built for the given platforms
-Dedicated servers are also built for the respective platforms
-Have LYTestTools installed - run in dev 'lmbr_test pysetup install ly_test_tools'
-A GameLift fleet is ready with its fleet id provided in the config.ini file

Part 3 of 3 for Linux GameLift Automation:
Launches 4 MultiplayerSample clients and connects to a given GameLift fleet
"""
import os
import pytest
import configparser
import logging
import shutil
pytest.importorskip('dateutil')
import boto3

pytest.importorskip("ly_test_tools")
# ly_test_tools dependencies
import ly_test_tools.builtin.helpers as helpers
import ly_test_tools.environment.waiter as waiter
import ly_test_tools.launchers.launcher_helper as launcher_helper
import ly_remote_console.remote_console_commands as remote_console

# test level dependencies
from ...ly_shared import file_utils, network_utils
from ...ly_shared.gamelift_utils import GameliftUtils
from ..test_lib import networking_constants

logger = logging.getLogger(__name__)
INTERVAL_TIME = 10  # in seconds, longer interval times to reduce api call spam
TIMEOUT = 30  # in seconds, time to wait for asyncronous api calls
CONFIG_SECTION = networking_constants.LINUX_GAMELIFT_WORKFLOW_CONFIG_SECTION
CONFIG_FILENAME = networking_constants.LINUX_GAMELIFT_WORKFLOW_CONFIG_FILENAME
CONFIG_FLEET_KEY = networking_constants.LINUX_GAMELIFT_WORKFLOW_CONFIG_FLEET_KEY
CONFIG_DIR = os.path.dirname(__file__)  # assumes CONFIG_FILENAME is a sibling to this file
CONFIG_FILE = os.path.join(CONFIG_DIR, CONFIG_FILENAME)
AWS_PROFILE = networking_constants.DEFAULT_PROFILE
REGION = networking_constants.DEFAULT_REGION
BOTO3_SESSION = boto3.session.Session(profile_name=AWS_PROFILE, region_name=REGION)
GAMELIFTUTILS = GameliftUtils(BOTO3_SESSION)
REMOTE_CONSOLE_PORT = 4600
LAUNCHER_PORT_DICTS = []



@pytest.fixture
def multiplayer_client_launcher_list(request, workspace, level, number_of_launchers):
    # type: (ly_test_tools.managers.workspace.AbstractWorkspaceManager, str, int) -> test_tools.launchers.platforms.base.Launcher
    """
    Create a list of simple launchers. Returns a list of dicts that contain the launcher and its remote console port.
    ex.
    for launcher_dict in launcher_list:
        game_launcher = launcher_dict['launcher']
        game_port = launcher_dict['port']
    :param request: Pytest request object
    :param workspace: The workspace where the launcher is located
    :param level: The level name to load
    :param number_of_launchers: The number of launcher instances

    :return: A fully configured list of dicts containing launchers and their respective port
    """
    launcher_list = []

    def teardown():
        for launcher_dict in launcher_list:
            if launcher_dict['launcher'].is_alive():
                launcher_dict['launcher'].stop()
            if launcher_dict['remote_console'].connected:
                launcher_dict['remote_console'].stop()
    request.addfinalizer(teardown)

    # Create a list of launchers
    for index in range(0, number_of_launchers):
        port = REMOTE_CONSOLE_PORT + index
        launcher_list.append({'launcher': launcher_helper.create_launcher(workspace),
                              'rc_port': port,
                              'remote_console': remote_console.RemoteConsole(port=port)})

    return launcher_list


@pytest.mark.parametrize('platform', ['win_x64_vs2017'])
@pytest.mark.parametrize('configuration', ['profile'])
@pytest.mark.parametrize('project', ['MultiplayerSample'])
@pytest.mark.parametrize('spec', ['all'])
@pytest.mark.parametrize('level', ['multiplayersample'])
@pytest.mark.parametrize('number_of_launchers', [4])
class TestRunWindowsGameliftOnLinuxFleet(object):
    """
    This test will connect 4 MultiplayerSample clients to a given fleet.
    """
    def test_Gamelift_WindowsClientsConnectLinuxFleet_Workflow(self, request, multiplayer_client_launcher_list,
                                                               workspace):
        def teardown():
            # Gather all log files in cache/user/log
            for file_name in os.listdir(workspace.paths.project_log()):
                file_path = os.path.join(workspace.paths.project_log(), file_name)
                try:
                    workspace.artifact_manager.save_artifact(file_path)
                except(IOError, WindowsError):
                    logger.debug(f'A log could not be saved: {file_path}')

            # If error or crash logs exist, move them to the artifact
            file_utils.gather_error_logs(workspace)

        # Required to use custom teardown()
        request.addfinalizer(teardown)

        # Delete previous logs
        if os.path.exists(workspace.paths.project_log()):
            shutil.rmtree(workspace.paths.project_log())

        # Test setup
        current_launcher_instance = 0
        gamelift_credentials = GameliftUtils.get_aws_credentials(AWS_PROFILE)

        # Get the fleet id
        config = configparser.ConfigParser()
        config.read(CONFIG_FILE)
        try:
            fleet_id = config[CONFIG_SECTION][CONFIG_FLEET_KEY]
        except KeyError as e:
            raise AssertionError(f'Cannot find section: {CONFIG_SECTION} or fleet key: {CONFIG_FLEET_KEY} in file: '
                                 f'{CONFIG_FILE}. {e}')

        host_launcher = multiplayer_client_launcher_list[0]
        host_launcher['launcher'].args = ['+sv_port', '33435', '+gamelift_fleet_id', fleet_id,
                                          '+gamelift_aws_access_key', gamelift_credentials['access'],
                                          '+gamelift_aws_secret_key', gamelift_credentials['secret'],
                                          '+gamelift_aws_region', REGION, '+gamelift_endpoint',
                                          f'gamelift.{REGION}.amazonaws.com', '+map', 'gamelobby']

        # Process Assets
        assert workspace.asset_processor.batch_process(), 'Assets did not process correctly when calling AP Batch'

        # Normally we start a launcher using with: which utilizes context managers to ensure teardown
        # However we are using multiple launchers so we ensure launcher.stop() is called in custom teardown
        host_launcher['launcher'].start()

        # Start the remote console
        host_launcher['remote_console'].start()

        # Wait for the launcher to load
        validate_level_loaded = "SetGlobalState 13->2 'LEVEL_LOAD_COMPLETE' -> 'RUNNING'"
        host_launcher['remote_console'].expect_log_line(validate_level_loaded, TIMEOUT)

        # Checks if the remote console port is listening
        waiter.wait_for(lambda: network_utils.check_for_listening_port(host_launcher['rc_port']),
                        timeout=TIMEOUT,
                        exc=AssertionError(f'Port {host_launcher["rc_port"]} not listening.'))

        # Set expected log line to assert
        gamelift_host_event = host_launcher['remote_console'].\
            expect_log_line('(GameLift) - Initialized GameLift client successfully.', TIMEOUT)

        # Send the remote console command to host a multiplayersample game
        host_launcher['remote_console'].\
            send_command('gamelift_host AutomationTest multiplayersample 8')

        assert gamelift_host_event(), 'Console never stated gamelift hosting was completed properly'

        # Wait for the game to load
        host_launcher['remote_console'].expect_log_line(validate_level_loaded, TIMEOUT)
        host_launcher['remote_console'].stop()
        current_launcher_instance += 1

        # Assert on if our client connected
        waiter.wait_for(lambda: GAMELIFTUTILS.check_all_current_player_sessions(fleet_id, current_launcher_instance),
                        timeout=TIMEOUT, interval=INTERVAL_TIME,
                        exc=AssertionError(f"Client {current_launcher_instance} never connected to GameLift"))

        # Repeat with all other clients, except the console command is different
        for launcher_dict in multiplayer_client_launcher_list[1:]:

            launcher_dict['launcher'].args = ['+sv_port', '33435', '+gamelift_fleet_id', fleet_id,
                                              '+gamelift_aws_access_key', gamelift_credentials['access'],
                                              '+gamelift_aws_secret_key', gamelift_credentials['secret'],
                                              '+gamelift_aws_region', REGION, '+gamelift_endpoint',
                                              f'gamelift.{REGION}.amazonaws.com', '+gm_enableMetrics']

            launcher_dict['launcher'].start()
            launcher_dict['remote_console'].start()
            launcher_dict['remote_console'].expect_log_line(validate_level_loaded, TIMEOUT)
            launcher_dict['remote_console'].send_command('gamelift_join 1')
            launcher_dict['remote_console'].expect_log_line(validate_level_loaded, TIMEOUT)

            waiter.wait_for(
                lambda: GAMELIFTUTILS.check_all_current_player_sessions(fleet_id, current_launcher_instance + 1),
                timeout=TIMEOUT, interval=INTERVAL_TIME,
                exc=AssertionError(f"Client {current_launcher_instance} never connected to GameLift"))

            waiter.wait_for(lambda: network_utils.check_for_listening_port(launcher_dict['rc_port']),
                            timeout=TIMEOUT, exc=AssertionError(f'Port {launcher_dict["rc_port"]} not listening.'))

            launcher_dict['remote_console'].stop()
            current_launcher_instance += 1

        current_launcher_instance = 0
        for launcher_dict in multiplayer_client_launcher_list:
            assert launcher_dict['launcher'].is_alive(), f"Client number: {current_launcher_instance} was closed " \
                                                         f"unexpectedly"
            current_launcher_instance += 1
