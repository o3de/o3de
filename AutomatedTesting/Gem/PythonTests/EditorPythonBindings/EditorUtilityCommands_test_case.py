"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# Tests the Python API from PythonEditorFuncs.cpp while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.python_editor_funcs as python_editor_funcs
import azlmbr.globals
import math


def testing_cvar(setMethod, methodName, label, value, compare):
    try:
        python_editor_funcs.PythonEditorBus(bus.Broadcast, setMethod, label, value)
        test_value = python_editor_funcs.PythonEditorBus(bus.Broadcast, 'GetCVar', label)
        if compare(test_value, value):
            print('{} worked'.format(methodName))
    except:
        print('{} failed'.format(methodName))


def testing_axis_constraints(constraint):
    python_editor_funcs.PythonEditorBus(bus.Broadcast, 'SetAxisConstraint', constraint)

    if constraint == python_editor_funcs.PythonEditorBus(bus.Broadcast, 'GetAxisConstraint'):
        return True

    return False


# ----- Test cvar

compare = lambda lhs, rhs: rhs == float(lhs)
testing_cvar('SetCVarFromFloat', 'SetCVarFromFloat', 'sys_LocalMemoryOuterViewDistance', 501.0, compare)

compare = lambda lhs, rhs: rhs == lhs
testing_cvar('SetCVarFromString', 'SetCVarFromString', 'e_ScreenShotFileFormat', 'jpg', compare)

compare = lambda lhs, rhs: rhs == int(lhs)
testing_cvar('SetCVarFromInteger', 'SetCVarFromInteger', 'sys_LocalMemoryGeometryLimit', 33, compare)

# ----- Test Axis Constraints

if (testing_axis_constraints("X") and testing_axis_constraints("Y") and 
    testing_axis_constraints("Z") and testing_axis_constraints("XY") and 
    testing_axis_constraints("XZ") and testing_axis_constraints("YZ") and 
    testing_axis_constraints("XYZ") and testing_axis_constraints("TERRAIN") and 
    testing_axis_constraints("TERRAINSNAP")):
    
    print("axis constraint works")

# ----- End

print("end of editor utility tests")

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
