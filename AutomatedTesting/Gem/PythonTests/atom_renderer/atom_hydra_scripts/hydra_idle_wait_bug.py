"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Test script investigation into why general.idle_wait() is causing issues.
"""

import os
import sys

import azlmbr.paths
import azlmbr.legacy.general as general

from editor_python_test_tools.utils import TestHelper

sys.path.append(os.path.join(azlmbr.paths.devroot, "AutomatedTesting", "Gem", "PythonTests"))


def run():
    """Testing AR bug with hydra idle_wait() calls."""
    general.idle_wait(0.5)

    def verify_idle_wait_works(test):
        general.idle_wait(0.5)
        general.log(f'Calling verify_idle_wait_works(): {test}')
        general.idle_wait_frames(1)

        def is_idle_wait_working():
            general.idle_wait(0.5)
            general.log(f'Called is_idle_wait_working() once: {test}')
            general.idle_wait_frames(1)
            general.idle_wait(1.0)
            return True

        general.idle_wait(0.5)
        general.idle_wait(1.0)
        general.log(f'Testing idle_wait() bug in AR: {is_idle_wait_working()}')
        general.idle_wait(2.0)
        general.idle_wait(0.5)
        general.idle_wait_frames(1)

    class HydraIdleWaitTest:
        def __init__(self, test):
            self.run_test(test)
            self.test = test
            general.idle_wait(0.5)
            general.idle_wait_frames(1)

        def run_test(self, test):
            verify_idle_wait_works(test)
            general.idle_wait(1.0)
            general.idle_wait_frames(1)

    # Wait for Editor idle loop before executing Python hydra scripts.
    TestHelper.init_idle()
    general.idle_wait_frames(1)

    HydraIdleWaitTest('foo')
    general.idle_wait(0.5)
    general.idle_wait_frames(1)

    HydraIdleWaitTest('bar')
    general.idle_wait(2.0)
    general.idle_wait_frames(1)


if __name__ == "__main__":
    run()
