"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests the Python API from PythonEditorFuncs.cpp while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.legacy.general as general
import azlmbr.globals
import math

def testing_cvar(setMethod, methodName, label, value, compare):
    try:
        setMethod(label, value)
        test_value = general.get_cvar(label)
        if(compare(test_value, value)):
            print('{} worked'.format(methodName))
    except:
            print('{} failed'.format(methodName))


def testing_axis_constraints(constraint):

    general.set_axis_constraint(constraint)

    if (general.get_axis_constraint(constraint)):
        return True

    return False


# ----- Test cvar

compare = lambda lhs, rhs: rhs == float(lhs)
testing_cvar(general.set_cvar_float, 'set_cvar_float', 'sys_LocalMemoryOuterViewDistance', 501.0, compare)

compare = lambda lhs, rhs: rhs == lhs
testing_cvar(general.set_cvar_string, 'set_cvar_string', 'e_ScreenShotFileFormat', 'jpg', compare)

compare = lambda lhs, rhs: rhs == int(lhs)
testing_cvar(general.set_cvar_integer, 'set_cvar_integer', 'sys_LocalMemoryGeometryLimit', 33, compare)


# ----- Test Axis Constraints

if (testing_axis_constraints("X") and testing_axis_constraints("Y") and
    testing_axis_constraints("Z") and testing_axis_constraints("XY") and
    testing_axis_constraints("XZ") and testing_axis_constraints("YZ") and
    testing_axis_constraints("XYZ") and testing_axis_constraints("TERRAIN") and
    testing_axis_constraints("TERRAINSNAP")):

    print("axis constraint works")

print("end of editor utility tests")

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
