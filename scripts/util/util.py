#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import json
import os
import re
import sys
import subprocess


def error(message):
    print(('Error: {}'.format(message)))
    exit(1)


def warn(message):
    print(('Warning: {}'.format(message)))


def execute_system_call(command, **kwargs):
    print(('Executing subprocess.check_call({})'.format(command)))
    try:
        subprocess.check_call(command, **kwargs)
    except subprocess.CalledProcessError as e:
        print((e.output))
        error('Executing subprocess.check_call({}) failed with error {}'.format(command, e))
    except FileNotFoundError as e:
        error("File Not Found - Failed to call {} with error {}".format(command, e))


def safe_execute_system_call(command, **kwargs):
    print(('Executing subprocess.check_call({})'.format(command)))
    try:
        subprocess.check_call(command, **kwargs)
    except subprocess.CalledProcessError as e:
        print((e.output))
        warn('Executing subprocess.check_call({}) failed'.format(command))
        return e.returncode
    return 0
