"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
class BaseClass():
    # Description: 
    # Helper class to encapsulate testing logic

    @staticmethod
    def check_result(result, msg):
        from editor_python_test_tools.utils import Report
        if not result:
            Report.result(msg, False)
            raise Exception(msg + " : FAILED")

    def test_case(self, callback, level='Base'):
        from editor_python_test_tools.utils import Report
        from editor_python_test_tools.utils import TestHelper
        import azlmbr.legacy.general

        # required for automated tests
        TestHelper.init_idle()

        # open the test level
        TestHelper.open_level(directory="", level=level)
        azlmbr.legacy.general.idle_wait_frames(1)

        # run the test case callback
        Report.start_test(callback)

        # all tests worked
        Report.result(f"{callback} all tests worked", True)
