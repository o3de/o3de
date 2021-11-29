# coding=utf-8
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C5959764
# Test Case Title : Check that rigid body (Capsule) gets impulse from force region


# fmt: off
class Tests:
    enter_game_mode       = ("Entered game mode",             "Failed to enter game mode")
    find_capsule          = ("Entity Capsule found",          "Capsule not found")
    find_force_region     = ("Force Region found",            "Force Region not found")
    capsule_gained_height = ("Capsule went up",               "Capsule didn't go up")
    force_region_success  = ("Force Region impulsed Capsule", "Force Region didn't impulse Capsule")
    exit_game_mode        = ("Exited game mode",              "Couldn't exit game mode")
    tests_completed       = ("Tests completed",               "Tests did not complete")

# fmt: on


def ForceRegion_ImpulsesCapsuleShapedRigidBody():
    """
    This run() function will open a a level and validate that a Capsule gets impulsed by a force region.
    It does this by:
      1) Open level
      2) Enters Game mode
      3) Finds the entities in the scene
      4) Gets the position of the Capsule
      5) Listens for Capsule to enter the force region
      6) Gets the vector and magnitude when Capsule is in force region
      7) Lets the Capsule travel up
      8) Gets new position of Capsule
      9) Validate the results
      10) Exits game mode and editor
    """
    # Setup path
    import os, sys



    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import azlmbr.legacy.general as general
    import azlmbr.bus

    class Capsule:
        id = None
        gained_height = False  # Did the Capsule gain height
        z_end_position = 0
        z_start_position = 0

    # Listen for Force Region events
    class RegionData:
        def __init__(self, ID):
            self.force_region_id = ID
            self.force_region_entered = False
            self.force_vector = None
            self.force_magnitude_on_capsule = 0  # Magnitude applied on Capsule
            self.force_region_magnitude = 0  # Magnitude value for the force region set in the editor
            self.force_region_magnitude_range = (
                0.01
            )  # Delta value allowed between magnitude force_magnitude_on_capsule and force_region_magnitude

        def force_region_in_range(self):
            return abs(self.force_magnitude_on_capsule - self.force_region_magnitude) < 0.01  # 0.01 for buffer room

    def ifVectorClose(vec1, vec2):
        return abs(vec1 - vec2) < 0.01  # 0.01 For buffer room for dif in the two vectors.

    helper.init_idle()

    # *****Variables*****
    TIME_OUT = 4.0  # Seconds
    UPWARD_Z_VECTOR = 1.0

    # Open level
    helper.open_level("Physics", "ForceRegion_ImpulsesCapsuleShapedRigidBody")

    # Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # Get Entities
    Capsule.id = general.find_game_entity("Capsule")
    Report.critical_result(Tests.find_capsule, Capsule.id.IsValid())

    RegionObject = RegionData(general.find_game_entity("Force Region"))
    Report.critical_result(Tests.find_force_region, RegionObject.force_region_id.IsValid())

    # Set values for Capsule and force region
    Capsule.z_start_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", Capsule.id)
    RegionObject.force_region_magnitude = azlmbr.physics.ForcePointRequestBus(
        azlmbr.bus.Event, "GetMagnitude", RegionObject.force_region_id
    )
    Report.info("Capsule start z position = {}".format(Capsule.z_start_position))

    # Force Region Event Handler
    def on_calculate_net_force(args):
        """
        Called when there is a collision in the level
        Args:
            args[0] - force region entity
            args[1] - entity entering
            args[2] - vector
            args[3] - magnitude
        """
        assert RegionObject.force_region_id.Equal(args[0])

        vect = args[2]
        mag = args[3]
        if not RegionObject.force_region_entered:
            RegionObject.force_region_entered = True
            RegionObject.force_vector = vect
            RegionObject.force_magnitude_on_capsule = mag
            Report.info("Force Region entered")

    # Assign the handler
    handler = azlmbr.physics.ForceRegionNotificationBusHandler()
    handler.connect(None)
    handler.add_callback("OnCalculateNetForce", on_calculate_net_force)

    # Give Capsule time to travel. Exit when Capsule is done moving or time runs out.
    def test_completed():
        Capsule.z_end_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", Capsule.id)
        Capsule.gained_height = Capsule.z_end_position > (Capsule.z_start_position + 0.5)  # 0.5 for buffer

        # Validate if Capsule gained height and entered force region
        if Capsule.gained_height and RegionObject.force_region_entered:
            Report.success(Tests.capsule_gained_height)
            return True
        return False

    # Wait for test to complete
    test_is_completed = helper.wait_for_condition(test_completed, TIME_OUT)

    if not test_is_completed:
        Report.info("The test timed out, check log for possible solutions or adjust the time out time")
        Report.failure(Tests.tests_completed)

    else:
        # Did Force Region succeed. True if vector z is close to 1 and force region magnitude is close to magnitude applied on Capsule
        force_region_result = (
            ifVectorClose(RegionObject.force_vector.z, UPWARD_Z_VECTOR) and RegionObject.force_region_in_range()
        )
        Report.result(Tests.force_region_success, force_region_result)
        Report.success(Tests.tests_completed)

    # Report test info to logSS
    Report.info("******* FINAL ENTITY INFORMATION *********")
    Report.info("Capsule Entered force region = {}".format(RegionObject.force_region_entered))
    Report.info("Start Z Position = {}  End Z Position = {}".format(Capsule.z_start_position, Capsule.z_end_position))
    Report.info("Capsule Gained height = {}".format(Capsule.gained_height))
    Report.info(
        "Vector = {}  Magnitude = {}".format(RegionObject.force_vector.z, RegionObject.force_magnitude_on_capsule)
    )

    # Exit game mode and close the editor
    Report.result(Tests.tests_completed, test_is_completed)
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_ImpulsesCapsuleShapedRigidBody)
