"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import time

def EditorTest_That_PassesToo():
    pass

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(EditorTest_That_PassesToo)
