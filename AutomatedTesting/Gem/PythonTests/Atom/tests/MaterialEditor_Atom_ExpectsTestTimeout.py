"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    material_editor_test_timed_out = (
        "Inspector pane is not visible, MaterialEditor test timed out as expected",
        "P0: Inspector pane is visible, MaterialEditor passed when it should have failed from time out"
    )


def MaterialEditor_TestsTimeout_TimeoutFailure():
    """
    Summary:
    Tests that the MaterialEditor can fail from timed out tests.

    Expected Behavior:
    The MaterialEditor test fails due to time out.

    Test Steps:
    1) Verify Material Inspector pane visibility to confirm the MaterialEditor launched.
    2) Look for errors and asserts.

    :return: None
    """
    import Atom.atom_utils.atom_tools_utils as atom_tools_utils

    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    with Tracer() as error_tracer:
        # 1. Set MaterialEditor Inspector pane visibility to False.
        atom_tools_utils.set_pane_visibility("Inspector", False)

        # 2. Use wait_for_condition() to cause a timeout failure.
        atom_tools_utils.wait_for_condition(lambda: atom_tools_utils.is_pane_visible("Inspector"), 1)

        # 3. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MaterialEditor_TestsTimeout_TimeoutFailure)
