"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_MainWindowCommands_Works(BaseClass):
    # Description: 
    # Tests the Python API from MainWindow.cpp while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.legacy.general as general

        # Get all pane names
        panes = general.get_pane_class_names()

        BaseClass.check_result(len(panes) > 0, f'get_pane_class_names worked of {panes}')

        # Get any element from the panes list
        test_pane = panes[0]

        general.open_pane(test_pane)

        BaseClass.check_result(general.is_pane_visible(test_pane), f'open_pane worked with {test_pane}')

        general.close_pane(test_pane)

        BaseClass.check_result(not(general.is_pane_visible(test_pane)), f'close_pane worked with {test_pane}')

if __name__ == "__main__":
    tester = Editor_MainWindowCommands_Works()
    tester.test_case(tester.test)
