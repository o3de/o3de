"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_UtilityCommands_Works(BaseClass):
    # Description: 
    # Tests the Python API from PythonEditorFuncs.cpp while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.legacy.general
        import azlmbr.entity as entity
        import azlmbr.python_editor_funcs as python_editor_funcs

        def testing_cvar(setMethod, methodName, label, value, compare):
            python_editor_funcs.PythonEditorBus(bus.Broadcast, setMethod, label, value)
            test_value = python_editor_funcs.PythonEditorBus(bus.Broadcast, 'GetCVar', label)
            BaseClass.check_result(compare(test_value, value), f'{methodName} operation')

        def testing_axis_constraints(constraint):
            python_editor_funcs.PythonEditorBus(bus.Broadcast, 'SetAxisConstraint', constraint)
            constraint == python_editor_funcs.PythonEditorBus(bus.Broadcast, 'GetAxisConstraint')
            BaseClass.check_result(constraint, f'Test axis {constraint}')

        # ----- Test cvar

        compare = lambda lhs, rhs: rhs == float(lhs)
        testing_cvar('SetCVarFromFloat', 'SetCVarFromFloat', 'sv_DedicatedCPUVariance', 501.0, compare)

        compare = lambda lhs, rhs: rhs == lhs
        testing_cvar('SetCVarFromString', 'SetCVarFromString', 'g_TemporaryLevelName', 'jpg', compare)

        compare = lambda lhs, rhs: rhs == int(lhs)
        testing_cvar('SetCVarFromInteger', 'SetCVarFromInteger', 'ed_backgroundUpdatePeriod', 33, compare)

        # ----- Test Axis Constraints

        testing_axis_constraints("X") 
        testing_axis_constraints("Y")
        testing_axis_constraints("Z") 
        testing_axis_constraints("XY")
        testing_axis_constraints("XZ") 
        testing_axis_constraints("YZ")
        testing_axis_constraints("XYZ") 
        testing_axis_constraints("TERRAIN")
        testing_axis_constraints("TERRAINSNAP")

if __name__ == "__main__":
    tester = Editor_UtilityCommands_Works()
    tester.test_case(tester.test, level="TestDependenciesLevel")
