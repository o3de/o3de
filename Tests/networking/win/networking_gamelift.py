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

All of these tests will test the ability for 4 clients ability to attach to a Gamelift server instance.  We will also
check that all of the game clients are alive and connected by the end of the tests.  If an assertion happens we will
gather all of the artifacts and move them into an artifact folder which will eventually be consumed by Flume.
"""
import os
import pytest
import shutil
import time
import logging
pytest.importorskip("boto3")
import boto3

pytest.importorskip("ly_test_tools")
# ly_test_tools dependencies
import ly_test_tools.builtin.helpers as helpers
import ly_test_tools.environment.waiter as waiter
import ly_test_tools.launchers.launcher_helper as launcher_helper
import ly_remote_console.remote_console_commands

# test level dependencies
from ...ly_shared import file_utils, network_utils
from ...ly_shared.gamelift_utils import GameliftUtils
from ..test_lib import networking_constants

import uuid

logger = logging.getLogger(__name__)
INTERVAL_TIME = 10  # seconds for api calls against aws
AWS_PROFILE = networking_constants.DEFAULT_PROFILE
REGION = networking_constants.DEFAULT_REGION
BOTO3_SESSION = boto3.session.Session(profile_name=AWS_PROFILE, region_name=REGION)
FLEET_TIMEOUT = 3600
gamelift_utils = GameliftUtils(BOTO3_SESSION)

AWS_REGION = 'us-west-2'


@pytest.fixture
def remote_console_instances(request,
                             number_of_launchers    # type: int
                             ):
    """
    Creates a list of RemoteConsole objects

    :param number_of_launchers: The number of launcher instances
    :return: A list of RemoteConsole objects
    """
    # Each launcher has its own remote console
    remote_console_list = []
    for x in range(0, number_of_launchers):
        remote_console_list.append(ly_remote_console.remote_console_commands.RemoteConsole(port=4600+x))


    def teardown():
        for console in remote_console_list:
            if console.connected:
                console.stop()

    request.addfinalizer(teardown)

    return remote_console_list


@pytest.fixture
def multiplayer_client_launcher_list(
        request,
        workspace,   # type: test_tools.managers.workspace.AbstractWorkspaceManager
        level,        # type: str
        number_of_launchers,    # type: int
        ):
    # type: (...) -> test_tools.launchers.platforms.base.Launcher
    """
    Create a list of simple launchers.

    :param workspace: The workspace where the launcher is located
    :param level: The level name to load
    :param number_of_launchers: The number of launcher instances

    :return: A fully configured list of launchers
    """
    launcher_list = []

    # Create a list of launchers
    for _ in range(0, number_of_launchers):
        launcher_list.append(launcher_helper.create_launcher(workspace))

    def teardown():
        # Gather all log files in cache/user/log
        try:
            for file_name in os.listdir(workspace.paths.project_log()):
                file_path = os.path.join(workspace.paths.project_log(), file_name)
                try:
                    workspace.artifact_manager.save_artifact(file_path)
                except(IOError, WindowsError):
                    logger.info('A log could not be saved: {}'.format(file_path))
        except:
            logger.info("No log folder found for path: {}".format(workspace.paths.project_log()))

        # If error or crash logs exist, move them to the artifact
        file_utils.gather_error_logs(workspace)

        # Custom teardown of multiple launchers because we cannot use context manager
        for client in launcher_list:
            try:
                client.stop()
            except AssertionError:
                logger.info("A launcher could not be shut down!")

    request.addfinalizer(teardown)

    return launcher_list

def add_game_lift_creds_and_endpoint_args(launcher, gamelift_credentials):
    launcher.args.extend(['+gamelift_aws_access_key', gamelift_credentials['access'],
                          '+gamelift_aws_secret_key', gamelift_credentials['secret'],
                          '+gamelift_aws_region', AWS_REGION,
                          '+gamelift_endpoint', 'gamelift.{}.amazonaws.com'.format(AWS_REGION)])


def create_game_lift_player_sessions(fleet_id, gamelift_credentials, timeout,
                                     remote_console_instances, multiplayer_client_launcher_list):
    """
    Creates Gamelift player sessions. Connects the MultiplayerSample game client to game sessions using remote console
    gamelift_join command. The command supports joining first game session found in the list of game sessions from
    GameLift
    :param fleet_id: Fleet to create player sessions
    :param gamelift_credentials: Credentials to use to make GameLift calls
    :param timeout: Timeout for waiter calls
    :param remote_console_instances: Remote console instances to work with
    :param multiplayer_client_launcher_list: Client launcher instances to work with
    :return:
    """
    # First 2 instances are reserved for creating game sessions. 3 and 4 instance of the launcher and remote console instances
    # are used for player sessions
    current_launcher_instance = 2

    while current_launcher_instance < 4:
        launcher = multiplayer_client_launcher_list[current_launcher_instance]
        launcher.args = []
        add_game_lift_creds_and_endpoint_args(launcher, gamelift_credentials)
        launcher.args.extend(['+gamelift_fleet_id', fleet_id, '+gm_enableMetrics'])

        launcher.start()

        # Hard wait for the clients to initialize. Hoping 10secs is enough on the worst machines as well.
        time.sleep(10)

        join_game_session_index = current_launcher_instance % 2 + 1
        remote_console_instances[current_launcher_instance].start()
        remote_console_instances[current_launcher_instance].send_command('gamelift_join {}'
                                                                         .format(join_game_session_index))

        waiter.wait_for(lambda: network_utils.check_for_listening_port(4600 + current_launcher_instance),
                        timeout=timeout,
                        exc=AssertionError('Port 460{} not listening.'.format(current_launcher_instance)))

        remote_console_instances[current_launcher_instance].stop()
        current_launcher_instance += 1

        waiter.wait_for(
            lambda: gamelift_utils.check_total_players_in_all_game_sessions(fleet_id, current_launcher_instance),
            timeout=timeout, interval=5,
            exc=AssertionError("Client {} never connected".format(current_launcher_instance)))


def create_gamelift_flexmatch_game_and_player_sessions(matchmaking_config_name, fleet_id, gamelift_credentials, timeout,
                                                       remote_console_instances, multiplayer_client_launcher_list):
    # First 2 instances are reserved for creating game sessions. 3 and 4 for player sessions. 
    # 5 and 6 instance are used for flex match.
    # are used for player sessions
    current_launcher_instance = 4

    multiplayer_client_launcher_count = len(multiplayer_client_launcher_list)
    while current_launcher_instance < multiplayer_client_launcher_count:
        launcher = multiplayer_client_launcher_list[current_launcher_instance]
        launcher.args = []
        add_game_lift_creds_and_endpoint_args(launcher, gamelift_credentials)
        launcher.start()

        # Hard wait for the clients to initialize. Hoping 10secs is enough on the worst machines as well.
        time.sleep(10)

        remote_console_instances[current_launcher_instance].start()

        waiter.wait_for(lambda: network_utils.check_for_listening_port(4600 + current_launcher_instance),
                        timeout=timeout,
                        exc=AssertionError('Port 460{} not listening.'.format(current_launcher_instance)))
        current_launcher_instance += 1

    current_launcher_instance = 4
    while current_launcher_instance < multiplayer_client_launcher_count:
        remote_console_instances[current_launcher_instance].send_command('gamelift_flexmatch {}'
                                                                         .format(matchmaking_config_name))

        remote_console_instances[current_launcher_instance].stop()
        current_launcher_instance += 1

    waiter.wait_for(
        lambda: gamelift_utils.check_total_players_in_all_game_sessions(fleet_id, multiplayer_client_launcher_count),
        timeout=timeout, interval=5,
        exc=AssertionError("Client 5 and 6 never connected"))

def create_gamelift_custom_flexmatch_game_and_player_sessions(matchmaking_config_name, fleet_id, gamelift_credentials, timeout,
                                                       remote_console_instances, multiplayer_client_launcher_list):

    current_launcher_instance = 0

    multiplayer_client_launcher_count = len(multiplayer_client_launcher_list)
    # Initialized all 4 clients
    while current_launcher_instance < multiplayer_client_launcher_count:
        launcher = multiplayer_client_launcher_list[current_launcher_instance]
        launcher.args = []
        add_game_lift_creds_and_endpoint_args(launcher, gamelift_credentials)
        launcher.start()

        # Hard wait for the clients to initialize. Setting to 10s to allow slowest machines to connect
        time.sleep(10)

        remote_console_instances[current_launcher_instance].start()

        waiter.wait_for(lambda: network_utils.check_for_listening_port(4600 + current_launcher_instance),
                        timeout=timeout,
                        exc=AssertionError('Port 460{} not listening.'.format(current_launcher_instance)))
        current_launcher_instance += 1

    # 2 clients request matchmaking
    current_launcher_instance = 0
    while current_launcher_instance < 2:
        remote_console_instances[current_launcher_instance].send_command('gamelift_flexmatch {}'
                                                                         .format(matchmaking_config_name))
        current_launcher_instance += 1

    # for initial delay
    time.sleep(timeout)

    # Add 1 more client
    remote_console_instances[current_launcher_instance].send_command('gamelift_flexmatch {}'
                                                                     .format(matchmaking_config_name))
    # for initial delay
    time.sleep(timeout)

    current_launcher_instance += 1
    # Add 1 more client
    remote_console_instances[current_launcher_instance].send_command('gamelift_flexmatch {}'
                                                                     .format(matchmaking_config_name))

    waiter.wait_for(
        lambda: gamelift_utils.check_total_players_in_all_game_sessions(fleet_id, multiplayer_client_launcher_count),
        timeout=timeout, interval=5,
         exc=AssertionError('Client never connected {}'.format(multiplayer_client_launcher_count)))

    # Disconnect 1 client
    current_launcher_instance = 0
    remote_console_instances[current_launcher_instance].send_command('mpdisconnect')
    waiter.wait_for(
        lambda: gamelift_utils.check_total_players_in_all_game_sessions(fleet_id, multiplayer_client_launcher_count-1),
        timeout=timeout, interval=5,
        exc=AssertionError("Client failed to disconnect"))

    time.sleep(20)

    # Reconnect disconnected client as new player
    remote_console_instances[current_launcher_instance].send_command('gamelift_flexmatch {}'
                                                                     .format(matchmaking_config_name))
    waiter.wait_for(
        lambda: gamelift_utils.check_total_players_in_all_game_sessions(fleet_id, multiplayer_client_launcher_count),
        timeout=timeout, interval=5,
        exc=AssertionError("Client never reconnected"))

    # Disconnect 3 clients. Game Session should end.
    current_launcher_instance = 1
    while current_launcher_instance < multiplayer_client_launcher_count:
        remote_console_instances[current_launcher_instance].send_command('mpdisconnect')
        current_launcher_instance += 1

    waiter.wait_for(
        lambda: gamelift_utils.check_total_players_in_all_game_sessions(fleet_id, 0),
        timeout=timeout, interval=5,
        exc=AssertionError("Client did not disconnect"))


def create_gamelift_game_sessions(fleet_id, queue_name, gamelift_credentials, timeout,
                                  remote_console_instances, multiplayer_client_launcher_list):
    """
     Creates Gamelift game sessions. Connects the current clients to the created game sessions.
    :param fleet_id: Fleet to create game sessions on
    :param gamelift_credentials: Credentials to use to make GameLift calls
    :param timeout: Timeout for waiter calls
    :param remote_console_instances: Remote console instances to work with
    :param multiplayer_client_launcher_list: Client launcher instances to work with
    :return:
    """

    # First 2 launcher and remote console instances are reserved for creating game sessions
    current_launcher_instance = 0
    while current_launcher_instance < 2:
        launcher = multiplayer_client_launcher_list[current_launcher_instance]
        launcher.args = []
        add_game_lift_creds_and_endpoint_args(launcher, gamelift_credentials)
        launcher.args.extend(['+map', 'gamelobby'])

        if current_launcher_instance == 0:
            launcher.args.extend(['+gamelift_fleet_id', fleet_id, '+sv_port', '33435'])
        else:
            launcher.args.extend(['+gamelift_queue_name', queue_name, '+sv_port', '33436'])

        # Normally we start a launcher using with: which utilizes context managers to ensure teardown
        # However we are using multiple launchers so we ensure launcher.stop() is called in custom teardown
        launcher.start()

        # Hard wait for the clients to initialize. Hoping 10secs is enough on the worst machines as well.
        time.sleep(10)

        # Checks if the port is listening
        waiter.wait_for(lambda: network_utils.check_for_listening_port(4600 + current_launcher_instance),
                        timeout=timeout,
                        exc=AssertionError('Port 460{} not listening.'.format(current_launcher_instance)))

        # Start the remote console
        remote_console_instances[current_launcher_instance].start()

        # Set expected log line to assert
        gamelift_host_event = remote_console_instances[current_launcher_instance].\
            expect_log_line('(GameLift) - Initialized GameLift client successfully.', 90)

        # Send the remote console command to host a multiplayersample game
        remote_console_instances[current_launcher_instance].\
            send_command('gamelift_host AutomationTest multiplayersample 8')

        assert gamelift_host_event(), 'Console never stated gamelift hosting was completed properly'

        current_launcher_instance += 1

        # Assert on if our client connected
        waiter.wait_for(lambda: gamelift_utils.check_total_players_in_all_game_sessions(fleet_id,
                                                                                     current_launcher_instance),
                        timeout=timeout,
                        interval=5,
                        exc=AssertionError("Client {} never connected".format(current_launcher_instance)))


@pytest.mark.parametrize('platform', ['win_x64_vs2017'])
@pytest.mark.parametrize('configuration', ['profile'])
@pytest.mark.parametrize('project', ['MultiplayerSample'])
@pytest.mark.parametrize('spec', ['all'])
@pytest.mark.parametrize('level', ['multiplayersample'])
@pytest.mark.parametrize('number_of_launchers', [6])
class TestRunWindowsGamelift(object):
    """
    This test will upload a build to Gamelift, creates a GameLift fleet and queue.
    Deploys 2 game sessions one directly on the fleet using fleetId and other using the queue (Queue name).
    1 game session created using Automatic backfill mode.
    It then connects clients between these 2 sessions. Half clients are connected using fleetId and
    other half using queueName
    Since MultiplayerSample is implemented on the client side to connect to first game session found from the list
    returned by GameLift describe game sessions and additionally GameLift response is non-deterministic(order),
    what is being verified is that across 2 game sessions total number of players can be tallied.
    """
    def test_RunWindowsGamelift(self, request, configuration, remote_console_instances,
                                multiplayer_client_launcher_list, workspace):
        # debug takes longer to load
        TIMEOUT = 60
        if configuration == 'debug':
            TIMEOUT = 120

        # Delete previous logs
        if os.path.exists(workspace.paths.project_log()):
            try:
                shutil.rmtree(workspace.paths.project_log())
            except Exception as e:
                logger.error(e)

        # Test setup
        gamelift_credentials = GameliftUtils.get_aws_credentials(AWS_PROFILE)
        file_utils.clear_out_file(os.path.join(workspace.paths.project(), 'initialmap.cfg'))
        workspace.configuration = configuration

        # Process Assets
        process_assets_success = workspace.asset_processor.batch_process()
        assert process_assets_success, 'Assets did not process correctly'

        # Create PC Gamelift package
        gamelift_utils.create_gamelift_package(workspace.paths.dev(), workspace.paths._find_bin_dir())

        # Upload Gamelift build
        build_root = os.path.join(workspace.paths.dev(), 'MultiplayerSample_pc_Paks_Dedicated')
        gamelift_build_id = GameliftUtils.upload_build_to_gamelift(build_root, 'WindowsGameliftTest', 'WINDOWS_2012',
                                                                   REGION)

        # Assert that the build uploaded successfully
        waiter.wait_for(lambda: gamelift_utils.is_build_status_ready(gamelift_build_id),
                        timeout=TIMEOUT, interval=INTERVAL_TIME,
                        exc=AssertionError("Gamelift build never became active"))

        # Create a Gamelift fleet
        runtime_configuration = networking_constants.WINDOWS_GAMELIFT_FLEET['runtime_configuration']
        ec2_instance_type = networking_constants.WINDOWS_GAMELIFT_FLEET['ec2_instance_type']
        fleet_name = networking_constants.WINDOWS_GAMELIFT_FLEET['fleet_name']
        build_dir = "{}.Dedicated".format(workspace.paths._find_bin_dir())
        for server_process in runtime_configuration['ServerProcesses']:
            server_process['LaunchPath'] = os.path.join("C:\\", "game", build_dir,
                                                        "MultiplayerSampleLauncher_Server.exe")
        # IPs should be manually set when testing locally, or set to clients' IPs when automated.
        ec2_inbound_permissions = networking_constants.WINDOWS_GAMELIFT_FLEET['ec2_inbound_permissions']
        fleet_id, fleet_arn = gamelift_utils.create_gamelift_fleet(name=fleet_name,
                                                        build_id=gamelift_build_id,
                                                        ec2_instance_type=ec2_instance_type,
                                                        runtime_configuration=runtime_configuration,
                                                        ec2_inbound_permissions=ec2_inbound_permissions
                                                        )

        # Assert that the fleet starts successfully
        waiter.wait_for(lambda: gamelift_utils.check_instance_state(fleet_id), timeout=FLEET_TIMEOUT,
                        interval=INTERVAL_TIME, exc=AssertionError("Gamelift server never became active"))

        queue_name = 'MSTest-{}'.format(str(uuid.uuid4()))

        # Create GameLift queue
        queue_arn = gamelift_utils.create_gamelift_queue(queue_name, fleet_arn)

        # Create GameLift matchmaking ruleset
        rule_set_name = 'MSTest-{}'.format(str(uuid.uuid4()))
        gamelift_utils.create_gamelift_matchmaking_rule_set(rule_set_name)

        # Create GameLift matchmaking configuration
        matchmaking_config_name = 'MSTest-{}'.format(str(uuid.uuid4()))
        gamelift_utils.create_gamelift_matchmaking_config(matchmaking_config_name, queue_arn, rule_set_name,
                                                          'AUTOMATIC')

        create_gamelift_game_sessions(fleet_id, queue_name, gamelift_credentials, TIMEOUT,
                                      remote_console_instances, multiplayer_client_launcher_list)

        # Test clients connecting to game lift servers.
        create_game_lift_player_sessions(fleet_id, gamelift_credentials, TIMEOUT,
                                         remote_console_instances, multiplayer_client_launcher_list)

        # Test flex match creating game session and players connecting to the created flex match game session
        create_gamelift_flexmatch_game_and_player_sessions(matchmaking_config_name, fleet_id, gamelift_credentials,
                                                           TIMEOUT, remote_console_instances,
                                                           multiplayer_client_launcher_list)

        # Verify all clients are alive.
        for launcher in multiplayer_client_launcher_list:
            assert launcher.is_alive(), "A process was closed unexpectedly"


@pytest.mark.parametrize('platform', ['win_x64_vs2017'])
@pytest.mark.parametrize('configuration', ['profile'])
@pytest.mark.parametrize('project', ['MultiplayerSample'])
@pytest.mark.parametrize('spec', ['all'])
@pytest.mark.parametrize('level', ['multiplayersample'])
@pytest.mark.parametrize('number_of_launchers', [4])
class TestRunWindowsCustomBackfillGamelift(object):
    """
    This test will upload a build to Gamelift, creates a GameLift fleet and queue. 1 game session is created using
    custom match backfill.
    Test players connecting and disconnecting the game session.
    """
    def test_RunWindowsGamelift_Custombackfill(self, request, configuration, remote_console_instances,
                                multiplayer_client_launcher_list, workspace):
        # debug takes longer to load
        TIMEOUT = 60
        if configuration == 'debug':
            TIMEOUT = 120

        # Test setup
        gamelift_credentials = GameliftUtils.get_aws_credentials(AWS_PROFILE)
        file_utils.clear_out_file(os.path.join(workspace.paths.project(), 'initialmap.cfg'))
        workspace.configuration = configuration

        # Process Assets
        process_assets_success = workspace.asset_processor.batch_process()
        assert process_assets_success, 'Assets did not process correctly'

        # Create PC Gamelift package
        gamelift_utils.create_gamelift_package(workspace.paths.dev(), workspace.paths._find_bin_dir())

        # Upload Gamelift build
        build_root = os.path.join(workspace.paths.dev(), 'MultiplayerSample_pc_Paks_Dedicated')
        gamelift_build_id = GameliftUtils.upload_build_to_gamelift(build_root, 'WindowsGameliftTest', 'WINDOWS_2012',
                                                                   REGION)

        # Assert that the build uploaded successfully
        waiter.wait_for(lambda: gamelift_utils.is_build_status_ready(gamelift_build_id),
                        timeout=TIMEOUT, interval=INTERVAL_TIME,
                        exc=AssertionError("Gamelift build never became active"))

        # Create a Gamelift fleet
        runtime_configuration = networking_constants.WINDOWS_CUSTOMBACKFILL_GAMELIFT_FLEET['runtime_configuration']
        ec2_instance_type = networking_constants.WINDOWS_CUSTOMBACKFILL_GAMELIFT_FLEET['ec2_instance_type']
        fleet_name = networking_constants.WINDOWS_CUSTOMBACKFILL_GAMELIFT_FLEET['fleet_name']
        build_dir = "{}.Dedicated".format(workspace.paths._find_bin_dir())
        for server_process in runtime_configuration['ServerProcesses']:
            server_process['LaunchPath'] = os.path.join("C:\\", "game", build_dir,
                                                        "MultiplayerSampleLauncher_Server.exe")
        # IPs should be manually set when testing locally, or set to clients' IPs when automated.
        ec2_inbound_permissions = networking_constants.WINDOWS_GAMELIFT_FLEET['ec2_inbound_permissions']
        fleet_id, fleet_arn = gamelift_utils.create_gamelift_fleet(name=fleet_name,
                                                        build_id=gamelift_build_id,
                                                        ec2_instance_type=ec2_instance_type,
                                                        runtime_configuration=runtime_configuration,
                                                        ec2_inbound_permissions=ec2_inbound_permissions
                                                        )

        # Assert that the fleet starts successfully
        waiter.wait_for(lambda: gamelift_utils.check_instance_state(fleet_id), timeout=FLEET_TIMEOUT,
                        interval=INTERVAL_TIME, exc=AssertionError("Gamelift server never became active"))

        queue_name = 'MSTest-{}'.format(str(uuid.uuid4()))

        # Create GameLift queue
        queue_arn = gamelift_utils.create_gamelift_queue(queue_name, fleet_arn)

        # Create GameLift matchmaking ruleset
        rule_set_name = 'MSTest-{}'.format(str(uuid.uuid4()))
        gamelift_utils.create_gamelift_matchmaking_rule_set(rule_set_name)

        # Create GameLift matchmaking configuration
        matchmaking_config_name = 'MSTest-{}'.format(str(uuid.uuid4()))
        gamelift_utils.create_gamelift_matchmaking_config(matchmaking_config_name, queue_arn, rule_set_name,
                                                          'MANUAL')

        # Test flex match creating game session and players connecting to the created custome flex match game session
        create_gamelift_custom_flexmatch_game_and_player_sessions(matchmaking_config_name, fleet_id, gamelift_credentials,
                                                           TIMEOUT, remote_console_instances,
                                                           multiplayer_client_launcher_list)

        # Verify all clients are alive.
        for launcher in multiplayer_client_launcher_list:
            assert launcher.is_alive(), "A process was closed unexpectedly"
