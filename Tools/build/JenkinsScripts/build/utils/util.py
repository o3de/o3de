"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import json
import os
import re
import subprocess


class LyBuildError(Exception):
    def __init__(self, message):
        super(LyBuildError, self).__init__(message)

    def __str__(self):
        return str(self.message)


def ly_build_error(message):
    raise LyBuildError(message)


def error(message):
    print('Error: {}'.format(message))
    exit(1)


# Exit with status code 0 means it won't fail the whole build process
def safe_exit_with_error(message):
    print('Error: {}'.format(message))
    exit(0)


def warn(message):
    print('Warning: {}'.format(message))


def execute_system_call(command, **kwargs):
    print('Executing subprocess.check_call({})'.format(command))
    try:
        subprocess.check_call(command, **kwargs)
    except subprocess.CalledProcessError as e:
        print(e.output)
        error('Executing subprocess.check_call({}) failed with error {}'.format(command, e))
    except FileNotFoundError as e:
        error("File Not Found - Failed to call {} with error {}".format(command, e))


def safe_execute_system_call(command, **kwargs):
    print('Executing subprocess.check_call({})'.format(command))
    try:
        subprocess.check_call(command, **kwargs)
    except subprocess.CalledProcessError as e:
        print(e.output)
        warn('Executing subprocess.check_call({}) failed'.format(command))
        return e.returncode
    return 0
