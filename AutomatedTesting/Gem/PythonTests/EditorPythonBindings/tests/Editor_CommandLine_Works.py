"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_CommandLine_Works(BaseClass):
    # Description: 
    # Tests the Python API from DisplaySettingsPythonFuncs.cpp while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.paths

        # make sure the @engroot@ exists as a azlmbr.paths property
        engroot = azlmbr.paths.engroot
        BaseClass.check_result(engroot is not None and len(engroot) is not 0, f'engroot is {engroot}')

        # resolving a basic path
        path = azlmbr.paths.resolve_path('@engroot@/engineassets/icons/averagememoryusage.tif.streamingimage')
        BaseClass.check_result(len(path) != 0 and path.find('@engroot@') == -1, f'path resolved to {path}')

if __name__ == "__main__":
    tester = Editor_CommandLine_Works()
    tester.test_case(tester.test)
