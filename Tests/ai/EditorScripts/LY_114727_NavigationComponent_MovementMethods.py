#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
'''
This script tests movement methods Transform, Physics and Custom
'''
import sys, os
import time
import azlmbr.legacy.general as general

from tests_common import TestHelper

class TestMovementMethods(TestHelper):
    def __init__(self):
         TestHelper.__init__(self, log_prefix = 'LY-114727', args=['level'])

    def run_test(self):
        # Start by assuming we'll crash and fail
        self.test_success = False
        # Open the level non-interactively
        level_opened = self.open_level(self.get_arg('level'))
        if not level_opened:
            return

        # Enter game mode, so that physics in the test level starts running.
        general.enter_game_mode()
        # Wait for game mode to start.  (Not sure if this is necessary, just being extra-cautious)
        while (general.is_in_game_mode() != True):
            general.idle_wait(1.0)
        # Wait for game mode to finish.  Entities should be navigating 
        while (general.is_in_game_mode() == True):
            general.idle_wait(2.0)

        # We finished and haven't crashed, so assume success and exit the Editor.
        self.test_success = True

test = TestMovementMethods()
test.run()

