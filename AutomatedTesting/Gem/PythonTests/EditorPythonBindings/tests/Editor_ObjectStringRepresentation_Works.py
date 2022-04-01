"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def check_result(result, msg):
    from editor_python_test_tools.utils import Report
    if not result:
        Report.result(msg, False)
        raise Exception(msg + " : FAILED")

def Editor_ObjectStringRepresentation_Works():
    # Description: 
    # Tests the PythonProxyObject __str__ and __repr__

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.entity as entity
    import azlmbr.object

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    test_id = entity.EntityId()
    test_id_repr = repr(test_id)

    check_result(test_id_repr.startswith('<EntityId via PythonProxyObject'),  "repr() returned expected value")

    test_id_str = str(test_id)
    test_id_to_string = test_id.ToString()
    check_result(test_id_str == test_id_to_string, f"str() returned {test_id_str} {test_id_to_string}")

    # all tests worked
    Report.result("Editor_ObjectStringRepresentation_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_ObjectStringRepresentation_Works)

