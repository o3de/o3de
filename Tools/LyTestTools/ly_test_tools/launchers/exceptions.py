"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Exceptions that can occur while interacting with a Launcher
"""


class CrashError(Exception):
    """ Indicates an unexpected termination """


class SetupError(Exception):
    """ Indicates error during setup """


class TeardownError(Exception):
    """ Indicates error during teardown """


class WaitTimeoutError(Exception):
    """ Indicates a timeout was reaching while waitinf for a process """
    

class ProcessNotStartedError(Exception):
    """ Indicates that the process was never started and was required for the operation """