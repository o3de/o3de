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

def Editor_UtilityCommandsLegacy_Works():
    # Description: 
    # Tests the Python API from PythonEditorFuncs.cpp while the Editor is running

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general
    import azlmbr.entity as entity
    import azlmbr.legacy.settings as settings

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="TestDependenciesLevel")
    azlmbr.legacy.general.idle_wait_frames(1)

    def testing_cvar(setMethod, methodName, label, value, compare):
        try:
            setMethod(label, value)
            test_value = azlmbr.legacy.general.get_cvar(label)
            if(compare(test_value, value)):
                print('{} worked'.format(methodName))
        except:
                check_result(f'{methodName} failed', False)


    def testing_axis_constraints(constraint):

        azlmbr.legacy.general.set_axis_constraint(constraint)

        if (azlmbr.legacy.general.get_axis_constraint(constraint)):
            return True

        check_result(f'Failed to test axis {constraint}', False)
        return False


    # ----- Test cvar

    compare = lambda lhs, rhs: rhs == float(lhs)
    testing_cvar(azlmbr.legacy.general.set_cvar_float, 'set_cvar_float', 'sv_DedicatedCPUVariance', 10.1, compare)

    compare = lambda lhs, rhs: rhs == lhs
    testing_cvar(azlmbr.legacy.general.set_cvar_string, 'set_cvar_string', 'g_TemporaryLevelName', 'jpg', compare)

    compare = lambda lhs, rhs: rhs == int(lhs)
    testing_cvar(azlmbr.legacy.general.set_cvar_integer, 'set_cvar_integer', 'ed_backgroundUpdatePeriod', 33, compare)

    # ----- Test Axis Constraints

    if (testing_axis_constraints("X") and testing_axis_constraints("Y") and
        testing_axis_constraints("Z") and testing_axis_constraints("XY") and
        testing_axis_constraints("XZ") and testing_axis_constraints("YZ") and
        testing_axis_constraints("XYZ") and testing_axis_constraints("TERRAIN") and
        testing_axis_constraints("TERRAINSNAP")):

        print("axis constraint works")

    # all tests worked
    Report.result("Editor_UtilityCommandsLegacy_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_UtilityCommandsLegacy_Works)
