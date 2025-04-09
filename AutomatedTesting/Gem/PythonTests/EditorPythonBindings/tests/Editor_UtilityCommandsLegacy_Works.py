"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_UtilityCommandsLegacy_Works(BaseClass):
    # Description: 
    # Tests the Python API from PythonEditorFuncs.cpp while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.legacy.general
        import azlmbr.entity as entity
        import azlmbr.legacy.settings as settings

        def testing_cvar_legacy(setMethod, methodName, label, value, compare):
            setMethod(label, value)
            test_value = azlmbr.legacy.general.get_cvar(label)
            BaseClass.check_result(compare(test_value, value), f'{methodName} operation')

        def testing_axis_constraints_legacy(constraint):
            azlmbr.legacy.general.set_axis_constraint(constraint)
            result = azlmbr.legacy.general.get_axis_constraint(constraint)
            BaseClass.check_result(result, f'Test axis {constraint}')

        # ----- Test cvar

        compare = lambda lhs, rhs: rhs == float(lhs)
        testing_cvar_legacy(azlmbr.legacy.general.set_cvar_float, 'set_cvar_float', 'sv_DedicatedCPUVariance', 10.1, compare)

        compare = lambda lhs, rhs: rhs == lhs
        testing_cvar_legacy(azlmbr.legacy.general.set_cvar_string, 'set_cvar_string', 'g_TemporaryLevelName', 'jpg', compare)

        compare = lambda lhs, rhs: rhs == int(lhs)
        testing_cvar_legacy(azlmbr.legacy.general.set_cvar_integer, 'set_cvar_integer', 'ed_backgroundUpdatePeriod', 33, compare)

        # ----- Test Axis Constraints

        testing_axis_constraints_legacy("X") 
        testing_axis_constraints_legacy("Y")
        testing_axis_constraints_legacy("Z") 
        testing_axis_constraints_legacy("XY")
        testing_axis_constraints_legacy("XZ") 
        testing_axis_constraints_legacy("YZ")
        testing_axis_constraints_legacy("XYZ") 
        testing_axis_constraints_legacy("TERRAIN")
        testing_axis_constraints_legacy("TERRAINSNAP")

if __name__ == "__main__":
    tester = Editor_UtilityCommandsLegacy_Works()
    tester.test_case(tester.test)
