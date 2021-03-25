"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import logging

import gamelift_utils

logger = logging.basicConfig(filename='test.log', filemode='w', level=logging.DEBUG)


def main():
    """
    This script cleans up the builds and fleets from gamelift. The main method retrieves a list of build and fleet
    ids, and then sends a console command to delete them. The script will exclude any builds and fleets that are
    currently in use. It will then validate to make sure all non excluded builds and fleets are deleted.
    """
    exclude_fleet_list = gamelift_utils.get_in_use_fleet_list_from_gamelift()
    fleet_list = gamelift_utils.get_fleet_list_from_gamelift(exclude_fleet_list)
    exclude_build_list = gamelift_utils.get_in_use_build_list_from_gamelift()
    build_list = gamelift_utils.get_build_list_from_gamelift(exclude_build_list)

    for fleet in fleet_list:
        gamelift_utils.delete_fleet_from_gamelift(fleet)

    assert validate_fleet_list_deleted(exclude_fleet_list), 'Fleet list was not deleted properly!'

    for build in build_list:
        gamelift_utils.delete_build_from_gamelift(build)

    assert validate_build_list_deleted(exclude_build_list), 'Build list was not deleted properly!'


def validate_build_list_deleted(exclude_build_list):
    """
    Validates whether the builds were deleted correctly
    :param exclude_build_list: a list of builds that should not be deleted
    :return: returns True if the builds are deleted correctly
    """
    build_is_empty = True
    build_list = gamelift_utils.get_build_list_from_gamelift()

    for build in build_list:
        if build not in exclude_build_list:
            build_is_empty = False
            break

    return build_is_empty


def validate_fleet_list_deleted(exclude_fleet_list):
    """
    Validates whether the fleets were deleted correctly
    :param exclude_fleet_list: a list of fleets that should not be deleted
    :return: returns True if the fleets are deleted correctly
    """
    fleet_is_empty = True
    fleet_attribute_list = gamelift_utils.get_fleet_list_attributes_from_gamelift()

    for fleet_attrib in fleet_attribute_list:
        if (fleet_attrib['Status'] not in ["DELETING"]) and (fleet_attrib['FleetId'] not in exclude_fleet_list):
            fleet_is_empty = False
            break

    return fleet_is_empty


if __name__ == '__main__':
    main()
