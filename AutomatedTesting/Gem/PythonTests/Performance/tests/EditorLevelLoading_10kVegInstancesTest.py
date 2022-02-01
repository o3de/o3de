"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

def EditorLevelLoading_10kVegInstancesTest():
    from EditorLevelLoading_base import time_editor_level_loading

    time_editor_level_loading('Performance', '10kVegInstancesTest')

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(EditorLevelLoading_10kVegInstancesTest)
