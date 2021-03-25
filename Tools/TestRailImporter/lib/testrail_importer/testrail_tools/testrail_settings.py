"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

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
