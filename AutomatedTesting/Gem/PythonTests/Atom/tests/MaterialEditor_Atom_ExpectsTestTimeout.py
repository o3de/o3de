"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    material_editor_test_timed_out = (  # This test result is pytest.mark.xfail.
        "AtomToolsSingleTest class didn't time out and passed (it should fail from timing out).",
        "P0: AtomToolsSingleTest timed out before the test could complete (expected failure)."
    )


def MaterialEditor_TestsTimeout_TimeoutFailure():
    """
    Summary:
    Tests that the MaterialEditor can fail from timed out tests on the AtomToolsSingleTest class objects.

    Expected Behavior:
    The MaterialEditor test fails due to time out from the timeout value in AtomToolsSingleTest class.

    Test Steps:
    1) Delay the start of the auto-success check so that timeout values in AtomToolsSingleTest take hold.
    2) Create callback function to wait for the delay so that the test times out before success is reached.
    3) Auto-pass the test now since it should time out before this point.
    4) Look for errors and asserts.

    :return: None
    """
    import time

    import Atom.atom_utils.atom_tools_utils as atom_tools_utils
    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    with Tracer() as error_tracer:
        # 1. Delay the start of the auto-success check so that timeout values in AtomToolsSingleTest take hold.
        start_time = time.time() + 12

        # 2. Create callback function to wait for the delay so that the test times out before success is reached.
        def time_check():
            if time.time() > start_time:
                return True
            return False
        atom_tools_utils.wait_for_condition(time_check(), 15)

        # 3. Auto-pass the test now since it should time out before this point.
        Report.success(Tests.material_editor_test_timed_out)

        # 4. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MaterialEditor_TestsTimeout_TimeoutFailure)
