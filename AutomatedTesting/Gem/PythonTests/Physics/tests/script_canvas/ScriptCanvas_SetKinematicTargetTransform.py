"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test Case
# ID    : C14976308
# Title : Verify that SetKinematicTarget on PhysX rigid body updates transform for kinematic entities and vice versa



# fmt: off
class Tests:
    enter_game_mode              = ("Entered game mode",                                                  "Failed to enter game mode")
    sphere_found_valid           = ("Sphere found and validated",                                         "Sphere not found and validated")
    kinematic_target_found_valid = ("Kinematic_Target found and validated",                               "Kinematic_Target not found and validated")
    transform_target_found_valid = ("Transform_Target found and validated",                               "Transform_Target not found and validated")
    signal_found_valid           = ("Signal found and validated",                                         "Signal not found and validated")
    sphere_gravity_disabled      = ("Gravity is disabled on Sphere",                                      "Gravity is not disabled on Sphere")
    sphere_kinematic             = ("Sphere is kinematic",                                                "Sphere is not kinematic")
    entity_translations_differ   = ("Each entity has a different initial translation",                    "Each entity does not have a different initial translation")
    entity_rotations_differ      = ("Each entity has a different initial rotation",                       "Each entity does not have a different initial rotation")
    entity_scales_differ         = ("Each entity has a different initial scale",                          "Each entity does not have a different initial scale")
    sphere_translation_1_valid   = ("Set Kinematic Target updated Sphere's translation to the new value", "Set Kinematic Target failed to update Sphere's translation to the new value")
    sphere_rotation_1_valid      = ("Set Kinematic Target updated Sphere's rotation to the new value",    "Set Kinematic Target failed to update Sphere's rotation to the new value")
    sphere_transform_2_valid     = ("Set World Transform updated Sphere's transform to the new value",    "Set World Transform failed to update Sphere's transform to the new value")
    exit_game_mode               = ("Exited game mode",                                                   "Failed to exit game mode")
# fmt: on


def ScriptCanvas_SetKinematicTargetTransform():
    """
    Summary:
    This script runs an automated test to verify the results of two Script Canvas nodes acting on a kinematic rigid
    body:
    1) Set Kinematic Target will update the kinematic rigid body's translation and rotation to those of the transform
    which the node passes to it.
    2) Set World Transform will update a kinematic rigid body's transform to the transform which the node passes to it.

    Level Description:
    Entity: Sphere:           PhysX Rigid Body, PhysX Collider with sphere shape, and Mesh with sphere asset
                              Gravity disabled, kinematic enabled
                              Translate (79.0, 39.0, 34.0) in meters
                              Rotate (1.0, 2.0, 3.0) in degrees
                              Scale (1.0, 1.0, 1.0)
    Entity: Kinematic_Target: Mesh with cube asset
                              Translate (48.0, 56.0, 36.0) in meters
                              Rotate (11.0, 12.0, 13.0) in degrees
                              Scale (1.1, 1.2, 1.3)
    Entity: Transform_Target: Mesh with cube asset
                              Translate (72.0, 80.0, 38.0) in meters
                              Rotate (21.0, 22.0, 23.0) in degrees
                              Scale (2.1, 2.2, 2.3)
    Entity: Signal:           Start inactive, attached Script Canvas asset which will cause the following:
                              On Signal Activated will Set Kinematic Target on Sphere to Kinematic_Target's transform
                              On Signal Deactivated will Set World Transform on Sphere to Transform_Target's transform
    Other than the signal, the entities all have different translations, rotations, and scales.

    Expected behavior:
    When the script activates Signal, Sphere's translation and rotation will update to those of Kinematic_Target. When
    the script deactivates Signal, Sphere's transform will update to that of Transform_Target.
    NOTE: There is a known bug (LY-107723) which causes the rotation to update to a value that is not sufficiently close
    to the expected result when using Set Kinematic Target which will cause the test to fail:
    LY-107723

    Test Steps:
     1) Open level and enter game mode
     2) Retrieve and validate entities
     3) Check that gravity is disabled on the sphere
     4) Check that the sphere is kinematic
     5) Check that each entity except the signal has a different initial translation
     6) Check that each entity except the signal has a different initial rotation
     7) Check that each entity except the signal has a different initial scale
     8) Activate the signal entity to trigger the Set Kinematic Target node in Script Canvas
     9) Wait one frame and check that the sphere's translation has updated to that of Kinematic_Target
    10) Check that the sphere's rotation has updated to that of Kinematic_Target
    11) Deactivate the signal entity to trigger the Set World Transform node in Script Canvas
    12) Check that the sphere's transform has updated to that of Transform_Target
    13) Exit game mode and close editor

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
    import azlmbr.math
    import azlmbr.physics
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import itertools

    class Entity:
        def __init__(self, name):
            self.name = name
            self.id = general.find_game_entity(name)
            self.found_valid_test = Tests.__dict__[self.name.lower() + "_found_valid"]
            Report.critical_result(self.found_valid_test, self.id.IsValid())

        def is_gravity_disabled(self):
            return not azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)

        def is_kinematic(self):
            return azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsKinematic", self.id)

        def get_world_transform(self):
            transform = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTM", self.id)
            Report.info_vector3(transform.position, "{}'s position:".format(self.name))
            Report.info_vector3(transform.basisX, "{}'s basisX:".format(self.name))
            Report.info_vector3(transform.basisY, "{}'s basisY:".format(self.name))
            Report.info_vector3(transform.basisZ, "{}'s basisZ:".format(self.name))
            return transform

        def get_world_translation(self):
            translation = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)
            Report.info_vector3(translation, "{}'s Translation:".format(self.name))
            return translation

        def get_world_rotation(self):
            rotation = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldRotation", self.id)
            Report.info_vector3(rotation, "{}'s Rotation:".format(self.name))
            return rotation

        def get_world_scale(self):
            scale = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldScale", self.id)
            Report.info_vector3(scale, "{}'s Scale:".format(self.name))
            return scale

        def transform_matches(self, other):
            return self.get_world_transform().Equal(other.get_world_transform())

        def translation_matches(self, other):
            return self.get_world_translation().Equal(other.get_world_translation())

        def rotation_matches(self, other):
            return self.get_world_rotation().Equal(other.get_world_rotation())

        def scale_matches(self, other):
            return self.get_world_scale().Equal(other.get_world_scale())

    def entities_translations_differ(entities):
        for entity_a, entity_b in itertools.combinations(entities, 2):
            if entity_a.translation_matches(entity_b):
                return False
        return True

    def entities_rotations_differ(entities):
        for entity_a, entity_b in itertools.combinations(entities, 2):
            if entity_a.rotation_matches(entity_b):
                return False
        return True

    def entities_scales_differ(entities):
        for entity_a, entity_b in itertools.combinations(entities, 2):
            if entity_a.scale_matches(entity_b):
                return False
        return True

    # 1) Open level and enter game mode
    helper.init_idle()
    helper.open_level("Physics", "ScriptCanvas_SetKinematicTargetTransform")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve and validate entities
    sphere = Entity("Sphere")
    kinematic_target = Entity("Kinematic_Target")
    transform_target = Entity("Transform_Target")
    signal = Entity("Signal")
    non_signal_entities = (sphere, kinematic_target, transform_target)

    # 3) Check that gravity is disabled on the sphere
    Report.critical_result(Tests.sphere_gravity_disabled, sphere.is_gravity_disabled())

    # 4) Check that the sphere is kinematic
    Report.critical_result(Tests.sphere_kinematic, sphere.is_kinematic())

    # 5) Check that each entity except the signal has a different initial translation
    Report.critical_result(Tests.entity_translations_differ, entities_translations_differ(non_signal_entities))

    # 6) Check that each entity except the signal has a different initial rotation
    Report.critical_result(Tests.entity_rotations_differ, entities_rotations_differ(non_signal_entities))

    # 7) Check that each entity except the signal has a different initial scale
    Report.critical_result(Tests.entity_scales_differ, entities_scales_differ(non_signal_entities))

    # 8) Activate the signal entity to trigger the Set Kinematic Target node in Script Canvas
    azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", signal.id)

    # 9) Wait one frame and check that the sphere's translation has updated to that of Kinematic_Target
    general.idle_wait_frames(1)
    Report.result(Tests.sphere_translation_1_valid, sphere.translation_matches(kinematic_target))

    # 10) Check that the sphere's rotation has updated to that of Kinematic_Target
    # NOTE: This test currently fails due to a known bug (LY-107723)
    Report.result(Tests.sphere_rotation_1_valid, sphere.rotation_matches(kinematic_target))

    # 11) Deactivate the signal entity to trigger the Set World Transform node in Script Canvas
    azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "DeactivateGameEntity", signal.id)

    # 12) Check that the sphere's transform has updated to that of Transform_Target
    Report.result(Tests.sphere_transform_2_valid, sphere.transform_matches(transform_target))

    # 13) Exit game mode and close editor
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ScriptCanvas_SetKinematicTargetTransform)
