"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C12905527
# Test Case Title : Check that deviation occurring in Force Magnitude due to Values in Force direction is not large


# fmt: off
class Tests():
    enter_game_mode       = ("Entered game mode",                                       "Failed to enter game mode")
    find_force_region     = ("Force region was found",                                  "Force region was not found")
    find_sphere           = ("Sphere was found",                                        "Sphere was not found")
    sphere_entered_region = ("Sphere entered force region",                             "Sphere did not enter force region")
    sphere_exited_region  = ("Sphere exited force region",                              "Sphere did not exit force region")
    force_magnitude_close = ("The net force magnitude was close to the expected value", "The net force magnitude was not close to the expected value")
    exit_game_mode        = ("Exited game mode",                                        "Couldn't exit game mode")
# fmt: on


def ForceRegion_SmallMagnitudeDeviationOnLargeForces():
    """
    Summary:
    Runs an automated test to ensure that the calculated net force magnitude is close to the configured value

    Level Description:
    A sphere (Sphere) is positioned above a force region (ForceRegion)
        Sphere has a sphere PhysX collider and PhysX Rigid Body. Gravity is disabled, and it has an initial velocity of
        2 m/s in the Z direction.

        ForceRegion has a box PhysX collider and PhysX Force Region. Magnitude on the force region is set to 1,000,000.0

    Expected Behavior:
    The sphere enters and exits the force region. on_calc_net_force returns a value close to 1,000,000.0

    Test Steps:
    1) Open level
    2) Enter game mode
    3) Validate entities
    4) Wait for the sphere to enter and exit the force region
    5) Check the calculated net force against what we expect
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

    import azlmbr
    import azlmbr.legacy.general as general

    EXPECTED_MAGNITUDE = 1000000.0
    PERMISSIBLE_ERROR = 0.001  # +/- 0.1%
    TIMEOUT = 1.0

    class Sphere:
        def __init__(self, name):
            self.name = name
            self.id = general.find_game_entity(name)
            self.entered_force_region = False
            self.exited_force_region = False
            self.net_force_magnitude = 0

    def on_trigger_enter(args):
        Report.info("triggered")
        other_id = args[0]
        if other_id.Equal(sphere.id):
            sphere.entered_force_region = True

    def on_trigger_exit(args):
        other_id = args[0]
        if other_id.Equal(sphere.id):
            sphere.exited_force_region = True

    def on_calc_net_force(args):
        other_id = args[1]
        force_magnitude = args[3]

        if other_id.Equal(sphere.id):
            sphere.net_force_magnitude = force_magnitude

    helper.init_idle()
    # 1) Open level
    Report.info(general.get_current_level_name())
    helper.open_level("Physics", "ForceRegion_SmallMagnitudeDeviationOnLargeForces")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Validate entities
    sphere = Sphere("Sphere")
    force_region_id = general.find_game_entity("ForceRegion")

    Report.critical_result(Tests.find_sphere, sphere.id.IsValid())
    Report.critical_result(Tests.find_force_region, force_region_id.IsValid())

    # Create handlers
    trigger_handler = azlmbr.physics.TriggerNotificationBusHandler()
    trigger_handler.connect(force_region_id)
    trigger_handler.add_callback("OnTriggerEnter", on_trigger_enter)
    trigger_handler.add_callback("OnTriggerExit", on_trigger_exit)

    net_force_handler = azlmbr.physics.ForceRegionNotificationBusHandler()
    net_force_handler.connect(None)
    net_force_handler.add_callback("OnCalculateNetForce", on_calc_net_force)

    # 4) Wait for the sphere to enter and exit the force region
    Report.result(Tests.sphere_entered_region, helper.wait_for_condition(lambda: sphere.entered_force_region, TIMEOUT))
    Report.result(Tests.sphere_exited_region, helper.wait_for_condition(lambda: sphere.exited_force_region, TIMEOUT))

    # 5) Check the calculated net force against what we expect
    absolute_difference = abs(sphere.net_force_magnitude - EXPECTED_MAGNITUDE)
    error = absolute_difference / EXPECTED_MAGNITUDE
    net_force_was_close = error < PERMISSIBLE_ERROR

    Report.result(Tests.force_magnitude_close, net_force_was_close)
    if not net_force_was_close:
        Report.info(
            "\nExpected Magnitude: {}"
            "\nActual Magnitude: {}"
            "\nPermissible Error: {}"
            "\nMeasured Error: {}".format(EXPECTED_MAGNITUDE, sphere.net_force_magnitude, PERMISSIBLE_ERROR, error)
        )

    # 6) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_SmallMagnitudeDeviationOnLargeForces)
