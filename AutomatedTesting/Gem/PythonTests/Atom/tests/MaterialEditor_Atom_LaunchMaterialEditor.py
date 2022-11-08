"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    material_editor_launched = (
        "Inspector pane is visible, MaterialEditor launch succeeded",
        "P0: Inspector pane not visible, MaterialEditor launch failed"
    )


def MaterialEditor_Launched_SuccessfullyLaunched():
    """
    Summary:
    Tests that the MaterialEditor can be launched successfully.

    Expected Behavior:
    The MaterialEditor executable can be launched and doesn't cause any crashes or errors.

    Test Steps:
    1) Verify Material Inspector pane visibility to confirm the MaterialEditor launched.
    2) Look for errors and asserts.

    :return: None
    """

    import Atom.atom_utils.atom_tools_utils as atom_tools_utils

    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    with Tracer() as error_tracer:
        # 1. Verify Material Inspector pane visibility to confirm the MaterialEditor launched.
        atom_tools_utils.set_pane_visibility("Inspector", True)
        Report.result(
            Tests.material_editor_launched,
            atom_tools_utils.is_pane_visible("Inspector") is True)

        # 2. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MaterialEditor_Launched_SuccessfullyLaunched)
