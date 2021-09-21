"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C12712453
# Test Case Title : Verify Raycast Multiple Node


# fmt:off
class Tests:
    enter_game_mode           = ("Entered game mode",                         "Failed to enter game mode")
    exit_game_mode            = ("Exited game mode",                          "Couldn't exit game mode")
    test_completed            = ("The test successfully completed",           "The test timed out")
    # entities found
    caster_found              = ("Caster entity found",                       "Caster entity NOT found")
    multi_target_close_found  = ("Multicast close target entity found",       "Multicast close target NOT found")
    multi_target_far_found    = ("Multicast far target entity found",         "Multicast far target NOT found")
    single_target_found       = ("Single close target entity found",          "Single close target NOT found")
    fail_entities_found       = ("Found all fail entities",                   "Failed to find at least one fail entities")
    # entity results
    caster_result             = ("Caster was activated",                      "Caster was NOT activated")
    multi_target_close_result = ("Multicast close target was hit with a ray", "Multicast close target WAS NOT hit with a ray")
    multi_target_far_result   = ("Multicast far target was hit with a ray",   "Multicast far target WAS NOT hit with a ray")
    single_target_result      = ("Single target was hit with a ray",          "Single target WAS NOT hit with a ray")

# fmt:on


# Lines to search for by the log monitor
class Lines:
    # FAILURE (in all caps) is used as the failure flag for both this python script and ScriptCanvas.
    # If this is present in the log, something has gone fatally wrong and the test will fail.
    # Additional details as to why the test fails will be printed in the log accompanying this entry.
    unexpected = ["FAILURE"]


def ScriptCanvas_MultipleRaycastNode():
    """
    Summary:
    Uses script canvas to cast two types of rays in game mode (multiple raycast and single raycast). Script canvas
    validates the results from the raycasts, then deactivates any entities hit. This python script ensures only
    the expected entities were deactivated.

    Level Description:
    Caster - A sphere with gravity disabled and a scriptcanvas component for emitting raycasts. Caster starts inactive.
    Targets - Three spheres with gravity disabled. They are the expected targets of the raycasts.
    Fail Boxes - These boxes are positioned in various places where no ray should collide with them. They are set
    up print a fail message if any ray should hit them.
    ScriptCanvas - The ScriptCanvas script is attached to the Caster entity, and is set to start on entity Activation.
        The script will activate two different raycast nodes, a single raycast and a multiple raycast. For every raycast
        hit, the script validates all properties of a raycast (while printing to log) and then deactivates any entity
        that is [raycast] hit. This ScriptCanvas file can be found in this test's level folder named
        "Raycast.scrpitcanvas".

    Expected Behavior:
    The level should load and appear to instantly close. After setup the Caster sphere should emit the raycasts which
    should deactivate all expected Targets (via script canvas). The python script waits for the Targets to be
    deactivated then prints the results.

    Steps:
    1) Load level / enter game mode
    2) Retrieve entities
    3) Start the test (activate the caster entity)
    4) Wait for Target entities to deactivate
    5) Exit game mode and close the editor

    :return: None
    """
    
    # Disabled until Script Canvas merges the new backend
    return

    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr

    # Constants
    TIME_OUT = 1.0
    FAIL_ENTITIES = 5

    # Entity base class handles very general entity initialization
    class EntityBase:
        def __init__(self, name):
            # type: (str) -> None
            self.name = name
            self.id = general.find_game_entity(name)

            # Stores entity "is active" state. Most entities start as "Active"
            self.activated = True

    # Caster starts deactivated. When the caster is activated the script canvas test will begin.
    class Caster(EntityBase):
        def __init__(self, name):
            # type: (str) -> None
            EntityBase.__init__(self, name)
            found_tuple = Tests.__dict__[name.lower() + "_found"]
            Report.critical_result(found_tuple, self.id.isValid())

            # Caster starts as "Inactive" because the script canvas starts when Caster is Activated
            self.activated = False

        # Activates the Caster which starts it's ScriptCanvas script (and the test)
        def activate(self):
            # type: () -> None
            azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", self.id)
            self.activated = True
            Report.success(Tests.__dict__[self.name + "_result"])

    # Targets start activated and only get deactivated by the ScriptCanvas script.
    # Successful execution means every Target gets deactivated via ScriptCanvas
    class Target(EntityBase):
        def __init__(self, name):
            # type: (str) -> None
            EntityBase.__init__(self, name)
            found_tuple = Tests.__dict__[name.lower() + "_found"]
            Report.critical_result(found_tuple, self.id.isValid())

            self.handler = azlmbr.entity.EntityBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnEntityDeactivated", self.on_deactivation)

        # Callback: gets called when entity is deactivated. This is expected for Target entities.
        def on_deactivation(self, args):
            # type: ([EntityId, ...]) -> None
            if args[0].Equal(self.id):
                Report.success(Tests.__dict__[self.name + "_result"])
                self.activated = False

        # Disconnects event handler.
        # (Needed so the deactivation callback doesn't not get called on level clean up)
        def disconnect(self):
            # type: () -> None
            self.handler.disconnect()
            self.handler = None

    # Failure entities start activated and get deactivated via the ScriptCanvas. Ideally none will be deactivated
    # during the test. If one is deactivated, and Failure message is logged that will be caught by the log monitor.
    class Failure(EntityBase):
        def __init__(self, num):
            # type: (int) -> None
            EntityBase.__init__(self, "fail_" + str(num))
            if not self.id.IsValid():
                Report.info("FAILURE: {} could not be found".format(self.name))
            self.handler = azlmbr.entity.EntityBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnEntityDeactivated", self.on_deactivation)

        # Callback: gets called when entity is deactivated. This is expected for Target entities.
        def on_deactivation(self, args):
            # type: ([EntityId, ...]) -> None
            if args[0].Equal(self.id):
                Report.info("{}: FAILURE".format(self.name))
                self.activated = False

        # Disconnects event handler.
        # (Needed so the deactivation callback doesn't not get called on level clean up)
        def disconnect(self):
            # type: () -> None
            self.handler.disconnect()
            self.handler = None

    # 1) Open level
    helper.init_idle()
    helper.open_level("Physics", "ScriptCanvas_MultipleRaycastNode")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve entities
    caster = Caster("caster")
    targets = [Target("multi_target_close"), Target("multi_target_far"), Target("single_target")]
    fails = [Failure(i) for i in range(FAIL_ENTITIES)]
    Report.critical_result(Tests.fail_entities_found, all(entity.id.isValid() for entity in fails))

    # 3) Start the test
    caster.activate()

    # 4) Wait for Target entities to deactivate
    test_completed = helper.wait_for_condition(lambda: all(not target.activated for target in targets), TIME_OUT)
    Report.result(Tests.test_completed, test_completed)

    # Disconnect all "on deactivated" event handlers so they don't get called implicitly on level cleanup
    for entity in targets + fails:
        entity.disconnect()

    # 5) Close test
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ScriptCanvas_MultipleRaycastNode)
