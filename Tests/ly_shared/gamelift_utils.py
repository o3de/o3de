"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Requires boto3 >= 1.12.4
"""

import configparser
import shutil
import os
import logging
import pytest
import six

pytest.importorskip("botocore")
from botocore.exceptions import ClientError
pytest.importorskip("boto3")
import boto3

import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.environment.file_system as file_system
from . import s3_utils

logger = logging.getLogger(__name__)
DEFAULT_REGION = 'us-west-2'


class GameliftUtils(object):
    """
    Stores a boto3 gamelift client to use for AWS Gamelift functionalities. Static methods are provided when no client
    is needed for the function.
    """
    def __init__(self, session=None):
        # type: (boto3.Session) -> None
        """
        The gamelift client can be set during init, or a default one will be created.
        :param client: A boto3 gamelift client
        """
        self._client = None
        self._session = session

    @property
    def client(self):
        if self._client is None:
            if self._session:
                self._client = self._session.client('gamelift')
            else:
                logger.info(f"No client provided, using default credentials with default region: {DEFAULT_REGION}")
                self._client = boto3.Session(region_name=DEFAULT_REGION).client('gamelift')
        return self._client

    @staticmethod
    def get_aws_credentials(profile):
        # type: () -> dict
        """
        Gets credentials from .aws/credentials file.  QA will setup our nodes using one default set for Isengard
        :profile: The profile name found in the credentials file. ex. [default]
        :return: Dictionary of AWS Credentials gathered
        """
        aws_credentials = {}
        home = os.path.expanduser("~")
        IniRead = configparser.ConfigParser()
        IniRead.read(os.path.join(home, '.aws', 'credentials'))
        aws_credentials['access'] = IniRead.get(profile, 'aws_access_key_id')
        aws_credentials['secret'] = IniRead.get(profile, 'aws_secret_access_key')
        return aws_credentials

    # This method is static because it uses the AWS CLI to upload a build locally. Boto3 only provides a way to create
    # a gamelift build from s3.
    @staticmethod
    def upload_build_to_gamelift(source_folder, name, operating_system, region, build_version='1.0.0'):
        # type: (str, str, str, str, str) -> str
        """
        Function to upload a build to gamelift. We use AWS CLI because boto3 can only create builds from s3

        :param source_folder:  Path to the build zip file
        :param name:  Name of the Gamelift Build
        :param operating_system:  Operating system parameter for the aws gamelift upload-build command
        :param region:  AWS region, such as us-east-1
        :param build_version:  Build version as a string
        Example command:
        aws gamelift upload-build --name "1.11 pc-47 (profile)" E:/lyengine/dev --build-version 1.11 --build-root
        "F:\\lumberyard-1.11-472772-pc-47\\dev\\multiplayersample_pc_paks_dedicated" --operating-system WINDOWS_2012
        --region us-west-2
        """

        upload_build_cmd = ['aws', 'gamelift', 'upload-build',
                            '--build-root', source_folder,
                            '--name', name,
                            '--operating-system', operating_system,
                            '--region', region,
                            '--build-version', build_version,
                            ]

        # We have to use AWS CLI to upload build from local machine, boto3 only uploads builds from S3
        # shell=True only works on Windows.
        output = process_utils.check_output(upload_build_cmd, shell=True)

        # Grab the line containing the Build ID
        build_output_line = ''
        for output_line in output.splitlines():
            if "Build ID" in output_line:
                build_output_line = output_line
                break

        # Check to see if the build was uploaded successfully
        assert build_output_line, f"Gamelift build was not successfully uploaded: {output}"

        # Find the build id and strip it for escape characters
        build_id = build_output_line[build_output_line.index('build-'):].rstrip()

        logger.info(f"Build: {build_id} was successfully uploaded.")
        return build_id

    def create_gamelift_queue(self, queue_name, destination_fleet_arn, timeout=120):
        """
        Sends request to create a gamelift queue.
        :param queue_name: Name of the queue to create
        :param destination_fleet_arn: GameLift fleet the queue will place game sessions on.
        :param timeout: Placement request timeout
        :return: Queue Arn of queue created
        """
        try:
            response = self.client.create_game_session_queue(
                Name=queue_name,
                TimeoutInSeconds=timeout,
                Destinations=[
                    {
                        'DestinationArn': destination_fleet_arn
                    },
                ]
            )
            created_game_session_queue = response['GameSessionQueue']
            queue_arn = created_game_session_queue['GameSessionQueueArn']
        except Exception as e:
            raise AssertionError('GameLift queue:{} creation failed for fleetArn:{} Error:{}'.format(queue_name,
                                                                                                     destination_fleet_arn,
                                                                                                     e))
        logger.info('GameLift queue:{} created Arn:{}'.format(queue_name, queue_arn))
        return queue_arn

    def create_gamelift_matchmaking_rule_set(self, rule_set_name):
        """
        Creates GameLift matchmaking rule set
        :param rule_set_name: Name of the rule set
        :return:
        """
        try:
            rule_set = '{"name": "player_vs_asteriods", "ruleLanguageVersion": "1.0", "teams": [{"name": "Players",' \
                       '"maxPlayers": 4,"minPlayers": 2}]} '
            self.client.create_matchmaking_rule_set(
                Name=rule_set_name,
                RuleSetBody=rule_set
            )
        except ClientError as e:
            raise AssertionError('GameLift matchmaking rule set:{} creation failed. Error {}'.format(rule_set_name, e))
        logger.info('GameLift matchmaking rule set:{}'.format(rule_set_name))

    def create_gamelift_matchmaking_config(self, config_name, queue_arn, rules_set_name, backfill_mode):
        """
        Creates GameLift matchmaking config
        :param config_name: Name of the matchmaking config
        :return:
        """
        try:
            response = self.client.create_matchmaking_configuration(
                Name=config_name,
                Description='MS Test config',
                GameSessionQueueArns=[
                    queue_arn
                ],
                RequestTimeoutSeconds=120,
                AcceptanceRequired=False,
                RuleSetName=rules_set_name,
                GameProperties=[
                    {
                        'Key': 'sv_name',
                        'Value': 'MSTestServer'
                    },
                    {
                        'Key': 'sv_map',
                        'Value': 'multiplayersample'
                    }
                ],
                BackfillMode= backfill_mode
            )
        except ClientError as e:
            raise AssertionError('GameLift matchmaking config :{} creation failed {}'.format(config_name, e))
        logger.info('GameLift matchmaking config:{}'.format(config_name))

    def create_gamelift_fleet(self, name, build_id, ec2_instance_type, runtime_configuration,
                              ec2_inbound_permissions):
        """
        Function to create a fleet within Gamelift
        :param name:  Name of the Gamelift Fleet
        :param build_id: The id of the build given back to us from Gamelift when we uploaded a build
        :param ec2_instance_type: AWS instance type, such as 'c3.large'
        :param runtime_configuration: A dictionary of LaunchPath, Parameters, and ConcurrentExecutions
        :param ec2_inbound_permissions: A list of port settings for the fleet
        :return: ID,ARN of the fleet that was created
        """
        for port_settings in ec2_inbound_permissions:
            if port_settings['IpRange'] == '0.0.0.0/0':
                logger.warning('WARNING - Creating fleet with IpRange 0.0.0.0/0 is a security risk. This setting can be'
                               'changed in the networking_constants.py file')

        output = self.client.create_fleet(Name=name,
                                          Description='Automated Test',
                                          BuildId=build_id,
                                          EC2InstanceType=ec2_instance_type,
                                          EC2InboundPermissions=ec2_inbound_permissions,
                                          RuntimeConfiguration=runtime_configuration
                                          )
        logger.info(output)
        try:
            fleet_id = output['FleetAttributes']['FleetId']
            fleet_arn = output['FleetAttributes']['FleetArn']
            return fleet_id, fleet_arn
        except ValueError as e:
            problem = ValueError(f'Error when trying to return Fleet Id from output: {output}.')
            six.raise_from(problem, e)

    def is_build_status_ready(self, build_id):
        # type: (str) -> bool
        """
        Returns True if the build status is 'READY', otherwise returns False
        :param build_id: The AWS build-id to be checked
        :return: bool
        """
        build_status = self.get_build_status(build_id)
        return build_status == 'READY'

    def get_build_status(self, build_id):
        # type: (str) -> str
        """
        Returns the status of the build. Possible statuses include the following: 'INITIALIZED', 'READY', 'FAILED'
        :param build_id: The AWS build-id to be checked
        :return: The status of the build as a string
        """
        output = self.client.describe_build(BuildId=build_id)
        logger.debug(output)

        try:
            build_status = output['Build']['Status']
        except ValueError as e:
            problem = ValueError(f'Cannot find Build Status in output: {output}.')
            six.raise_from(problem, e)

        return build_status

    def check_instance_state(self, fleet_id):
        # type: (str) -> bool
        """
        Checked the state of an instance. Returns true when the instance is in an Active state. Asserts when the fleet
        has an ERROR state, otherwise false
        :param fleet_id: the Id of the fleet we are checking the state of
        :return: True when fleet is in the active state
        """
        fleet_attribute = self.get_fleet_list_attributes([fleet_id])

        # We expect fleet_attribute to be a list of one element
        try:
            if len(fleet_attribute) == 0:
                logger.debug(f'Fleet attribute was empty for fleet: {fleet_id}')
                return False
            fleet_status = fleet_attribute[0]['Status']
            if fleet_status == 'ACTIVE':
                return True
            elif fleet_status == 'ERROR':
                raise AssertionError(f'Fleet: {fleet_id} has status ERROR!')
        except ValueError as e:
            problem = ValueError(f'Error occurred when checking fleet status for fleet: {fleet_id} from attribute: '
                                 f'{fleet_attribute}')
            six.raise_from(problem, e)

        logger.debug(f'Fleet attribute was expected to be ACTIVE, but was: {fleet_status} for fleet {fleet_id}')
        return False

    def get_fleet_list_attributes(self, fleet_ids=None):
        # type: (list[str]) -> list[str]
        """
        Function to retrieve the fleet attribute for a given fleet. Will return all fleet attributes if no fleet_id is
        given
        :param fleet_ids: a list of fleet ids (can be a list of one) to get attributes for
        :return: list
        """
        fleet_attribute_list = []

        # Get paginator
        try:
            fleet_attribute_paginator = self.client.get_paginator('describe_fleet_attributes')
        except ClientError as e:
            problem = ClientError('Error occurred when getting paginator describe_fleet_attributes. Check to see if the'
                                  'botocore version is compatible')
            six.raise_from(problem, e)

        # Get iterator
        try:
            if fleet_ids:
                fleet_attribute_iterator = fleet_attribute_paginator.paginate(FleetIds=fleet_ids)
            else:
                fleet_attribute_iterator = fleet_attribute_paginator.paginate()
        except ClientError as e:
            problem = ClientError(f'Error occurred when getting fleet attributes for fleet: {fleet_ids}')
            six.raise_from(problem, e)

        # Iterate through the fleet responses
        try:
            for fleet_response in fleet_attribute_iterator:
                for fleet_attribute in fleet_response['FleetAttributes']:
                    fleet_attribute_list.append(fleet_attribute)
        except ValueError as e:
            problem = ValueError(f'Cannot find FleetAttributes in output: {fleet_response}')
            six.raise_from(problem, e)

        return fleet_attribute_list

    def check_all_current_player_sessions(self, fleet_id, number_of_players):
        # type: (str, int) -> bool
        """
        Checks for the number of players inside a game session against the expected value.
        :param fleet_id: The ID of the gamelift fleet
        :param number_of_players: Number of players to be expecting for this function to verify
        :return:  Returns true of the number of players match the number of connected clients
        """
        game_sessions = self.get_game_sessions(fleet_id)
        if len(game_sessions) == 0:
            return False
        for game_session in game_sessions:
            if game_session['CurrentPlayerSessionCount'] != number_of_players:
                return False
        return True

    def check_total_players_in_all_game_sessions(self, fleet_id, number_of_players):
        """
        Checks the number of players passed in is same as the total players in all game sessions on a Gamelift fleet.
        The reason to perform a check for total players instead of checking individual game sessions is the current
        MultiplayerSample implementation connects to the first game session it finds. In addition to that the response
        from GameLift describing active game sessions in a fleet is non-deterministic (order). So the best check is to
        verify total number of player sessions count
        :param fleet_id:
        :param number_of_players:
        :return:
        """
        game_sessions = self.get_game_sessions(fleet_id)
        total_players = 0
        for game_session in game_sessions:
            total_players += game_session['CurrentPlayerSessionCount']

        return number_of_players == total_players

    def get_game_sessions(self, fleet_id=None):
        # type: (str) -> dict
        """
        Gets a gamelift game session from the given fleet. Returns None if there is no active game_session for the fleet.
        If no fleet is provided, will return all game sessions.
        :param fleet_id: The ID of the gamelift fleet
        :return: The gamelift game session response
        """
        game_sessions_list = []
        if fleet_id:
            logger.debug(f"Getting game session for fleet: {fleet_id}")

        # Get paginator
        try:
            game_sessions_paginator = self.client.get_paginator('describe_game_sessions')
        except ClientError as e:
            problem = ClientError('Error occurred when getting paginator describe_game_sessions. Check to see if the '
                                  'botocore version is compatible')
            six.raise_from(problem, e)

        # Get iterator
        try:
            if fleet_id:
                game_sessions_iterator = game_sessions_paginator.paginate(FleetId=fleet_id)
            else:
                game_sessions_iterator = game_sessions_paginator.paginate()
        except ClientError as e:
            problem = ClientError('Error occurred when getting game sessions')
            six.raise_from(problem, e)

        # Iterate through the fleet responses
        try:
            for sessions_response in game_sessions_iterator:
                for game_session in sessions_response['GameSessions']:
                    game_sessions_list.append(game_session)
        except ValueError as e:
            problem = ValueError(f'Cannot find GameSessions in output: {sessions_response}')
            six.raise_from(problem, e)

        return game_sessions_list

    @staticmethod
    def run_multiplayer_sample_paks_pc_dedicated(dev_path):
        # type: (str) -> None
        """
        Runs a bat script which will package up the items needed for Gamelift
        :param dev_path:  Path to the dev folder of Lumberyard
        """
        run_paks_cmd = [dev_path + r'\BuildMultiplayerSample_Paks_PC_dedicated.bat']
        logger.info(process_utils.check_output(run_paks_cmd))


    @staticmethod
    def run_multiplayer_sample_linux_packer(dev_path):
        # type: (str) -> str
        """
        Runs a bat script which will create a .tar Gamelift package that can be transferred to a Linux machine
        :param dev_path:  Path to the dev folder of Lumberyard
        :return: The full path of the tar package
        """
        run_packer_cmd = os.path.join(dev_path, 'MultiplayerSample_LinuxPacker.bat')
        output = process_utils.check_output(run_packer_cmd)
        validation_text = 'Uncompressed archive successfully generated at '

        for line in output.decode().split('\r'):
            if validation_text in line:
                tar_filepath = line[line.index(validation_text)+len(validation_text):].rstrip()
                return tar_filepath
        logger.info(output)
        raise AssertionError('tar package not found!')

    @staticmethod
    def copy_bin_folder(bin_directory, dev_path):
        # type: (str, str) -> None
        """
        Copies bin folder into the MultiplayerSamples_pc_Paks_Dedicated folder for Windows Gamelift Builds
        :param bin_directory: The bin directory to copy (usually the Dedicated Bin folder)
        :param dev_path: The path to the ~/dev directory
        :return: None
        """
        to_folder = dev_path + '\\MultiplayerSample_pc_Paks_Dedicated\\' + bin_directory
        if os.path.exists(to_folder):
            file_system.delete([to_folder], True, True)
        shutil.copytree(os.path.join(dev_path, bin_directory), to_folder)

    def download_required_gamelift_files(self, destination_dir):
        # type: (str) -> None
        """
        Downloads required vc redistributables, their installation script, and debug libs from S3. Not all these files are
        required by Gamelift, but is for organizing our builds.
        :param destination_dir: Path on disk to be checked.
        """
        s3_util = s3_utils.S3Utils(self._session)
        gamelift_files_bucket = 'ly-net-gamelift-required-files'
        keys = ['vc_redist.x64.exe', 'VC_redist2017.x64.exe', 'vcredist_x64.exe', 'install.bat',
                'msvcp140d.dll', 'ucrtbased.dll', 'vcruntime140d.dll']

        for file_key in keys:
            s3_util.download_from_bucket(gamelift_files_bucket, file_key, destination_dir)

    def create_gamelift_package(self, dev_path, bin_directory):
        # type: (str, str) -> None
        """
        Creates a package ready to be deployed to gamelift via aws cli
        :param dev_path: Path to dev directory
        :param bin_directory: path to bin directory
        :return: None
        """
        paks_dir = os.path.join(dev_path, 'MultiplayerSample_pc_Paks_Dedicated')

        if os.path.exists(paks_dir):
            file_system.delete([paks_dir], True, True)

        GameliftUtils.run_multiplayer_sample_paks_pc_dedicated(dev_path)
        self.download_required_gamelift_files(paks_dir)
        GameliftUtils.copy_bin_folder(f"{bin_directory}.Dedicated", dev_path)

    def get_build_id_from_fleet_id(self, fleet_id):
        # type: (str) -> str
        """
        Retreives the build id that a fleet was created from.
        :param fleet_id: The ID from a gamelift fleet
        :return: A gamelift build id
        """
        fleet_attributes = self.get_fleet_list_attributes([fleet_id])
        # We expect a list of one element
        try:
            build_id = fleet_attributes[0]['BuildId']
        except ValueError as e:
            problem = ValueError(f'Cannot find build id in fleet attributes: {fleet_attributes} for fleet: {fleet_id}')
            six.raise_from(problem, e)

        return build_id

    def get_in_use_build_list_from_gamelift(self):
        # type: () -> list[str]
        """
        Function that returns a list of in use builds. 'In use builds' include those that are being initalized and if an
        'in use fleet' is being created from that build. This function is used to target stale builds that are either idle
        or have errored out.
        :return: A list of build id's that are in use
        """
        exclude_build_list = []

        # Gather in use fleets and their builds
        for fleet in self.get_in_use_fleet_list():
            exclude_build_list.append(self.get_build_id_from_fleet_id(fleet))

        # Gather any build that is being initialized
        build_list = self.get_build_id_list(status='INITIALIZED')
        exclude_build_list.extend(build_list)

        return exclude_build_list

    def get_build_id_list(self, status=None):
        # type: (str) -> list[str]
        """
        Gets all build id's from the boto3 client. An optional status filter is provided to gather all builds with a
        given status
        :param status: A build status to filter. Can be 'READY', 'INITIALIZED', or 'FAILED'
        :return: a list of build id's
        """
        build_list = []

        # Get paginator
        try:
            build_paginator = self.client.get_paginator('list_builds')
        except ClientError as e:
            problem = ClientError('Error occurred when getting paginator list_builds. Check to see if the '
                                  'botocore version is compatible')
            six.raise_from(problem, e)

        # Get iterator
        try:
            if status:
                build_iterator = build_paginator.paginate(Status=status)
            else:
                build_iterator = build_paginator.paginate()
        except ClientError as e:
            problem = ClientError('Error occurred when getting build list')
            six.raise_from(problem, e)

        # Iterate over the build responses
        try:
            for build_response in build_iterator:
                for build_attributes in build_response['Builds']:
                    build_list.append(build_attributes['BuildId'])
        except ValueError as e:
            problem = ValueError(f'Cannot find Builds in output: {build_response}')
            six.raise_from(problem, e)

        return build_list

    def get_in_use_fleet_list(self):
        # type: () -> list[str]
        """
        Function that returns a list of in use fleets. 'In use fleets' include those that are being initialized or
        currently in use in an active game session. This function is used to target stale fleets that are either idle
        or have errored out.
        :return: a list of fleet id's
        """
        in_use_fleet_list = set()

        # Gather all fleets
        fleet_list = self.get_fleet_list()

        # Exclude any fleets that have an "in use" status
        fleet_attribute_list = self.get_fleet_list_attributes()
        for fleet_attrib in fleet_attribute_list:
            if fleet_attrib['Status'] in ["DELETING", "DOWNLOADING", "ACTIVATING", "NEW", "VALIDATING", "BUILDING"]:
                in_use_fleet_list.add(fleet_attrib['FleetId'])

        # Exclude any fleets that are active, but have players connected to them
        for fleet_id in fleet_list:
            game_sessions = self.get_game_sessions(fleet_id)
            for game_session in game_sessions:
                if game_session['CurrentPlayerSessionCount'] > 0:
                    in_use_fleet_list.add(fleet_id)

        return list(in_use_fleet_list)

    def get_fleet_list(self):
        # type: () -> list[str]
        """
        Function to retrieve all fleet Id's from gamelift.
        :return: A list of fleet ids
        """
        fleet_list = []

        # Get paginator
        try:
            fleet_paginator = self.client.get_paginator('list_fleets')
        except ClientError as e:
            problem = ClientError('Error occurred when getting paginator list_fleets. Check to see if the '
                                  'botocore version is compatible')
            six.raise_from(problem, e)

        # Get iterator
        try:
            fleet_iterator = fleet_paginator.paginate()
        except ClientError as e:
            problem = ClientError('Error occurred when getting fleet list')
            six.raise_from(problem, e)

        # Iterate over the fleet responses
        try:
            for fleet_response in fleet_iterator:
                for fleet_id in fleet_response['FleetIds']:
                    fleet_list.append(fleet_id)
        except ValueError as e:
            problem = ValueError(f'Cannot find FleetIds in output: {fleet_response}')
            six.raise_from(problem, e)

        return fleet_list

    def delete_build_from_gamelift(self, build_id):
        # type: (str) -> None
        """
        Function to delete a build from gamelift
        :param build_id: The id of the build given back to us from Gamelift when we uploaded a build
        :return: None
        """
        try:
            self.client.delete_build(BuildId=build_id)
            logger.debug(f'Deleting build: {build_id}')
        except ClientError as e:
            problem = ClientError(f'Cannot delete build: {build_id}')
            six.raise_from(problem, e)

    def delete_fleet_from_gamelift(self, fleet_id):
        # type: (str) -> None
        """
        Function to delete a fleet from gamelift
        :param fleet_id:  The id of the fleet given back to us from Gamelift when we created a fleet
        :return: None
        """
        # Update fleet capacity command must be ran before deleting the fleet
        try:
            self.check_instance_state(fleet_id)
            self.update_fleet_capacity(fleet_id, 0)
        # Don't update fleet capacity when fleet status is in ERROR state
        except AssertionError:
            logger.debug('Cannot update fleet capacity because fleet is in an ERROR state')

        try:
            self.client.delete_fleet(FleetId=fleet_id)
            logger.debug(f"Deleting fleet: {fleet_id}")
        except ClientError as e:
            problem = f"Error occurred while attempting to delete fleet: {fleet_id}"
            six.raise_from(problem, e)

    def update_fleet_capacity(self, fleet_id, desired_instances):
        # type: (str, int) -> None
        """
        Function to update a fleet capacity in gamelift.
        :param fleet_id: The id of the fleet given back to us from Gamelift when we created a fleet
        :param desired_instances: The capacity to set
        :return: None
        """
        try:
            self.client.update_fleet_capacity(FleetId=fleet_id, DesiredInstances=desired_instances)
        except ClientError as e:
            problem = f"Error occurred while trying to update fleet: {fleet_id} to capacity: {desired_instances}"
            six.raise_from(problem, e)

    def get_gamelift_queue_names(self):
        """
        Calls describe game session queues to list current active queues.
        :return: list of queue names found
        """
        try:
            response = self.client.describe_game_session_queues()

            queue_names = []
            for queue in response['GameSessionQueues']:
                queue_names.append(queue['Name'])
            return queue_names
        except ClientError as e:
            raise AssertionError('Failed describing gamelift queues, Error:{}'.format(e))

    def delete_gamelift_queue(self, queue_name):
        """
        Deletes a gamelift queue
        :param queue_name:
        :return:
        """
        try:
            self.client.delete_game_session_queue(
                Name=queue_name
            )
        except ClientError as e:
            raise AssertionError('Failed deleting queue, Name:{} Error:{}'.format(queue_name, e))

    def get_gamelift_matchmaking_rule_set(self):
        """
        Calls describe matchmaking rule set to list current active rule sets
        :return: list of rule sets found
        """
        try:
            response = self.client.describe_matchmaking_rule_sets()
            rule_set_names = []
            for rule_set in response['RuleSets']:
                rule_set_names.append(rule_set['RuleSetName'])
            return rule_set_names
        except ClientError as e:
            raise AssertionError('Failed describing gamelift matchmaking rule sets, Error:{}'.format(e))

    def delete_gamelift_matchmaking_rule_set(self, rule_set_name):
        """
        Deletes a gamelift matchmaking rule set
        :param rule_set_name:
        :return:
        """
        try:
            self.client.delete_matchmaking_rule_set(
                Name=rule_set_name
            )
        except ClientError as e:
            raise AssertionError(
                'Failed deleting gamelift matchmaking rule set, Name:{} Error:{}'.format(rule_set_name, e))

    def get_gamelift_matchmaking_configuration(self):
        """
        Calls describe matchmaking configuration to list current active rule sets
        :return: list of rule sets found
        """
        try:
            response = self.client.describe_matchmaking_configurations()
            configuration_names = []
            for configuration in response['Configurations']:
                configuration_names.append(configuration['Name'])
            return configuration_names
        except ClientError as e:
            raise AssertionError('Failed describing gamelift matchmaking configurations, Error:{}'.format(e))

    def delete_gamelift_matchmaking_configurations(self, configuration_name):
        """
        Deletes a gamelift matchmaking configuration
        :param configuration_name:
        :return:
        """
        try:
            self.client.delete_matchmaking_configuration(
                Name=configuration_name
            )
        except ClientError as e:
            raise AssertionError(
                'Failed deleting gamelift matchmaking configuration, Name:{} Error:{}'.format(configuration_name, e))
