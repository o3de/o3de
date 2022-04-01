"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def check_result(msg, result):
    from editor_python_test_tools.utils import Report
    if not result:
        Report.result(msg, False)
        raise Exception(msg + " : FAILED")

def Editor_ViewCommands_Works():
    # Description: 
    # Tests a portion of the Editor View Python API from CryEdit.cpp while the Editor is running

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.math
    import azlmbr.legacy.general as general

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="TestDependenciesLevel")
    azlmbr.legacy.general.idle_wait_frames(1)

    def fetch_vector3_parts(vec3):
        x = vec3.get_property('x')
        y = vec3.get_property('y')
        z = vec3.get_property('z')
        return (x, y, z)

    def compare_vec3(lhs, rhs):
        xClose = azlmbr.math.Math_IsClose(lhs[0], rhs[0], 0.001)
        yClose = azlmbr.math.Math_IsClose(lhs[1], rhs[1], 0.001)
        zClose = azlmbr.math.Math_IsClose(lhs[2], rhs[2], 0.001)
        return xClose and yClose and zClose

    pos = general.get_current_view_position()
    rot = general.get_current_view_rotation()

    px, py, pz = fetch_vector3_parts(pos)
    rx, ry, rz = fetch_vector3_parts(rot)

    px = px + 5.0
    py = py - 2.0
    pz = pz + 1.0

    rx = rx + 2.0
    ry = ry + 3.0
    rz = rz - 5.0

    general.set_current_view_position(px, py, pz)
    general.set_current_view_rotation(rx, ry, rz)

    pos2 = general.get_current_view_position()
    rot2 = general.get_current_view_rotation()

    p2x, p2y, p2z = fetch_vector3_parts(pos2)
    r2x, r2y, r2z = fetch_vector3_parts(rot2)

    check_result(f'set_current_view_position', compare_vec3( (px,py,pz), (p2x,p2y,p2z) ))
    check_result(f'set_current_view_rotation {rx},{ry},{rz} {r2x},{r2y},{r2z}', compare_vec3( (rx,ry,rz), (r2x,r2y,r2z) ))

    # all tests worked
    Report.result("Editor_ViewCommands_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_ViewCommands_Works)
