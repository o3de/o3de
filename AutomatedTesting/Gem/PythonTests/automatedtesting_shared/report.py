#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# These methods are useful for testing purposes as they output different patterns of
# lines that would appear in the console or various log files. These lines can then be
# picked up via console/log monitoring or log parsing for automated/manual testing

import azlmbr.legacy.general as general

class Report:
    @staticmethod
    def info(msg: str) -> None:
        print(f"Info: {msg}")

    @staticmethod
    def success(success_message: str) -> None:
        print(f"Success: {success_message}")

    @staticmethod
    def failure() -> None:
        print("Failure: A pass condition has not passed in this test")

    @staticmethod
    def result(success_message: str, condition: bool) -> None:
        """
        This would be used for expecting a True output on some condition
        Example: 
            level_loaded = EditorTestHelper.open_level("Example")
            Report.result("Example level loaded successfully", level_loaded)
        """
        if not isinstance(condition, bool):
            raise TypeError("condition argument must be a bool")

        if condition:
            Report.success(success_message)
        else:
            Report.failure()
        return condition