"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_ViewCommands_Works(BaseClass):
    # Description: 
    # Tests a portion of the Editor View Python API from CryEdit.cpp while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.math
        import azlmbr.legacy.general as general

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

        BaseClass.check_result(compare_vec3( (px,py,pz), (p2x,p2y,p2z) ), 'set_current_view_position')
        BaseClass.check_result(compare_vec3( (rx,ry,rz), (r2x,r2y,r2z) ), 'set_current_view_rotation')

if __name__ == "__main__":
    tester = Editor_ViewCommands_Works()
    tester.test_case(tester.test)
