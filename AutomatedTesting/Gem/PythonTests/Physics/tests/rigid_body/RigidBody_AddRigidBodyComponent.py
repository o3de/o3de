"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C4976194
# Test Case Title : Verify that you can add PhysX Rigid Bodies Physics component to an Entity without any warning or Error.



# fmt: off
class Tests():
    enter_game_mode             = ("Entered game mode",          "Failed to enter game mode")
    find_cube                   = ("Entity Cube found",          "Cube not found")
    linear_damp                 = ("Cube Linear Damping equal",  "Cube Linear Damping not equal")
    angular_damp                = ("Cube Angular Damping equal", "Cube Angular Damping not equal")
    mass                        = ("Cube Mass equal",            "Cube Mass not equal")
    exit_game_mode              = ("Exited game mode",           "Couldn't exit game mode")
# fmt: on


def RigidBody_AddRigidBodyComponent():

    """
    Summary:
    Open a Project that already has a PhysxRigidBody component in it and verify PhysxRigidBody is working in Game Mode

    Level Description:
    PhysxRigidBody (entity) -  PhysxRigidBody entity is created in the level

    Expected Behavior:
    The PhysxRigidBody entity should be working in the game mode 

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve and validate entity
     4) Validate Linear damping, Angular Damping and Mass of PhysxRigidBody
     5) Set a new values for Linear damping, Angular Damping and Mass of PhysxRigidBody and validate it
     6) Exit game mode
     7) Close the editor


    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os, sys



    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus

    # Constants
    DEFAULT_LINEAR_DAMPING = 0.05
    DEFAULT_ANGULAR_DAMPING = 0.15
    DEFAULT_MASS = 1.0
    NEW_LINEAR_DAMPING = 1.05
    NEW_ANGULAR_DAMPING = 1.15
    NEW_MASS = 2.0
    CLOSE_ENOUGH = 0.001

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "RigidBody_AddRigidBodyComponent")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate Entity
    cube_id = general.find_game_entity("CubeRigidBody")
    Report.critical_result(Tests.find_cube, cube_id.IsValid())

    # 4) Validate Linear damping, Angular Damping and Mass of PhysxRigidBody
    cube_linear_damping = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearDamping", cube_id)
    Report.result(Tests.linear_damp, abs(cube_linear_damping - DEFAULT_LINEAR_DAMPING) < CLOSE_ENOUGH)
    cube_angular_damping = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetAngularDamping", cube_id)
    Report.result(Tests.angular_damp, abs(cube_angular_damping - DEFAULT_ANGULAR_DAMPING) < CLOSE_ENOUGH)
    cube_mass = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetMass", cube_id)
    Report.result(Tests.mass, abs(cube_mass - DEFAULT_MASS) < CLOSE_ENOUGH)

    # 5) Set a new values for Linear damping, Angular Damping and Mass of PhysxRigidBody and validate it
    azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetLinearDamping", cube_id, NEW_LINEAR_DAMPING)
    cube_linear_damping = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearDamping", cube_id)
    Report.result(Tests.linear_damp, abs(cube_linear_damping - NEW_LINEAR_DAMPING) < CLOSE_ENOUGH)
    azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetAngularDamping", cube_id, NEW_ANGULAR_DAMPING)
    cube_angular_damping = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetAngularDamping", cube_id)
    Report.result(Tests.angular_damp, abs(cube_angular_damping - NEW_ANGULAR_DAMPING) < CLOSE_ENOUGH)
    azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetMass", cube_id, NEW_MASS)
    cube_mass = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetMass", cube_id)
    Report.result(Tests.mass, abs(cube_mass - NEW_MASS) < CLOSE_ENOUGH)

    # 6) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RigidBody_AddRigidBodyComponent)
