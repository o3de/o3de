"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Helper functions for build systems
"""
import os
import logging

logger = logging.getLogger(__name__)


def verify_build_log(build_log_path, platform, configuration):
    """
    This will search the log file for an expected success message for a specific platform configuration.
    :param build_log_path: the full path to the log file. e.g. \\dev\TestResults\timestamp_folder\pytest_results\...
    :param platform: the compiler to use, e.g. "win_x64_vs2017"
    :param configuration: the flavor of the build, e.g. "profile"
    :return: True, if success message is found within the build log. False, if success message is not found and raise an assertion error if the build log cannot be found.
    """
    success_message = "[WAF] 'build_{0}_{1}' finished successfully".format(platform, configuration)
    if os.path.exists(build_log_path):
        with open(build_log_path) as build_file:
            for line in build_file:
                if success_message in line:
                    logger.info('Success message was found for {0}_{1}'.format(platform, configuration))
                    return True
        logger.info('Success message not found for {0}_{1}'.format(platform, configuration))
        return False
    else:
        logger.info('We cannot find the build log and this is the path we are looking for {0}'.format(build_log_path))
        raise AssertionError
