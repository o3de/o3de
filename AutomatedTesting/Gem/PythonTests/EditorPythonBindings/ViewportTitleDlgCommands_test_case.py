"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests the ViewportTitleDlg Python API from ViewportTitleDlg.cpp while the Editor is running

import azlmbr.math
import azlmbr.legacy.general as general

# Open a level (any level should work)
general.open_level_no_prompt('Base')
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
