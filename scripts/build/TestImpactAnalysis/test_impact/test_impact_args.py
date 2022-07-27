#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from enum import Enum

value = 0


class RuntimeArgs(Enum):
    """
    Enum for all of the possible arguments that we could pass through to the TIAF runtime.
    Defined by three properties:
    py_arg: The argument that will be passed to the tiaf_driver.
    runtime_arg: The argument that we will set the value to.
    message: The message to log when we apply this argument to the runtime.
    """
    # Base arguments
    SEQUENCE = ("sequence", "--sequence=", "Sequence type is set to: ")
    FPOLICY = ("test_failure_policy", "--fpolicy=",
               "Test failure policy is set to: ")
    SUITE = ("suite", "--suite=", "Test suite is set to: ")
    EXCLUDE = ("exclude_file", "--exclude=",
               "Exclude file found, excluding tests stored at: ")
    TEST_TIMEOUT = ("test_timeout", "--ttimeout=",
                    "Test target timeout in seconds is set to: ")
    GLOBAL_TIMEOUT = ("global_timeout", "--gtimeout=",
                      "Global sequence timeout in seconds is set to: ")
    IPOLICY = ("integration_policy", "--ipolicy=",
               "Integration failure policy is set to: ")
    CHANGELIST = ("change_list", "--changelist=", "Change list is set to: ")

    # Native arguments
    SAFEMODE = ("safe_mode", "--safemode=", "Safe mode set to: ")

    # Python arguments

    def __init__(self, py_arg, runtime_arg, message):
        self.py_arg = py_arg
        self.runtime_arg = runtime_arg
        self.message = message
