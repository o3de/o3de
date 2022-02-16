"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from Performance.utils.perf_timer import time_editor_level_loading

def EditorLevelLoading_10KEntityCpuPerfTest():
    time_editor_level_loading('Performance', '10KEntityCpuPerfTest')

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(EditorLevelLoading_10KEntityCpuPerfTest)
