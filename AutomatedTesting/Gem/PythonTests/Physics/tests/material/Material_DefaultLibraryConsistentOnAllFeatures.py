"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C15096735
# Test Case Title : Verify that default material library works consistently across all systems that use it



# fmt:off
class Tests:
    # *** Universal test tuples ***
    enter_game_mode = ("Entered game mode",    "Failed to enter game mode")
    no_time_out     = ("No time out detected", "The test timed out")
    exit_game_mode  = ("Exited game mode",     "Couldn't exit game mode")

    # *** Terrain test tuples ***
    terrain_rubber_result_found                      = ("Terrain's Rubber Result Entity Found",                      "Terrain's Rubber Result Entity NOT Found")
    terrain_concrete_result_found                    = ("Terrain's Concrete Result Entity Found",                    "Terrain's Concrete Result Entity NOT Found")
    terrain_rubber_result_stopped                    = ("Terrain's Rubber Result Entity Stopped",                    "Terrain's Rubber Result Entity DID NOT Stop")
    terrain_concrete_result_stopped                  = ("Terrain's Concrete Result Entity Stopped",                  "Terrain's Concrete Result Entity DID NOT Stop")
    terrain_found                                    = ("Terrain Entity Found",                                      "Terrain Entity NOT Found")
    terrain_expected_collisions                      = ("Terrain Entity Collisions Were Expected",                   "Terrain Entity DID NOT Collide With All Expected Entities")
    terrain_trigger_rubber_high_found                = ("Terrain's Rubber High Trigger Found",                       "Terrain's Rubber High Trigger NOT Found")
    terrain_trigger_rubber_high_expected_collision   = ("Terrain's Rubber High Trigger Collision Was As Expected",   "Terrain's Rubber High Trigger Collision Was Not As Expected")
    terrain_trigger_rubber_low_found                 = ("Terrain's Rubber Low Trigger Found",                        "Terrain's Rubber Low Trigger NOT Found")
    terrain_trigger_rubber_low_expected_collision    = ("Terrain's Rubber Low Trigger Collision Was As Expected",    "Terrain's Rubber Low Trigger Collision Was Not As Expected")
    terrain_trigger_concrete_high_found              = ("Terrain's Concrete High Trigger Found",                     "Terrain's Concrete High Trigger NOT Found")
    terrain_trigger_concrete_high_expected_collision = ("Terrain's Concrete High Trigger Collision Was As Expected", "Terrain's Concrete High Trigger Collision Was Not As Expected")
    terrain_trigger_concrete_low_found               = ("Terrain's Concrete Low Trigger Found",                      "Terrain's Concrete Low Trigger NOT Found")
    terrain_trigger_concrete_low_expected_collision  = ("Terrain's Concrete Low Trigger Collision Was As Expected",  "Terrain's Concrete Low Trigger Collision Was Not As Expected")

    # *** Platform test tuples ***
    platform_rubber_result_found                      = ("Platform's Rubber Result Entity Found",                      "Platform's Rubber Result Entity NOT Found")
    platform_concrete_result_found                    = ("Platform's Concrete Result Entity Found",                    "Platform's Concrete Result Entity NOT Found")
    platform_rubber_result_stopped                    = ("Platform's Rubber Result Entity Stopped",                    "Platform's Rubber Result Entity DID NOT Stop")
    platform_concrete_result_stopped                  = ("Platform's Concrete Result Entity Stopped",                  "Platform's Concrete Result Entity DID NOT Stop")
    platform_rubber_found                             = ("Platform Rubber Test Entity Found",                          "Platform Rubber Test Entity NOT Found")
    platform_rubber_expected_collisions               = ("Platform Rubber Test Entity Collisions Were Expected",       "Platform Rubber Test Entity DID NOT Collide With All Expected Entities")
    platform_concrete_found                           = ("Platform Concrete Test Entity Found",                        "Platform Concrete Test Entity NOT Found")
    platform_concrete_expected_collisions             = ("Platform Concrete Test Entity Collisions Were Expected",     "Platform Concrete Test Entity DID NOT Collide With All Expected Entities")
    platform_trigger_rubber_high_found                = ("Platform's Rubber High Trigger Found",                       "Platform's Rubber High Trigger NOT Found")
    platform_trigger_rubber_high_expected_collision   = ("Platform's Rubber High Trigger Collision Was As Expected",   "Platform's Rubber High Trigger Collision Was Not As Expected")
    platform_trigger_rubber_low_found                 = ("Platform's Rubber Low Trigger Found",                        "Platform's Rubber Low Trigger NOT Found")
    platform_trigger_rubber_low_expected_collision    = ("Platform's Rubber Low Trigger Collision Was As Expected",    "Platform's Rubber Low Trigger Collision Was Not As Expected")
    platform_trigger_concrete_high_found              = ("Platform's Concrete High Trigger Found",                     "Platform's Concrete High Trigger NOT Found")
    platform_trigger_concrete_high_expected_collision = ("Platform's Concrete High Trigger Collision Was As Expected", "Platform's Concrete High Trigger Collision Was Not As Expected")
    platform_trigger_concrete_low_found               = ("Platform's Concrete Low Trigger Found",                      "Platform's Concrete Low Trigger NOT Found")
    platform_trigger_concrete_low_expected_collision  = ("Platform's Concrete Low Trigger Collision Was As Expected",  "Platform's Concrete Low Trigger Collision Was Not As Expected")

    # *** Controller test tuples ***
    controller_rubber_result_found                      = ("Controller's Rubber Result Entity Found",                      "Controller's Rubber Result Entity NOT Found")
    controller_concrete_result_found                    = ("Controller's Concrete Result Entity Found",                    "Controller's Concrete Result Entity NOT Found")
    controller_rubber_result_stopped                    = ("Controller's Rubber Result Entity Stopped",                    "Controller's Rubber Result Entity DID NOT Stop")
    controller_concrete_result_stopped                  = ("Controller's Concrete Result Entity Stopped",                  "Controller's Concrete Result Entity DID NOT Stop")
    controller_rubber_found                             = ("Controller Rubber Test Entity Found",                          "Controller Rubber Test Entity NOT Found")
    controller_rubber_expected_collisions               = ("Controller Rubber Test Entity Collisions Were Expected",       "Controller Rubber Test Entity DID NOT Collide With All Expected Entities")
    controller_concrete_found                           = ("Controller Concrete Test Entity Found",                        "Controller Concrete Test Entity NOT Found")
    controller_concrete_expected_collisions             = ("Controller Concrete Test Entity Collisions Were Expected",     "Controller Concrete Test Entity DID NOT Collide With All Expected Entities")
    controller_trigger_rubber_high_found                = ("Controller's Rubber High Trigger Found",                       "Controller's Rubber High Trigger NOT Found")
    controller_trigger_rubber_high_expected_collision   = ("Controller's Rubber High Trigger Collision Was As Expected",   "Controller's Rubber High Trigger Collision Was Not As Expected")
    controller_trigger_rubber_low_found                 = ("Controller's Rubber Low Trigger Found",                        "Controller's Rubber Low Trigger NOT Found")
    controller_trigger_rubber_low_expected_collision    = ("Controller's Rubber Low Trigger Collision Was As Expected",    "Controller's Rubber Low Trigger Collision Was Not As Expected")
    controller_trigger_concrete_high_found              = ("Controller's Concrete High Trigger Found",                     "Controller's Concrete High Trigger NOT Found")
    controller_trigger_concrete_high_expected_collision = ("Controller's Concrete High Trigger Collision Was As Expected", "Controller's Concrete High Trigger Collision Was Not As Expected")
    controller_trigger_concrete_low_found               = ("Controller's Concrete Low Trigger Found",                      "Controller's Concrete Low Trigger NOT Found")
    controller_trigger_concrete_low_expected_collision  = ("Controller's Concrete Low Trigger Collision Was As Expected",  "Controller's Concrete Low Trigger Collision Was Not As Expected")

    # *** Ragdoll test tuples ***
    ragdoll_rubber_result_found                      = ("Ragdoll's Rubber Result Entity Found",                      "Ragdoll's Rubber Result Entity NOT Found")
    ragdoll_concrete_result_found                    = ("Ragdoll's Concrete Result Entity Found",                    "Ragdoll's Concrete Result Entity NOT Found")
    ragdoll_rubber_result_stopped                    = ("Ragdoll's Rubber Result Entity Stopped",                    "Ragdoll's Rubber Result Entity DID NOT Stop")
    ragdoll_concrete_result_stopped                  = ("Ragdoll's Concrete Result Entity Stopped",                  "Ragdoll's Concrete Result Entity DID NOT Stop")
    ragdoll_rubber_found                             = ("Ragdoll Rubber Test Entity Found",                          "Ragdoll Rubber Test Entity NOT Found")
    ragdoll_rubber_expected_collisions               = ("Ragdoll Rubber Test Entity Collisions Were Expected",       "Ragdoll Rubber Test Entity DID NOT Collide With All Expected Entities")
    ragdoll_concrete_found                           = ("Ragdoll Concrete Test Entity Found",                        "Ragdoll Concrete Test Entity NOT Found")
    ragdoll_concrete_expected_collisions             = ("Ragdoll Concrete Test Entity Collisions Were Expected",     "Ragdoll Concrete Test Entity DID NOT Collide With All Expected Entities")
    ragdoll_trigger_rubber_high_found                = ("Ragdoll's Rubber High Trigger Found",                       "Ragdoll's Rubber High Trigger NOT Found")
    ragdoll_trigger_rubber_high_expected_collision   = ("Ragdoll's Rubber High Trigger Collision Was As Expected",   "Ragdoll's Rubber High Trigger Collision Was Not As Expected")
    ragdoll_trigger_rubber_low_found                 = ("Ragdoll's Rubber Low Trigger Found",                        "Ragdoll's Rubber Low Trigger NOT Found")
    ragdoll_trigger_rubber_low_expected_collision    = ("Ragdoll's Rubber Low Trigger Collision Was As Expected",    "Ragdoll's Rubber Low Trigger Collision Was Not As Expected")
    ragdoll_trigger_concrete_high_found              = ("Ragdoll's Concrete High Trigger Found",                     "Ragdoll's Concrete High Trigger NOT Found")
    ragdoll_trigger_concrete_high_expected_collision = ("Ragdoll's Concrete High Trigger Collision Was As Expected", "Ragdoll's Concrete High Trigger Collision Was Not As Expected")
    ragdoll_trigger_concrete_low_found               = ("Ragdoll's Concrete Low Trigger Found",                      "Ragdoll's Concrete Low Trigger NOT Found")
    ragdoll_trigger_concrete_low_expected_collision  = ("Ragdoll's Concrete Low Trigger Collision Was As Expected",  "Ragdoll's Concrete Low Trigger Collision Was Not As Expected")

    @staticmethod
    # Accesses the Tests dictionary to retrieve test tuples
    def get_test(test_name):
        return Tests.__dict__[test_name.lower()]
# fmt:on


def Material_DefaultLibraryConsistentOnAllFeatures():
    """
    Summary:
    This script tests the behavior of the default PhysXMaterial library. Two separate materials are applied to a variety
    of game entity types. The two materials have opposite restitution values (0.0 and 1.0). For each entity type and
    material another entity is made to "bounce" off it. The distance of the bounce is measured and validated via
    Triggers.

    Level Description:
    Four tests are step up, each with 1 or 2 TestEntities. Each of these sub-tests have two ResultEntities (either
    spheres or boxes) each set to collide with either a rubber or concrete material. Each of these ResultEntities have
    two TriggerEntities associated with them (High and Low). These Triggers are set up so the bouncing ResultEntities
    should trigger the Low TriggerEntity, but not the High.
    The four TestEntities whose material properties are validated are:
        Terrain - Using the Terrain Texture Tools
        Platforms - Basic box entities with RigidBodies and Colliders
        Character Controllers - PhysXCharacterController entities
        Ragdolls - Entities with Actor, AnimGraph and PhysXRagdoll components

    Expected Behavior:
    The four entity tests should run in series. Each test should have two spheres (or cubes) bounce off of their
    assigned test entity. Upon collision, Triggers should appear, and the spheres (or cubes) should only intersect
    with the lower trigger. When the spheres (or cubes) reach the highest point of their bounce they should disappear.
    At this time the Triggers should disappear and the next test should activate.

    Test Steps:
        1) Load level and enter game mode
        2) Find entities and initialize test states
        For each test
            3) Activate ResultEntities
            4) Wait for expected collision(s)
            5) Activate Triggers
            6) Wait for ResultEntities to stop / test to conclude
            7) Deactivate Triggers
        4) Exit game mode / Close editor

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
    import azlmbr
    import azlmbr.math as azmath

    # Constants
    TIME_OUT = 2.5
    INITIAL_VELOCITY = azmath.Vector3(0.0, 0.0, -10.0)

    # Entity Base class to be inherited by specific Entity classes
    # Handles as much "general entity" logic as possible to reduce code copying
    # Should be considered "virtual" and should not be directly instantiated
    class EntityBase:

        # Initializes the core features for an Entity and reports the critical result for being located successfully
        def __init__(self, name):
            # type: (str) -> None
            self.name = name
            self.active = True
            self.id = general.find_game_entity(self.name)
            # Report result
            found_test_tuple = Tests.get_test(self.name + "_Found")
            Report.critical_result(found_test_tuple, self.id.IsValid())

        # Sets whether the Entity is activated or deactivated. Logs event
        def set_active(self, active):
            # type: (bool) -> None
            if active and not self.active:
                azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", self.id)
            elif not active and self.active:
                azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "DeactivateGameEntity", self.id)
            self.active = active

        # String cast, returns Entity name
        def __str__(self):
            # type: () -> str
            return self.name

    # They are the default objects to be "bounced" off of TestEntities.
    # ResultEntities collect data about how far they bounce and deactivate themselves when done
    class ResultEntity(EntityBase):

        # Instantiates a ResultEntity: calls EntityBase.__init__
        def __init__(self, name):
            # type: (str) -> None
            EntityBase.__init__(self, name)
            self.collision_entity = None
            self.bounce_peak_pos = None
            self.result_tuple = Tests.get_test(self.name + "_Stopped")
            self.velocity = None
            self.current_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)
            self.initial_pos = self.current_pos
            # Double check that gravity is enabled
            if not azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id):
                azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetGravityEnabled", self.id, True)
            self.set_active(False)

        # Refreshes current velocity and checks if this Entity has stopped (or started "falling")
        # after expected collision, then deactivates itself
        def refresh(self):
            # type: () -> None
            if self.active:
                # 4) Wait for expected collision
                self.current_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)
                self.velocity = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)
                if self.collision_entity is not None:
                    # After collision takes place, track the highest bounce position
                    if self.velocity.z <= 0.0:
                        self.bounce_peak_pos = self.current_pos
                        self.set_active(False)

        # Overload of EntityBase::set_active
        # When activated, sets the linear velocity to the calibrated LINEAR_VELOCITY
        def set_active(self, active):
            # type: (bool) -> None
            EntityBase.set_active(self, active)
            if active:
                self.velocity = INITIAL_VELOCITY
                azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "SetLinearVelocity", self.id, self.velocity)

        # Reports test result.
        # Successful if we collided with something and then came to a stop
        def report_result(self):
            # type: () -> None
            Report.result(self.result_tuple, self.collision_entity is not None and self.bounce_peak_pos is not None)

        # Returns true if the entity is done with it's test
        def is_done(self):
            # type: () -> bool
            return self.bounce_peak_pos is not None

    # TestEntities are the surfaces that have their physics material set.
    # When a ResultEntity collides with a TestEntity, relevant Triggers are Activated
    class TestEntity(EntityBase):

        # Initializes a TestEntity: calls EntityBase.__init__
        def __init__(self, name, expected_entity, triggers):
            # type: (str, ResultEntity, [TriggerEntity,]) -> None
            EntityBase.__init__(self, name)
            self.expected_entity = expected_entity
            self.triggers = triggers
            self.result_tuple = Tests.get_test(self.name + "_Expected_Collisions")
            self.collision = False
            # Assign event handler
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)

        # Event handler for when a collision begins
        def on_collision_begin(self, args):
            # type: ([EntityId]) -> None
            if self.expected_entity.id.Equal(args[0]):
                if not self.collision:
                    self.collision = True
                    self.expected_entity.collision_entity = self  # Assign myself as their collision_entity
                    # 5) Activate triggers associated with the colliding Entity's test
                    for trigger in self.triggers:
                        trigger.set_active(True)

        # Reports result:
        # Successful if expected collision occurred
        def report_result(self):
            # type: () -> None
            Report.result(self.result_tuple, self.collision)

    # TriggerEntities are quantitative test metrics. They are used to either look for
    # expected collisions (Low Triggers) or to look for unexpected collisions (High Triggers)
    class TriggerEntity(EntityBase):

        def __init__(self, name, expected_entity):
            # type: (str, ResultEntity or None) -> None
            EntityBase.__init__(self, name)
            self.expected_entity = expected_entity  # Expected Entity to hit trigger (or None)
            self.result_entity = None  # Actual Entity to hit trigger (or None)
            self.triggered = False
            self.handler = None
            self.result_tuple = Tests.get_test(self.name + "_Expected_Collision")
            self.set_active(False)  # Triggers Deactivate after initialization and are activated by TestEntities

        # Override for EntityBase::set_active(bool) -> None
        # Sets event handler and calls EntityBase.set_active(bool)
        def set_active(self, active):
            # type: (bool) -> None
            if not self.active and active:
                # Activating: register event handler
                self.handler = azlmbr.physics.TriggerNotificationBusHandler()
                self.handler.connect(self.id)
                self.handler.add_callback("OnTriggerEnter", self.on_trigger_enter)
            elif self.active and not active:
                # Deactivating: disconnect event handler and set to None
                if self.handler is not None:
                    self.handler.disconnect()
                    self.handler = None
            EntityBase.set_active(self, active)

        # Event handler for when an entity enters trigger
        def on_trigger_enter(self, args):
            # type: ([EntityId]) -> None
            if not self.triggered:
                self.triggered = True
                self.result_entity = args[0]

        # Reports result:
        # Successful if the expected_entity and the result_entity are the same
        #   (Both None or both referencing the same Game Entity)
        def report_result(self):
            # type: () -> None
            if self.expected_entity is None:
                result = self.result_entity is None
            elif self.result_entity is None:
                result = False
            else:
                result = self.expected_entity.id.Equal(self.result_entity)
            Report.result(self.result_tuple, result)

    # Tests manage all the Entities required for a specific Material Assignment Test.
    class Test:

        # Initializes the test by setting up the required entities and lists for managing them.
        def __init__(self, base_str):
            # type: (str) -> None
            self.name = base_str

            rubber_result = ResultEntity(base_str + "_Rubber_Result")
            concrete_result = ResultEntity(base_str + "_Concrete_Result")
            rubber_triggers = [
                # Trigger Entities associated with rubber Result Entity
                TriggerEntity(base_str + "_Trigger_Rubber_High", None),
                TriggerEntity(base_str + "_Trigger_Rubber_Low", rubber_result),
            ]
            concrete_triggers = [
                # Trigger Entities associated with concrete Result Entity
                TriggerEntity(base_str + "_Trigger_Concrete_High", None),
                TriggerEntity(base_str + "_Trigger_Concrete_Low", concrete_result),
            ]

            # If base_str is "Terrain" both test entities should reference the same Terrain
            rubber_test_entity_name = base_str if base_str == "Terrain" else base_str + "_Rubber"
            concrete_test_entity_name = base_str if base_str == "Terrain" else base_str + "_Concrete"

            # Test Entities
            rubber_test_entity = TestEntity(rubber_test_entity_name, rubber_result, rubber_triggers)
            concrete_test_entity = TestEntity(concrete_test_entity_name, concrete_result, concrete_triggers)

            # Add entities to my lists
            self.results = [rubber_result, concrete_result]
            self.triggers = concrete_triggers + rubber_triggers
            self.test_objects = self.triggers + self.results + [rubber_test_entity, concrete_test_entity]

        # Calls refresh on result entities.
        def refresh(self):
            # type: () -> None
            for result in self.results:
                result.refresh()

        # Silently calls update, then returns True if all results are collected
        def is_done(self):
            # type: () -> bool
            self.refresh()
            if all(result.is_done() for result in self.results):
                # 7) Deactivate Triggers
                for trigger in self.triggers:
                    trigger.set_active(False)
                return True
            return False

        # Activates the result entity to start the test
        def start(self):
            # type: () -> None
            # 3 Activate ResultEntities
            for result in self.results:
                result.set_active(True)

        # Reports results for all test objects
        def report_result(self):
            # type: () -> None
            for obj in self.test_objects:
                obj.report_result()

    # *********** Execution Code ************

    # 1) Open level and start game mode
    helper.init_idle()
    helper.open_level("Physics", "Material_DefaultLibraryConsistentOnAllFeatures")
    helper.enter_game_mode(Tests.enter_game_mode)

    # Create and start Terrain Test
    tests = [
        # 2) Find entities and initialize test states
        Test("Terrain"),
        Test("Platform"),
        Test("Controller"),
        Test("Ragdoll")
    ]

    # 3) Run tests
    for test in tests:
        test.start()

        # 6) Wait for ResultEntities to stop / test to conclude
        Report.result(Tests.no_time_out, helper.wait_for_condition(test.is_done, TIME_OUT))

        test.report_result()

    # 4) Exit game mode and close editor
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_DefaultLibraryConsistentOnAllFeatures)
