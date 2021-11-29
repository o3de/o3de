"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5959760
# Test Case Title : Check that force region (capsule) exerts point force



# fmt: off
class Tests:
    enter_game_mode         = ("Entered game mode",                 "Failed to enter game mode")
    box_entity_found        = ("Box was found in game",             "Box COULD NOT be found in game")
    capsule_entity_found    = ("Capsule was found in game",         "Capsule COULD NOT be found in game")
    box_pos_found           = ("Box position found",                "Box position not found")
    capsule_pos_found       = ("Capsule position found",            "Capsule position not found")
    force_region_entered    = ("Force region entered",              "Force region never entered")
    force_exertion_predicted = ("Force exerted was predictable",    "The force exerted WAS NOT predicted")
    box_fell                = ("Box fell",                          "The box did not fall")
    box_was_pushed_x_z      = ("Box moved positive X, Z",           "Box DID NOT move in positive X, Z direction")
    box_no_y_movement       = ("Box had no substantial Y movement", "Box HAD substantial Y movement")
    capsule_no_move         = ("Capsule did not move",              "Capsule DID move")
    exit_game_mode          = ("Exited game mode",                  "Couldn't exit game mode")
    time_out                = ("Test did not time out",             "Test DID time out")

# fmt: on


def ForceRegion_CapsuleShapedForce():
    # type: () -> None
    """
    Summary:
    Runs an automated test to ensure point force from a capsule force region is exerted on rigid body objects.

    Level Description:
    A cube (entity: Box) set above a capsule force region (entity: Capsule). The Capsule was assigned point force
    with magnitude set to 1000. The Box has been set for "gravity enabled"

    Expected behavior:
    The Box will fall (due to gravity) into the Capsule's force region. The force region should exert the point
    force on the Box, applying a positive X and Z force of substantial magnitude.

    Test Steps:
    1) Loads the level / Enters game mode
    2) Retrieve entities
    3) Ensures that the test objects (Box and Capsule) are located
        3.5) set up variables and handlers for monitoring results
    4) Waits for the box to fall into the force region
            or for time out if something unexpected happens
    5) Logs results
    6) Closes the editor

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

    # Global constants
    CLOSE_ENOUGH = 0.001
    TIME_OUT = 1.5

    # Base class
    class EntityBase:
        def __init__(self, name):
            self.name = name
            self.id = None
            self.initial_pos = None
            self.current_pos = None

    # Box child class of EntityBase
    class Box(EntityBase):
        def __init__(self, name):
            EntityBase.__init__(self, name)
            self.triggered_pos = None
            self.fell = False
            self.force_observed = False

        def check_for_fall(self):
            FALL_BUFFER = 0.2
            if not self.fell:
                self.fell = (
                    self.initial_pos.z > self.current_pos.z + FALL_BUFFER
                    and abs(self.initial_pos.x - self.current_pos.x) < CLOSE_ENOUGH
                    and abs(self.initial_pos.y - self.current_pos.y) < CLOSE_ENOUGH
                )
            return self.fell

        def check_for_force(self):
            FORCE_BUFFER = 0.2
            if not self.force_observed:
                self.force_observed = (
                    self.current_pos.z > self.triggered_pos.z + FORCE_BUFFER
                    and self.current_pos.x > self.triggered_pos.x
                    and abs(self.triggered_pos.y - self.current_pos.y) < CLOSE_ENOUGH
                )
            return self.force_observed

    # Force Region child class of EntityBase
    class ForceRegion(EntityBase):
        def __init__(self, name):
            EntityBase.__init__(self, name)
            self.expected_force_magnitude = None
            self.actual_force_vector = None
            self.actual_force_magnitude = None
            self.forced_entity = None
            self.triggered = False

    # 1) Open level / Enter game mode
    helper.init_idle()
    helper.open_level("Physics", "ForceRegion_CapsuleShapedForce")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve entities
    box = Box("Box")
    box.id = general.find_game_entity(box.name)
    capsule = ForceRegion("Capsule")
    capsule.id = general.find_game_entity(capsule.name)

    Report.critical_result(Tests.box_entity_found, box.id.IsValid())
    Report.critical_result(Tests.capsule_entity_found, capsule.id.IsValid())

    # 3) Log positions for Box and Capsule
    box.initial_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", box.id)
    capsule.initial_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", capsule.id)
    box.current_pos = box.initial_pos
    capsule.current_pos = capsule.initial_pos

    # validate and print positions to confirm objects were found
    Report.critical_result(Tests.box_pos_found, box.initial_pos is not None and not box.initial_pos.IsZero())
    Report.critical_result(
        Tests.capsule_pos_found, capsule.initial_pos is not None and not capsule.initial_pos.IsZero()
    )
    capsule.expected_force_magnitude = azlmbr.physics.ForcePointRequestBus(azlmbr.bus.Event, "GetMagnitude", capsule.id)

    # 3.5) set up handler

    # Force Region Event Handler
    def on_force_calculated(args):

        # Only store data for first force region calculation
        if not capsule.triggered and capsule.id.Equal(args[0]):
            capsule.triggered = True
            capsule.forced_entity = args[1]
            capsule.actual_force_vector = args[2]
            capsule.actual_force_magnitude = args[3]
            if capsule.forced_entity.Equal(box.id):
                box.triggered_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", box.id)
                Report.info("Force Region exerted force on {}".format(box.name))

    # Assign the handler
    handler = azlmbr.physics.ForceRegionNotificationBusHandler()
    handler.connect(None)
    handler.add_callback("OnCalculateNetForce", on_force_calculated)

    def done_collecting_results():
        # Update entity positions
        capsule.current_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", capsule.id)
        box.current_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", box.id)
        # Check for three "test complete" conditions
        # ! Careful ordering for logic short circuiting. DO NOT SWAP ORDER !
        return box.check_for_fall() and capsule.triggered and box.check_for_force()

    # 4) wait for force region entry or time out
    test_completed = helper.wait_for_condition(done_collecting_results, TIME_OUT)
    Report.critical_result(Tests.time_out, test_completed)

    # 5) Report findings
    Report.result(Tests.box_fell, box.fell)
    Report.result(Tests.force_region_entered, capsule.triggered)
    Report.result(
        Tests.force_exertion_predicted,
        abs(capsule.expected_force_magnitude - capsule.actual_force_magnitude) < CLOSE_ENOUGH,
    )
    Report.result(Tests.box_was_pushed_x_z, box.force_observed)
    Report.result(Tests.box_no_y_movement, abs(box.initial_pos.y - box.current_pos.y) < CLOSE_ENOUGH)
    Report.result(Tests.capsule_no_move, capsule.initial_pos.IsClose(capsule.current_pos))

    # Collected Data Dump
    Report.info("******* Collected Data *******")
    Report.info("Entity: {}".format(box.name))
    Report.info_vector3(box.initial_pos, " Initial Position:")
    Report.info_vector3(box.triggered_pos, " Trigger Position:")
    Report.info_vector3(box.current_pos, " Final Position:")
    Report.info(" Fell: {}".format(box.fell))
    Report.info(" Force Observed: {}".format(box.force_observed))
    Report.info("******************************")
    Report.info("Entity: {}".format(capsule.name))
    Report.info_vector3(capsule.initial_pos, " Initial Position:")
    Report.info_vector3(capsule.current_pos, " Final Position:")
    Report.info(" Expected Force Magnitude: {:.2f}".format(capsule.expected_force_magnitude))
    Report.info_vector3(capsule.actual_force_vector, " Actual Force Vector:", capsule.actual_force_magnitude)
    Report.info(" Triggered: {}".format(capsule.triggered))
    Report.info(
        " Triggered Entity: {}".format(
            azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "GetEntityName", capsule.forced_entity)
        )
    )

    Report.info("******************************")

    # 6) Exit Game mode
    helper.exit_game_mode(Tests.exit_game_mode)
    Report.info("*** FINISHED TEST ***")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_CapsuleShapedForce)
