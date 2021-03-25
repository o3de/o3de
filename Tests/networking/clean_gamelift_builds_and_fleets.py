"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

PreRequisites:
-AWS Credentials are set using AWS
configure
To Run:
cd ~/dev/Tests
~/Python/python3.cmd -m networking.clean_gamelift_builds_and_fleets
"""

import logging
import boto3

from ly_shared import gamelift_utils
from .test_lib import networking_constants

logger = logging.basicConfig(filename='test.log', filemode='w', level=logging.DEBUG)
AWS_PROFILE = networking_constants.DEFAULT_PROFILE
REGION = networking_constants.DEFAULT_REGION
BOTO3_SESSION = boto3.session.Session(profile_name=AWS_PROFILE, region_name=REGION)
GAMELIFT_UTILS = gamelift_utils.GameliftUtils(BOTO3_SESSION)


def main():
    """
    This script cleans up the builds and fleets from gamelift. The main method retrieves a list of build and fleet
    ids, and then sends a console command to delete them. The script will exclude any builds and fleets that are
    currently in use. It will then validate to make sure all non excluded builds and fleets are deleted.
    """   
    print("Starting Gamelift cleanup script")
    fleet_list = _get_unused_fleets_to_delete()
    build_list = _get_builds_to_delete()
    
    queue_list = GAMELIFT_UTILS.get_gamelift_queue_names()
    configuration_list = GAMELIFT_UTILS.get_gamelift_matchmaking_configuration()

    for configuration in configuration_list:
        GAMELIFT_UTILS.delete_gamelift_matchmaking_configurations(configuration)

    assert validate_matchmaking_configuration_list_empty(), 'Matchmaking configuration list was not deleted properly!'

    rule_set_list = GAMELIFT_UTILS.get_gamelift_matchmaking_rule_set()
    for rule_set in rule_set_list:
        GAMELIFT_UTILS.delete_gamelift_matchmaking_rule_set(rule_set)

    assert validate_matchmaking_rule_set_list_empty(), 'Matchmaking rule set list was not deleted properly!'

    for queue in queue_list:
        GAMELIFT_UTILS.delete_gamelift_queue(queue)

    assert validate_queue_list_empty(), 'queue list was not deleted properly!'

    for fleet in fleet_list:
        GAMELIFT_UTILS.delete_fleet_from_gamelift(fleet)

    assert validate_fleet_list_deleted(fleet_list), 'Fleet list was not deleted properly!'

    for build_id in build_list:
        GAMELIFT_UTILS.delete_build_from_gamelift(build_id)

    assert validate_build_list_deleted(build_list), 'Build list was not deleted properly!'


def _get_unused_fleets_to_delete():
    """
    Gathers all fleets and removes any in use fleets from the list. This results in a list of fleet id's to delete
    :return: a list of fleet ids
    """
    fleet_id_list = GAMELIFT_UTILS.get_fleet_list()
    for fleet_id in GAMELIFT_UTILS.get_in_use_fleet_list():
        try:
            fleet_id_list.remove(fleet_id)
        except ValueError:
            pass  # Try to remove any excluded fleets from our list
    return fleet_id_list


def _get_builds_to_delete():
    """
    Gathers all builds and removes any in use builds from the list. This results in a list of build id's to delete
    :return: a list of build ids
    """
    build_id_list = GAMELIFT_UTILS.get_build_id_list()
    for build_id in GAMELIFT_UTILS.get_in_use_build_list_from_gamelift():
        try:
            build_id_list.remove(build_id)
        except ValueError:
            pass  # Try to remove any excluded builds from our list
    return build_id_list


def validate_build_list_deleted(exclude_build_list):
    """
    Validates whether the builds were deleted correctly
    :param exclude_build_list: a list of builds that should not be deleted
    :return: returns True if the builds are deleted correctly
    """
    build_list = GAMELIFT_UTILS.get_build_id_list()

    for build in build_list:
        if build in exclude_build_list:
            return False
    return True


def validate_fleet_list_deleted(exclude_fleet_list):
    """
    Validates whether the fleets were deleted correctly
    :param exclude_fleet_list: a list of fleets that should not be deleted
    :return: returns True if the fleets are deleted correctly
    """
    fleet_attribute_list = GAMELIFT_UTILS.get_fleet_list_attributes()

    for fleet_attrib in fleet_attribute_list:
        if (fleet_attrib['Status'] != "DELETING") and (fleet_attrib['FleetId'] in exclude_fleet_list):
            return False
    return True


def validate_queue_list_empty():
    """
    Verifies there are no gamelift queues.
    :return: bool True: If no queues.
    """
    queue_list = GAMELIFT_UTILS.get_gamelift_queue_names()
    return len(queue_list) == 0


def validate_matchmaking_configuration_list_empty():
    """
    Verifies there are no gamelift matchmaking configurations.
    :return: bool True: If no queues.
    """
    configuration_list = GAMELIFT_UTILS.get_gamelift_matchmaking_configuration()
    return len(configuration_list) == 0


def validate_matchmaking_rule_set_list_empty():
    """
    Verifies there are no gamelift matchmaking rule sets.
    :return: bool True: If no queues.
    """
    rule_set_list = GAMELIFT_UTILS.get_gamelift_matchmaking_rule_set()
    return len(rule_set_list) == 0


if __name__ == '__main__':
    main()
