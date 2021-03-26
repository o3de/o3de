"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# Tests the ViewportTitleDlg Python API from ViewportTitleDlg.cpp while the Editor is running

import azlmbr.math
import azlmbr.legacy.general as general

# Open a level (any level should work)
general.open_level_no_prompt('WaterSample')
general.idle_wait(0.5)

test_success = True

# Get the current state of our helpers toggle.  
initial_state = general.is_helpers_shown()

# Toggle it once and verify it changed
general.toggle_helpers()
test_success = test_success and (initial_state != general.is_helpers_shown())

# Toggle it a second time and verify it changed back to the original setting.
# Note:  We specifically choose an even number of times so we leave the Editor
# in the same state as we found it. :)
general.toggle_helpers()
test_success = test_success and (initial_state == general.is_helpers_shown())

if test_success:
    print('toggle_helpers works')
    print('is_helpers_shown works')

general.exit_no_prompt()
