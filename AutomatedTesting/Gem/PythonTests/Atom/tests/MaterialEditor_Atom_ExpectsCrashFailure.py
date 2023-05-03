"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    material_editor_test_crashed = (  # This test result is pytest.mark.xfail.
        "P0: AtomToolsSingleTest class didn't crash and passed (it should fail from crashing).",
        "P0: AtomToolsSingleTest crashed before the test could complete (expected failure)."
    )


def MaterialEditor_TestsTimeoutCrashFailure():
    """
    Summary:
    Tests that the MaterialEditor can fail from crashing in a test.

    Expected Behavior:
    The MaterialEditor test fails due to crash from calling azlmbr.atomtools.crash()

    Test Steps:
    1) Start the MaterialEditor then force a crash and verify it fails through pytext.mark.xfail result.

    :return: None
    """

    import Atom.atom_utils.atom_tools_utils as atom_tools_utils
    from editor_python_test_tools.utils import Report, TestHelper

    # 1. Start the MaterialEditor then force a crash and verify it fails through pytext.mark.xfail result.
    Report.result(Tests.material_editor_test_crashed, atom_tools_utils.crash())


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MaterialEditor_TestsTimeoutCrashFailure)
