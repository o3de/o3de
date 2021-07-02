"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Settings file for the TestRailImporter tool.
"""

TESTRAIL_STATUS_IDS = {  # For more info see http://docs.gurock.com/testrail-api2/reference-statuses
    'pass_id': 1,
    'block_id': 2,
    'untested_id': 3,
    'retest_id': 4,
    'fail_id': 5,
    'na_id': 6,
    'pass_issues_id': 7,
    'failed_nonblocker_id': 8,
    'punted_id': 9,
}
