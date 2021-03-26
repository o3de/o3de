"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import ConfigParser
import json
import subprocess
import shutil
from s3_utils import *


logger = logging.getLogger(__name__)


def get_aws_credentials():
    """
    Gets credentials from .aws/credentials file.  QA will setup our nodes using one default set for Isengard
    :return: Dictionary of AWS Credentials gathered
    """
    aws_credentials = {}
    home = os.path.expanduser("~")
    IniRead = ConfigParser.ConfigParser()
    IniRead.read('{}\.aws\credentials'.format(home))
    aws_credentials['access'] = IniRead.get('default', 'aws_access_key_id')
    aws_credentials['secret'] = IniRead.get('default', 'aws_secret_access_key')
    return aws_credentials


def upload_build_to_gamelift(dev_path):
    """
    Function to upload a build to gamelift
    :param dev_path:  Path to the dev folder of Lumberyard
    Example command:
    aws Gamelift upload-build --name "1.11 pc-47 (profile)" --build-version 1.11 --build-root 
    "F:\lumberyard-1.11-472772-pc-47\dev\multiplayersample_pc_paks_dedicated" --operating-system WINDOWS_2012 
    --region us-west-2
    """

    upload_build_cmd = ['aws', 'gamelift', 'upload-build', '--name', dev_path, '--build-version', '1.0.0',
                        '--operating-system',
                        'WINDOWS_2012', '--build-root',
                        r'{}\MultiplayerSample_pc_Paks_Dedicated'.format(dev_path),
                        '--region', 'us-west-1']

    proc = subprocess.Popen(upload_build_cmd, shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            cwd=dev_path)
    lines = []

    for stdout_line in iter(proc.stdout.readline, ''):
        lines.append(stdout_line)
    proc.stdout.close()

    assert ('Successfully uploaded' in lines[0]), "Not successfully uploaded"
    build_id = lines[1].split(" ")[-1:][0].rstrip()

    logger.info(build_id)

    return build_id


def create_gamelift_fleet(dev_path, build_id, build_dir):
    """
    Function to create a fleet within Gamelift
    :param dev_path:  Path to the dev folder of Lumberyard
    :param build_id: The id of the build given back to us from Gamelift when we uploaded a build
    :param build_dir: The directory where the build exists
    :return: ID of the fleet that was created
    """
    upload_build_cmd = ['aws', 'gamelift', 'create-fleet', '--name', dev_path, '--description',
                       'Automated Testing for Networking', '--build-id', build_id,
                        '--ec2-instance-type', 'c3.large', '--runtime-configuration',
                        'ServerProcesses=[{'
                        + 'LaunchPath=C:\game\{}\MultiplayerSampleLauncher_Server.exe'.format(build_dir) +
                        ',Parameters=+sv_port 33435 '
                        '+gamelift_start_server,ConcurrentExecutions=1}]',
                        '--ec2-inbound-permissions', 'FromPort=33435,ToPort=33436,IpRange=0.0.0.0/0,Protocol=UDP',
                        'FromPort=3389,ToPort=3389,IpRange=0.0.0.0/0,Protocol=TCP', '--log-paths', 'c:\game\user\log',
                        '--region', 'us-west-1']

    output, _ = subprocess.Popen(upload_build_cmd, shell=True,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT,
                                 cwd=dev_path).communicate()

    logger.info(output)

    try:
        fleet_id = json.loads(output)['FleetAttributes']['FleetId']
    except ValueError:
        raise AssertionError('Json value not returned when executing upload, build')

    return fleet_id


def check_instance_state(dev_path, fleet_id):
    """
    Checked the state of an instance.  Returns true when the instance is in an Active state
    :param dev_path:  Path to the dev folder of Lumberyard
    :param fleet_id: the Id of the fleet we are checking the state of
    :return: True when fleet is in the active state
    """
    fleet_attribute = get_fleet_list_attributes_from_gamelift(fleet_id)

    try:
        if len(fleet_attribute) == 0:
            return False
        elif fleet_attribute[0]['Status'] == 'ACTIVE':
            return True
    except ValueError:
        raise AssertionError('Json value not returned when executing upload, build')

    return False


def check_current_player_sessions(fleet_id, number_of_players):
    """
    Checks for the number of players inside a game session
    :param dev_path:  Path to the dev folder of Lumberyard
    :param fleet_id: The ID of the gamelift fleet
    :param number_of_players: Number of players to be expecting for this function to verify
    :return:  Returns true of the number of players match the number of connected clients
    """
    game_session = get_game_session_from_gamelift(fleet_id)
    if len(game_session) == 0:
        return False
    elif game_session[0]['CurrentPlayerSessionCount'] == number_of_players:
        return True


def run_multiplayer_sample_paks_pc_dedicated(dev_path):
    """
    Runs a bat script which will package up the items needed for Gamelift
    :param dev_path:  Path to the dev folder of Lumberyard
    """
    cmd = [dev_path + r'\BuildMultiplayerSample_Paks_PC_dedicated.bat']

    try:
        proc = subprocess.Popen(cmd, shell=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,
                                cwd=dev_path)
        for stdout_line in iter(proc.stdout.readline, ''):
            logger.info(stdout_line)
        proc.stdout.close()
    except subprocess.CalledProcessError as e:
        assert e.returncode > 0, 'Script to make PC Paks for Gamelift failed'


def copy_bin_folder(bin_directory, dev_path):
    """
    Copies bin folder into the MultiplayerSamples_pc_Paks_Dedicated folder for Windows Gamelift Builds
    :param bin_directory:
    :param dev_path:
    :return:  None
    """
    to_folder = dev_path + '\\MultiplayerSample_pc_Paks_Dedicated\\' + bin_directory
    if os.path.exists(to_folder):
        shutil.rmtree(to_folder, onerror=onerror)
    shutil.copytree(os.path.join(dev_path, bin_directory), to_folder)


def download_required_gamelift_files(destination_dir):
    """
    Downloads required vc redistributables and their installation script from S3.
    :param destination_dir: Path on disk to be checked.
    """
    gamelift_files_bucket = 'ly-net-gamelift-required-files'
    keys = ['vc_redist.x64.exe', 'VC_redist2017.x64.exe', 'vcredist_x64.exe', 'install.bat',
            'msvcp140d.dll', 'ucrtbased.dll', 'vcruntime140d.dll']

    for file_key in keys:
        download_from_bucket(gamelift_files_bucket, file_key, destination_dir)


def create_gamelift_package(dev_path, bin_directory):
    """
    Creates a package ready to be deployed to gamelift via aws cli
    :param dev_path: Path to dev directory
    :param bin_directory: path to bin directory
    :return: None
    """
    run_multiplayer_sample_paks_pc_dedicated(dev_path)

    download_required_gamelift_files(dev_path + r'\MultiplayerSample_pc_Paks_Dedicated')

    copy_bin_folder("{}.Dedicated".format(bin_directory), dev_path)


def onerror(func, path, exc_info):
    """
    Error handler for ``shutil.rmtree``.
    If the error is due to an access error (read only file)
    it attempts to add write permission and then retries.
    If the error is for another reason it re-raises the error.
    """
    import stat
    if not os.access(path, os.W_OK):
        # Is the error an access error ?
        os.chmod(path, stat.S_IWUSR)
        func(path)
    else:
        raise


def get_game_session_from_gamelift(fleet_id):
    get_game_session_cmd = ['aws', 'gamelift', 'describe-game-sessions', '--fleet-id', fleet_id]
    try:
        output, _ = subprocess.Popen(get_game_session_cmd, shell=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,).communicate()
    except subprocess.CalledProcessError as e:
        assert e.returncode > 0, 'AWS Gamelift get_game_session_from_gamelift command failed'

    logger.info(output)

    try:
        game_session = json.loads(output)['GameSessions']
    except ValueError:
        raise AssertionError('Json values not returned when executing get_game_session_cmd command')
    # Return None if there is no active game_session for the fleet

    return game_session


def get_build_id_from_fleet_id(fleet_id):
    get_build_from_id_cmd = ['aws', 'gamelift', 'describe-fleet-attributes', '--fleet-id', fleet_id]
    try:
        output, _ = subprocess.Popen(get_build_from_id_cmd, shell=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,).communicate()
    except subprocess.CalledProcessError as e:
        assert e.returncode > 0, 'AWS Gamelift get_build_id_from_fleet_id command failed'

    logger.info(output)

    try:
        fleet_attribute = json.loads(output)['FleetAttributes']
        build_id = fleet_attribute[0]['BuildId']
    except ValueError:
        raise AssertionError('Json values not returned when executing get_build_id_from_fleet_id command')

    return build_id


def get_exclude_build_list_from_gamelift():
    """
    Function to get the excluded builds and return them to a new list. Excluded builds include those that are being
    initalized and if an excluded fleet is using that build
    :return: list
    """
    exclude_build_list = []

    exclude_fleet_list = get_exclude_fleet_list_from_gamelift()
    for fleet in exclude_fleet_list:
        exclude_build_list.append(get_build_id_from_fleet_id(fleet))

    build_list = get_build_list_from_gamelift()

    for build_id in build_list:
        get_describe_build_list_cmd = ['aws', 'gamelift', 'describe-build', '--build-id', build_id]

        try:
            output, error = subprocess.Popen(get_describe_build_list_cmd, shell=True,
                                             stdout=subprocess.PIPE,
                                             stderr=subprocess.STDOUT, ).communicate()
        except subprocess.CalledProcessError as e:
            assert e.returncode > 0, 'AWS Gamelift get_describe_build_list_cmd command failed'

        try:
            build = json.loads(output)['Build']
            if build['Status'] == 'INITIALIZED':
                exclude_build_list.append(build['BuildId'])
        except ValueError:
            raise AssertionError('Json values not returned when executing get_describe_build_list_cmd command')

    return exclude_build_list


def get_exclude_fleet_list_from_gamelift():
    """
    Function to remove the excluded fleets and return them to a new list. Excluded fleets include those that are being
    initialized or currently in use in an active game session.
    :return: list
    """
    exclude_fleet_list = []
    fleet_list = get_fleet_list_from_gamelift()

    fleet_attribute_list = get_fleet_list_attributes_from_gamelift()
    for fleet_attrib in fleet_attribute_list:
        if fleet_attrib['Status'] in ["DOWNLOADING", "ACTIVATING", "NEW", "VALIDATING", "BUILDING"]:
            exclude_fleet_list.append(fleet_attrib['FleetId'])

    for fleet_id in fleet_list:
        game_session = get_game_session_from_gamelift(fleet_id)
        if len(game_session) != 0 and game_session[0]['CurrentPlayerSessionCount'] > 0:
            exclude_fleet_list.append(fleet_id)

    return exclude_fleet_list


def get_fleet_list_from_gamelift(exclude_fleet_list=[]):
    """
    Function to retrieve all fleet Id's from gamelift that are not in the excluded list. If no list is provided, it will
    return all fleet Id's
    :return: list
    """
    get_fleet_list_cmd = ['aws', 'gamelift', 'list-fleets']

    try:
        output, error = subprocess.Popen(get_fleet_list_cmd, shell=True,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT,).communicate()
    except subprocess.CalledProcessError as e:
        assert e.returncode > 0, 'AWS Gamelift get_fleet_list_cmd command failed'

    logger.info(output)
    next_token = None

    try:
        next_token = json.loads(output)['NextToken']
    except KeyError:
        pass
        # If the NextToken exists take it, otherwise ignore that we don't have one

    try:
        all_fleet_list = json.loads(output)['FleetIds']
        if next_token is not None:
            get_next_fleet_list_cmd = ['aws', 'gamelift', 'list-fleets', '--next-token', next_token]
            try:
                output, error = subprocess.Popen(get_next_fleet_list_cmd, shell=True,
                                                 stdout=subprocess.PIPE,
                                                 stderr=subprocess.STDOUT, ).communicate()
                next_fleet_list = json.loads(output)['FleetIds']
                all_fleet_list += next_fleet_list

            except subprocess.CalledProcessError as e:
                assert e.returncode > 0, 'AWS Gamelift get_next_fleet_list_cmd command failed'

            logger.info(output)

    except ValueError:
        raise AssertionError('Json values not returned when executing get_fleet_list_cmd command')

    fleet_list = []
    for fleet in all_fleet_list:
        if fleet not in exclude_fleet_list:
            fleet_list.append(fleet)
    return fleet_list


def get_build_list_from_gamelift(exclude_build_list=[]):
    """
    Function to retrieve all build ID's from gamelift that are not in the excluded list. If no list is provided, it will
    return all build Id's
    :return: list
    """
    get_build_list_cmd = ['aws', 'gamelift', 'list-builds']

    try:
        output, _ = subprocess.Popen(get_build_list_cmd, shell=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,).communicate()
    except subprocess.CalledProcessError as e:
        assert e.returncode > 0, 'AWS Gamelift get_build_list_cmd command failed'

    logger.info(output)

    try:
        build_list = json.loads(output)['Builds']
    except ValueError:
        raise AssertionError('Json values not returned when executing get_build_list_cmd command')

    build_id_list = []
    for build in build_list:
        if build['BuildId'] not in exclude_build_list:
            build_id_list.append(build['BuildId'])
    return build_id_list


def get_fleet_list_attributes_from_gamelift(fleet_id=None):
    """
    Function to retrieve the fleet attribute for a given fleet. Will return all fleet attributes if no fleet_id is given
    :return: list
    """
    if fleet_id is None:
        get_fleet_list_attributes_cmd = ['aws', 'gamelift', 'describe-fleet-attributes']
    else:
        get_fleet_list_attributes_cmd = ['aws', 'gamelift', 'describe-fleet-attributes', '--fleet-id', fleet_id]
    try:
        output, _ = subprocess.Popen(get_fleet_list_attributes_cmd, shell=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,).communicate()
    except subprocess.CalledProcessError as e:
        assert e.returncode > 0, 'AWS Gamelift get_fleet_list_attributes_cmd command failed'

    logger.info(output)
    next_token = None

    try:
        next_token = json.loads(output)['NextToken']
    except KeyError:
        pass
        # If the NextToken exists take it, otherwise ignore that we don't have one

    try:
        fleet_attribute_list = json.loads(output)['FleetAttributes']
        if next_token is not None:
            get_next_fleet_list_cmd = ['aws', 'gamelift', 'describe-fleet-attributes', '--next-token', next_token]
            try:
                output, error = subprocess.Popen(get_next_fleet_list_cmd, shell=True,
                                                 stdout=subprocess.PIPE,
                                                 stderr=subprocess.STDOUT, ).communicate()
                next_fleet_list = json.loads(output)['FleetAttributes']
                fleet_attribute_list += next_fleet_list

            except subprocess.CalledProcessError as e:
                assert e.returncode > 0, 'AWS Gamelift get_next_fleet_list_cmd next token command failed'

            logger.info(output)

    except ValueError:
        raise AssertionError('Json values not returned when executing get_fleet_list_attributes_cmd command')

    return fleet_attribute_list


def delete_build_from_gamelift(build_id):
    """
    Function to delete a build from gamelift
    :param build_id:  The id of the build given back to us from Gamelift when we uploaded a build
    Example command:
    aws Gamelift delete-build --build-id build-f4t3gFDS32dfDa
    """
    delete_build_cmd = ['aws', 'gamelift', 'delete-build', '--build-id', build_id]
    try:
        output, _ = subprocess.Popen(delete_build_cmd, shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT).communicate()
    except subprocess.CalledProcessError as e:
        assert e.returncode > 0, 'AWS Gamelift delete_build_cmd command failed'

    logger.info(output)


def delete_fleet_from_gamelift(fleet_id):
    """
    Function to delete a fleet from gamelift
    :param fleet_id:  The id of the fleet given back to us from Gamelift when we created a fleet
    Example command:
    aws Gamelift delete-build --build-id build-f4t3gFDS32dfDa
    """

    # Update fleet capacity command must be ran before deleting the fleet
    update_fleet_capacity_cmd = ['aws', 'gamelift', 'update-fleet-capacity', '--fleet-id', fleet_id, '--desired-instances', '0']

    try:
        output_update, _ = subprocess.Popen(update_fleet_capacity_cmd, shell=True,
                     stdout=subprocess.PIPE,
                     stderr=subprocess.STDOUT).communicate()
    except subprocess.CalledProcessError as e:
        assert e.returncode > 0, 'AWS Gamelift update_fleet_capacity_cmd command failed'

    logger.info(output_update)

    delete_fleet_cmd = ['aws', 'gamelift', 'delete-fleet', '--fleet-id', fleet_id]

    try:
        output_delete, _ = subprocess.Popen(delete_fleet_cmd, shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT).communicate()
    except subprocess.CalledProcessError as e:
        assert e.returncode > 0, 'AWS Gamelift delete_fleet_cmd command failed'

    logger.info(output_delete)


