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
    driver_argument: The argument that will be passed to the tiaf_driver. This can be used in TestImpact objects to access any of the values that might be passed by TIAF driver arguments.
    runtime_arg: The argument that we will set the value to.
    message: The message to log when we apply this argument to the runtime.
    """
    # Base arguments
    COMMON_SEQUENCE = ("sequence", "--sequence=", "Sequence type is set to: ")
    COMMON_FPOLICY = ("test_failure_policy", "--fpolicy=",
               "Test failure policy is set to: ")
    COMMON_SUITE = ("suite", "--suite=", "Test suite is set to: ")
    COMMON_EXCLUDE = ("exclude_file", "--excluded=",
               "Exclude file found, excluding tests stored at: ")
    COMMON_TEST_TIMEOUT = ("test_timeout", "--ttimeout=",
                    "Test target timeout in seconds is set to: ")
    COMMON_GLOBAL_TIMEOUT = ("global_timeout", "--gtimeout=",
                      "Global sequence timeout in seconds is set to: ")
    COMMON_IPOLICY = ("integration_policy", "--ipolicy=",
               "Integration failure policy is set to: ")
    COMMON_CHANGELIST = ("change_list", "--changelist=", "Change list is set to: ")
    COMMON_REPORT = ("report", "--report=", "Sequencer report file is set to: ")
    COMMON_TARGET_OUTPUT = ("target_output", "--targetout=",
                      "Test target output capture is set to: ")

    # Native arguments
    NATIVE_SAFEMODE = ("safe_mode", "--safemode=", "Safe mode set to: ")

    # Python arguments
    PYTHON_TEST_RUNNER = ("testrunner_policy", "--testrunner=", "Test runner policy is set to: ")

    def __init__(self, driver_argument, runtime_arg, message):
        self.driver_argument = driver_argument
        self.runtime_arg = runtime_arg
        self.message = message
