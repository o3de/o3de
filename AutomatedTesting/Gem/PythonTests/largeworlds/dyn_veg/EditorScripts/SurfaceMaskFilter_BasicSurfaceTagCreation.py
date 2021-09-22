"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    tags_same_value_equal = (
        "Two Surface Tags of the same value evaluated as equal",
        "Two Surface Tags of the same value unexpectedly evaluated as unequal"
    )
    tags_different_value_unequal = (
        "Two Surface Tags of different values evaluated as unequal",
        "Two Surface Tags of different values unexpectedly evaluated as equal"
    )


def SurfaceMaskFilter_BasicSurfaceTagCreation():
    """
    Summary:
    Verifies basic surface tag value equality

    Expected Behavior:
    Surface tags of the same name are equal, and different names aren't.

    Test Steps:
     1) Open level
     2) Create 2 new surface tags of identical names and verify they resolve as equal.
     3) Create another new tag of a different name and verify they resolve as different.

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import azlmbr.surface_data as surface_data
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    tag1 = surface_data.SurfaceTag()
    tag2 = surface_data.SurfaceTag()

    # Test 1:  Verify that two tags with the same value are equal
    tag1.SetTag('equal_test')
    tag2.SetTag('equal_test')
    Report.result(Tests.tags_same_value_equal, tag1.Equal(tag2))

    # Test 2:  Verify that two tags with different values are not equal
    tag2.SetTag('not_equal_test')
    Report.result(Tests.tags_different_value_unequal, not tag1.Equal(tag2))


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(SurfaceMaskFilter_BasicSurfaceTagCreation)
