"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C12712455
# Test Case Title : Verify shape cast nodes in SC


# fmt: off
class Tests:
    enter_game_mode                = ("Entered game mode",                      "Failed to enter game mode")
    Ball_0_found                   = ("Ball_0 entity is found",                 "Ball_0 entity is not found")
    Ball_1_found                   = ("Ball_1 entity is found",                 "Ball_1 entity is not found")
    Notification_Entity_0_found    = ("Notification_Entity_0 entity found",     "Notification_Entity_0 entity invalid")
    Notification_Entity_1_found    = ("Notification_Entity_1 entity found",     "Notification_Entity_1 entity invalid")
    Notification_Entity_2_found    = ("Notification_Entity_2 entity found",     "Notification_Entity_2 entity invalid")
    Ball_0_gravity                 = ("Ball_0 gravity is disabled",             "Ball_0 gravity is enabled")
    Ball_1_gravity                 = ("Ball_1 gravity is disabled",             "Ball_1 gravity is enabled")
    Notification_Entity_0_gravity  = ("Notification_Entity_0 gravity disabled", "Notification_Entity_0 gravity enabled")
    Notification_Entity_1_gravity  = ("Notification_Entity_1 gravity disabled", "Notification_Entity_1 gravity enabled")
    Notification_Entity_2_gravity  = ("Notification_Entity_2 gravity disabled", "Notification_Entity_2 gravity enabled")
    script_canvas_translation_node = ("Script canvas through translation node", "Script canvas failed translation node")
    script_canvas_sphere_cast_node = ("Script canvas through sphere cast node", "Script canvas failed sphere cast node")
    script_canvas_draw_sphere_node = ("Script canvas through draw sphere node", "Script canvas failed draw sphere node")
    exit_game_mode                 = ("Exited game mode",                       "Couldn't exit game mode")
# fmt: on


def ScriptCanvas_ShapeCast():
    """
    Summary: Verifies that shape cast nodes in script editor function properly.

    Level Description:
    Ball_0 - Starts stationary with gravity disabled on -y axis from Ball_1; has Sphere shape collider, Rigid Body,
        Sphere Shape, Script Canvas
    Ball_1 - Starts stationary with gravity disabled on +y axis from Ball_0; has Sphere shape collider, Rigid Body,
        Sphere Shape
    Notification_Entity_0 - Starts stationary with gravity disabled; has rigid body, Sphere shape
    Notification_Entity_1 - Starts stationary with gravity disabled; has rigid body, Sphere shape
    Notification_Entity_2 - Starts stationary with gravity disabled; has rigid body, Sphere shape
    Script Canvas - Creates a sphere cast from Ball_0 to Ball_1 and draws the sphere in. As the script progresses
        through the translation, sphere cast, and sphere draw nodes it enables gravity on the spheres in order to
        show that each node has been passed.

    Expected Behavior: A sphere will be drawn on ball_1 in from ball_0. The three notification_entities will fall as
        gravity is enabled on them in order.

    Test Steps:
    1) Open Level
    2) Enter Game Mode
    3) Create entity objects
    4) Verify entities and check gravity
    5) Wait for conditions or timeout
    6) Log results
    7) Exit Game Mode
    8) Close Editor

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

    # Helper Functions
    class Entity:
        # Constants
        GRAVITY_WAIT_TIMEOUT = 1

        def __init__(self, name, expected_initial_velocity=None, expected_final_velocity=None):
            self.id = general.find_game_entity(name)
            self.name = name
            self.gravity_was_enabled = False
            self.handler = None
            # 4) Verify entities and check gravity
            try:
                self.found_test = Tests.__dict__[self.name + "_found"]
                self.gravity_disabled_test = Tests.__dict__[self.name + "_gravity"]
            except Exception as e:
                Report.info("Can not find specified tuples in Tests class")
                Report.info(e)
                raise ValueError
            Report.critical_result(self.found_test, self.id.isValid())
            gravity_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)
            Report.critical_result(self.gravity_disabled_test, not gravity_enabled)

        def check_gravity_enabled(self):
            self.gravity_was_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)
            return self.gravity_was_enabled

        def wait_for_gravity_enabled(self):
            helper.wait_for_condition(self.check_gravity_enabled, Entity.GRAVITY_WAIT_TIMEOUT)

    # Main Script
    helper.init_idle()
    # 1) Open Level
    helper.open_level("Physics", "ScriptCanvas_ShapeCast")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create entity objects
    ball_0 = Entity("Ball_0")
    ball_1 = Entity("Ball_1")
    notification_entity_0 = Entity("Notification_Entity_0")
    notification_entity_1 = Entity("Notification_Entity_1")
    notification_entity_2 = Entity("Notification_Entity_2")

    # 5) Wait for conditions or timeout
    notification_list = [notification_entity_0, notification_entity_1, notification_entity_2]
    for entity in notification_list:
        entity.wait_for_gravity_enabled()

    # 6) Log results
    Report.result(Tests.script_canvas_translation_node, notification_entity_0.gravity_was_enabled)
    Report.result(Tests.script_canvas_sphere_cast_node, notification_entity_1.gravity_was_enabled)
    Report.result(Tests.script_canvas_draw_sphere_node, notification_entity_2.gravity_was_enabled)

    # 7) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ScriptCanvas_ShapeCast)
