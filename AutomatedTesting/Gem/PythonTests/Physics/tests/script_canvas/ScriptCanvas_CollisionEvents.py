"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test Case ID    : C12712452
# Test Case Title : Verify ScriptCanvas Collision Events



# fmt: off
class Tests:
    enter_game_mode            = ("Entered game mode",                  "Failed to enter game mode")
    terrain_found_valid        = ("PhysX Terrain found and validated",  "PhysX Terrain not found and validated")
    sphere_found_valid         = ("Sphere found and validated",         "Sphere not found and validated")
    begin_signal_found_valid   = ("Begin Signal found and validated",   "Begin Signal not found and validated")
    persist_signal_found_valid = ("Persist Signal found and validated", "Persist Signal not found and validated")
    end_signal_found_valid     = ("End Signal found and validated",     "End Signal not found and validated")
    sphere_above_terrain       = ("Sphere is above terrain",            "Sphere is not above terrain")
    sphere_gravity_enabled     = ("Gravity is enabled on Sphere",       "Gravity is not enabled on Sphere")
    sphere_started_bouncing    = ("Sphere started bouncing",            "Sphere did not start bouncing")
    sphere_stopped_bouncing    = ("Sphere stopped bouncing",            "Sphere did not stop bouncing")
    event_records_match        = ("Event records match",                "Event records do not match")
    exit_game_mode             = ("Exited game mode",                   "Failed to exit game mode")
# fmt: on


def ScriptCanvas_CollisionEvents():
    """
    Summary:
    This script runs an automated test to verify that the Script Canvas nodes "On Collision Begin", "On Collision
    Persist", and "On Collision End" will function as intended for the entity to which their Script Canvas is attached.

    Level Description:
    A sphere (entity: Sphere) is above a terrain (entity: PhysX Terrain). The sphere has a PhysX Rigid Body, a PhysX
    Collider with shape Sphere, and gravity enabled. The sphere has a Script Canvas attached to it which will toggle the
    activation of three signal entities (entity: Begin Signal), (entity: Persist Signal), and (entity: End Signal).
    Begin Signal's activation will toggle (switch from activated to deactivated or vice versa) when a collision begins
    with the sphere, Persist Signal's activation will toggle when a collision persists with the sphere, and End Signal's
    activation will toggle when a collision ends with the sphere.

    Expected behavior:
    The sphere will fall toward and collide with the terrain. The sphere will bounce until it comes to rest. The Script
    Canvas will cause the signal entities to activate and deactivate in the same pattern as the collision events which
    occur on the sphere.

    Test Steps:
    1) Open level and enter game mode
    2) Retrieve and validate entities
    3) Check that the sphere is above the terrain
    4) Check that gravity is enabled on the sphere
    5) Wait for the initial collision between the sphere and the terrain or timeout
    6) Wait for the sphere to stop bouncing
    7 Check that the event records match
    8) Exit game mode and close editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    # Setup path
    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr.components
    import azlmbr.entity
    import azlmbr.physics
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Constants
    TIME_OUT_SECONDS = 3.0
    TERRAIN_START_Z = 32.0
    SPHERE_RADIUS = 1.0

    class Entity:
        def __init__(self, name, found_valid_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.found_valid_test = found_valid_test

    class Sphere(Entity):
        def __init__(self, name, found_valid_test, event_records_match_test):
            Entity.__init__(self, name, found_valid_test)
            self.event_records_match_test = event_records_match_test
            self.collided = False
            self.stopped_bouncing = False
            self.collision_event_record = []
            self.script_canvas_event_record = []

            # Set up collision notification handler
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)
            self.handler.add_callback("OnCollisionPersist", self.on_collision_persist)
            self.handler.add_callback("OnCollisionEnd", self.on_collision_end)

        def get_z_position(self):
            z_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", self.id)
            Report.info("{}'s z-position: {}".format(self.name, z_position))
            return z_position

        def is_gravity_enabled(self):
            return azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)

        # Set up reporting of the event records and whether they match
        def match_event_records(self):
            Report.info("{} collision event record: {}".format(self.name, self.collision_event_record))
            Report.info("Script Canvas event record: {}".format(self.script_canvas_event_record))
            return self.collision_event_record == self.script_canvas_event_record

        # Set up collision event detection and update collision event record
        def on_collision(self, event, other_id):
            if not self.collided:
                self.collided = True
            other_name = azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "GetEntityName", other_id)
            Report.info("{} collision {}s with {}".format(self.name, event, other_name))
            self.collision_event_record.append(event)

        def on_collision_begin(self, args):
            self.on_collision("begin", args[0])

        def on_collision_persist(self, args):
            self.on_collision("persist", args[0])

        def on_collision_end(self, args):
            self.on_collision("end", args[0])

        # Set up detection of the sphere coming to rest
        def bouncing_stopped(self):
            if not azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsAwake", self.id):
                self.stopped_bouncing = True
            return self.stopped_bouncing

    class SignalEntity(Entity):
        def __init__(self, name, found_valid_test, monitored_entity, event):
            Entity.__init__(self, name, found_valid_test)
            self.monitored_entity = monitored_entity
            self.event = event

            # Set up activation and deactivation notification handler
            self.handler = azlmbr.entity.EntityBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnEntityActivated", self.on_entity_activated)
            self.handler.add_callback("OnEntityDeactivated", self.on_entity_deactivated)

        # Set up activation and deactivation detection and update Script Canvas event record
        def on_entity_activated(self, args):
            self.monitored_entity.script_canvas_event_record.append(self.event)

        def on_entity_deactivated(self, args):
            self.monitored_entity.script_canvas_event_record.append(self.event)

    # 1) Open level and enter game mode
    helper.init_idle()
    helper.open_level("Physics", "ScriptCanvas_CollisionEvents")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve and validate entities
    terrain = Entity("PhysX Terrain", Tests.terrain_found_valid)
    sphere = Sphere("Sphere", Tests.sphere_found_valid, Tests.event_records_match)
    begin_signal = SignalEntity("Begin Signal", Tests.begin_signal_found_valid, sphere, "begin")
    persist_signal = SignalEntity("Persist Signal", Tests.persist_signal_found_valid, sphere, "persist")
    end_signal = SignalEntity("End Signal", Tests.end_signal_found_valid, sphere, "end")

    entities = [terrain, sphere, begin_signal, persist_signal, end_signal]
    for entity in entities:
        Report.critical_result(entity.found_valid_test, entity.id.IsValid())

    # 3) Check that the sphere is above the terrain
    Report.critical_result(Tests.sphere_above_terrain, sphere.get_z_position() - SPHERE_RADIUS > TERRAIN_START_Z)

    # 4) Check that gravity is enabled on the sphere
    Report.critical_result(Tests.sphere_gravity_enabled, sphere.is_gravity_enabled())

    # 5) Wait for the initial collision between the sphere and the terrain or timeout
    helper.wait_for_condition(lambda: sphere.collided, TIME_OUT_SECONDS)
    Report.critical_result(Tests.sphere_started_bouncing, sphere.collided)

    # 6) Wait for the sphere to stop bouncing
    helper.wait_for_condition(sphere.bouncing_stopped, TIME_OUT_SECONDS)
    Report.result(Tests.sphere_stopped_bouncing, sphere.stopped_bouncing)

    # 7 Check that the event records match
    Report.result(Tests.event_records_match, sphere.match_event_records())

    # 8) Exit game mode and close editor
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ScriptCanvas_CollisionEvents)
