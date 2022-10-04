"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import time


def MaterialEditorTest_That_TimesOut():
    try:
        time.sleep(20)
    except Exception:  # purposefully broad
        raise Exception('MaterialEditorTest_That_TimesOut raised exception instead of timing out.')


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MaterialEditorTest_That_TimesOut)
