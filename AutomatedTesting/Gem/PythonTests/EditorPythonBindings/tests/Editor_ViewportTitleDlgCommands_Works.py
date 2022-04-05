"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_ViewportTitleDlgCommands_Works(BaseClass):
    # Description: 
    # Tests the ViewportTitleDlg Python API from ViewportTitleDlg.cpp while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.math
        import azlmbr.legacy.general as general

        # Get the current state of our helpers toggle.  
        initial_state = general.is_helpers_shown()

        # Toggle it once and verify it changed
        general.toggle_helpers() 
        BaseClass.check_result(initial_state != general.is_helpers_shown(), "Toggle it once and verify it changed")

        # Toggle it a second time and verify it changed back to the original setting.
        # Note:  We specifically choose an even number of times so we leave the Editor
        # in the same state as we found it. :)
        general.toggle_helpers()
        BaseClass.check_result(initial_state == general.is_helpers_shown(), "is_helpers_shown + toggle_helpers")

if __name__ == "__main__":
    tester = Editor_ViewportTitleDlgCommands_Works()
    tester.test_case(tester.test)
