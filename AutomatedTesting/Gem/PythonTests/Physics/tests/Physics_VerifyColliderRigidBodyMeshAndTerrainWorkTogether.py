"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5689529
# Test Case Title : Create an entity with PhysX Terrain component and add
#                   PhysX Rigid Body PhysX, PhysX Collider and Rendering Mesh to it and
#                   verify that it works in game mode



# fmt: off
class Tests():
    enter_game_mode = ("Entered game mode",                             "Failed to enter game mode")
    find_entity     = ("Entity found",                                  "Entity not found")
    gravity_enabled = ("Gravity is enabled",                            "Gravity is disabled") 
    mass_equal      = ("Mass of rigid body equal to the expected mass", "Mass of rigid body not equal to the expected mass")
    exit_game_mode  = ("Exited game mode",                              "Couldn't exit game mode")
# fmt: on


def Physics_VerifyColliderRigidBodyMeshAndTerrainWorkTogether():

    """
    Summary:
    Create an entity with PhysX Terrain component and add PhysX Rigid Body PhysX,
    PhysX Collider and Rendering Mesh to it and verify that it works in game mode

    Level Description:
    PhysXRigidBody (entity) - Entity with components PhysX Rigid Body, PhysX Collider, Terrain and Rendering Mesh

    Expected Behavior:
    The rigid body entity should be working in the game mode.
    We are checking if entity id is valid and extracting some properties of rigid body.
    Also we are waiting for few frames to ensure that the editor did not crash.

    Test Steps:
     1) Open level
     2) Enter game mode
     3) Retrieve and validate entities
     4) Waiting for WAIT_FRAMES to ensure that the editor did not crash in the game mode
     5) Check the properties of rigid body like Gravity(enabled) and Mass(1.0kg)
     6) Exit game mode
     7) Close the editor


    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus

    # Constants
    WAIT_FRAMES = 2
    EXPECTED_MASS = 1.0  # Default mass of a rigid body component

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "Physics_VerifyColliderRigidBodyMeshAndTerrainWorkTogether")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Retrieve and validate entities
    rigid_body_id = general.find_game_entity("PhysXRigidBody")
    Report.critical_result(Tests.find_entity, rigid_body_id.IsValid())

    class RigidBody:
        gravity_enabled = False
        collison_occued = False
        mass = 0.0

    # 4) Waiting for WAIT_FRAMES to ensure that the editor did not crash in the game mode
    general.idle_wait_frames(WAIT_FRAMES)
    Report.info("Editor did not crash after waiting for " + str(WAIT_FRAMES) + " frames")

    # 5) Check the properties of rigid body like Gravity(enabled) and Mass(1.0kg)
    RigidBody.gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", rigid_body_id)
    Report.critical_result(Tests.gravity_enabled, RigidBody.gravity_enabled)
    RigidBody.mass = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetMass", rigid_body_id)
    Report.critical_result(Tests.mass_equal, RigidBody.mass == EXPECTED_MASS)

    # 6) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Physics_VerifyColliderRigidBodyMeshAndTerrainWorkTogether)
