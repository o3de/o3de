"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C15096737
# Test Case Title : Verify that a change in the default material library material information
# affects all the materials that reference it, even non-defaulted
# exactly like if the library was selected


# fmt: off
class Tests:
    # level
    enter_game_mode_0                = ("Entered game mode 0",                           "Failed to enter game mode 0")
    exit_game_mode_0                 = ("Exited game mode 0",                            "Couldn't exit game mode 0")
    enter_game_mode_1                = ("Entered game mode 1",                           "Failed to enter game mode 1")
    exit_game_mode_1                 = ("Exited game mode 1",                            "Couldn't exit game mode 1")
    
    # targets
    terrain_found                    = ("Terrain found in each test",                    "TERRAIN NOT FOUND in a test")
    target_character_rubber_found    = ("target_character_rubber found in each test",    "target_character_rubber NOT FOUND in a test")
    target_character_concrete_found  = ("target_character_concrete found in each test",  "target_character_concrete NOT FOUND in a test")

    # collider activity
    rubber_sphere_found              = ("rubber_sphere found in each test",              "rubber_sphere NOT FOUND in a test in a test")
    rubber_sphere_trigger_found      = ("rubber_sphere_trigger found in each test",      "rubber_sphere_trigger NOT FOUND in a test")
    rubber_sphere_collided           = ("rubber_sphere collided in each test",           "rubber_sphere DIDN'T COLLIDE in a test")
    
    concrete_sphere_found            = ("concrete_sphere found in each test",            "concrete_sphere NOT FOUND in a test")
    concrete_sphere_trigger_found    = ("concrete_sphere_trigger found in each test",    "concrete_sphere_trigger NOT FOUND in a test")
    concrete_sphere_collided         = ("concrete_sphere collided in each test",         "concrete_sphere DIDN'T COLLIDE in a test")
    
    character_rubber_found           = ("character_rubber found in each test",           "character_rubber NOT FOUND in a test")
    character_rubber_trigger_found   = ("character_rubber_trigger found in each test",   "character_rubber_trigger NOT FOUND in a test")
    character_rubber_collided        = ("character_rubber collided in each test",        "character_rubber DIDN'T COLLIDE in a test")
    
    character_concrete_found         = ("character_concrete found in each test",         "character_concrete NOT FOUND in a test")
    character_concrete_trigger_found = ("character_concrete_trigger found in each test", "character_concrete_trigger NOT FOUND in a test")
    character_concrete_collided      = ("character_concrete collided in each test",      "character_concrete DIDN'T COLLIDE in a test")
    
    terrain_rubber_found             = ("terrain_rubber found in each test",             "terrain_rubber NOT FOUND in a test")
    terrain_rubber_trigger_found     = ("terrain_rubber_trigger found in each test",     "terrain_rubber_trigger NOT FOUND in a test")
    terrain_rubber_collided          = ("terrain_rubber collided in each test",          "terrain_rubber DIDN'T COLLIDE in a test")
    
    terrain_concrete_found           = ("terrain_concrete found in each test",           "terrain_concrete NOT FOUND in a test")
    terrain_concrete_trigger_found   = ("terrain_concrete_trigger found in each test",   "terrain_concrete_trigger NOT FOUND in a test")
    terrain_concrete_collided        = ("terrain_concrete collided in each test",        "terrain_concrete DIDN'T COLLIDE in a test")
    
    ragdoll_rubber_found             = ("ragdoll_rubber found in each test",             "ragdoll_rubber NOT FOUND in a test")
    ragdoll_rubber_trigger_found     = ("ragdoll_rubber_trigger found in each test",     "ragdoll_rubber_trigger NOT FOUND in a test")
    ragdoll_rubber_collided          = ("ragdoll_rubber collided in each test",          "ragdoll_rubber DIDN'T COLLIDE in a test")
    
    ragdoll_concrete_found           = ("ragdoll_concrete found in each test",           "ragdoll_concrete NOT FOUND in a test")
    ragdoll_concrete_trigger_found   = ("ragdoll_concrete_trigger found in each test",   "ragdoll_concrete_trigger NOT FOUND in a test")
    ragdoll_concrete_collided        = ("ragdoll_concrete collided in each test",        "ragdoll_concrete DIDN'T COLLIDE in a test")

    # Verification
    material_library_updated         = ("Default material library updated",              "Default material library not updated")
    rubber_material_changed          = ("Rubber material changed correctly",             "Rubber didn't react correctly")
    concrete_material_changed        = ("Concrete material changed correctly",           "Concrete didn't react correctly")
# fmt: on

def Material_DefaultMaterialLibraryChangesWork():
    """
    Summary: Runs an automated test to verify that material selected in the default material library is applied to PhysX
        colliders, character controller, terrain texture layers and ragdolls and that material can respond to change.

    PhysX Config Description:
        A PhysX material library called all_ones is set as the default material library in PhysX Config File.
        The library has two materials surfaces: rubber with Restitution = 1.0, Restitution Combine = Maximum
        and concrete with Restitution = 0.0, Restitution combine = Multiply.
        The custom config file is loaded before editor is launched.

    Level Description:
        Consists of 4 sets of entities.
        Each entity has either rubber or concrete material assigned to it. Each entity has a corresponding trigger placed
        between the entity and its collision target entity (terrain or character controller).
        The entities, their triggers and their target are colored blue if they have rubber material, or red for concrete.

    Expected Behavior:
        The entities start their movement once the level is loaded. They should touch their corresponding triggers first,
        then collide with their target entity. The ones with rubber material are supposed to bounce back and touch the
        triggers. The ones with concrete material are supposed to stick to the target and stop moving, therefore not
        touching the triggers anymore. After the edits to material library the affect will be swapped.

    Main Script Steps:
    1) Loads the level
    2) Setup targets and colliders
    3) Run Test 0
    4) Edit Material Library
    5) Run Test 1
    6) Validate Results
    7) Close editor

    Test Steps:
    1) Enter Game Mode
    2) Validate target Id's
    3) Validate all Colliders and setup targets
    4) Wait for Collision, Report Results
    5) Allow Time to Hit trigger
    6) Exit Game Mode
    """

    import os
    import sys


    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import azlmbr.legacy.general as general
    import azlmbr.bus
    from Physmaterial_Editor import Physmaterial_Editor

    # Constants
    TIME_OUT = 2.0
    PROPAGATION_FRAMES = 180

    def get_test(entity_name, suffix):
        return Tests.__dict__[entity_name + suffix]

    # Base class for triggers, targets and colliders
    class Entity(object):
        # Global Holding Variable for test index
        current_test = None

        def __init__(self, name):
            self.name = name
            self.found_in_before_test = False

        # Validates entity ids reports if the ids are valid for both test cases
        # Fast fails if any id is invalid
        def validate_id(self):
            self.id = general.find_game_entity(self.name)
            if Entity.current_test == 0 and self.id.IsValid():
                self.found_in_before_test = True
            elif Entity.current_test == 1:
                Report.critical_result(get_test(self.name, "_found"), self.id.IsValid() and self.found_in_before_test)
            else:
                helper.fail_fast("{} was not found in test {}".format(self.name, Entity.current_test))

        @property
        def position(self):
            return azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", self.id)

        @property
        def velocity(self):
            return azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "GetLinearVelocity", self.id)

    class Collider(Entity):
        def __init__(self, name, target):
            Entity.__init__(self, name)
            self.target = target
            # Data holding variables
            self.collided_with_target_0 = False
            self.collided_with_target_1 = False
            self.hit_trigger_0 = False
            self.hit_trigger_1 = False

        # Initialized target collisions
        def setup_target(self):
            self.target.validate_id
            # Watch target for collision with collider
            self.collision_handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.collision_handler.connect(self.id)
            self.collision_handler.add_callback("OnCollisionBegin", self.detect_collision_target)

        def activate_trigger(self):
            azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "ActivateGameEntity", self.trigger.id)
            Report.info("{} activated".format(self.trigger.name))
        
        # Sets up trigger and activates it post-collision with target
        def setup_trigger(self):
            if Entity.current_test == 0:
                self.trigger = Entity(self.name + "_trigger")
            self.trigger.validate_id()
            self.activate_trigger()
            # Watch for collider entrance
            self.trigger.handler = azlmbr.physics.TriggerNotificationBusHandler()
            self.trigger.handler.connect(self.trigger.id)
            self.trigger.handler.add_callback("OnTriggerEnter", self.on_trigger_enter)

        def on_trigger_enter(self, args):
            if self.id.equal(args[0]) and not getattr(self, "hit_trigger_{}".format(Entity.current_test)):
                Report.info("{} entered {} in test {}".format(self.name, self.trigger.name, Entity.current_test))
                setattr(self, "hit_trigger_{}".format(Entity.current_test), True)

        def detect_collision_target(self, args):
            print("Collision_going_on")
            if self.target.id.equal(args[0]) and not getattr(self, "collided_with_target_{}".format(Entity.current_test)):
                Report.info("{} collided with {}".format(self.name, self.target.name))
                setattr(self, "collided_with_target_{}".format(Entity.current_test), True)
                self.setup_trigger()

    def edit_material_library():
        # Flips the  Restitution values of rubber and concrete
        material_library = Physmaterial_Editor("all_ones_1.physmaterial")
        rubber_restitution = material_library.modify_material("rubber", "Restitution", 0)
        rubber_restitution_combine = material_library.modify_material("rubber", "RestitutionCombine", "Multiply")
        concrete_restitution = material_library.modify_material("concrete", "Restitution", 1)
        concrete_restitution_combine = material_library.modify_material("concrete", "RestitutionCombine", "Average")

        material_library.save_changes()
        return rubber_restitution and rubber_restitution_combine and concrete_restitution and concrete_restitution_combine

    def check_rubber_material_updated(rubber_colliders):
        # Checks that all rubber colliders hit the trigger on test 0 and not on test 1
        before_test_passed = all([collider.hit_trigger_0 for collider in rubber_colliders])
        after_test_passed = all([not collider.hit_trigger_1 for collider in rubber_colliders])

        return before_test_passed and after_test_passed

    def check_concrete_material_updated(concrete_colliders):
        # Checks that all concrete colliders didn't hit the trigger on test 0 and did on test 1
        before_test_passed = all([not collider.hit_trigger_0 for collider in concrete_colliders])
        after_test_passed = all([collider.hit_trigger_1 for collider in concrete_colliders])

        return before_test_passed and after_test_passed

    def test_run(index, all_colliders):
        Entity.current_test = index
        # 1) Enter Game Mode
        helper.enter_game_mode(get_test("enter_game_mode_", str(index)))

        # 2) Validate target Ids 
        terrain.validate_id()
        target_character_concrete.validate_id()
        target_character_rubber.validate_id()

        # 3) Validate all Colliders and setup targets
        for collider in all_colliders:
            collider.validate_id()
            collider.setup_target()

        # 4) Wait for Collision, Report Results
        if not helper.wait_for_condition(lambda: all([getattr(collider, "collided_with_target_{}".format(index)) for collider in all_colliders]), TIME_OUT):
            failed_colliders = ", ".join([collider.name for collider in all_colliders if not getattr(collider, "collided_with_target_{}".format(index))])
            helper.fail_fast("A collision with target did not occur for these colliders: {}".format(failed_colliders))
        elif index == 1:
            for collider in all_colliders:
                Report.result(get_test(collider.name, "_collided"), collider.collided_with_target_0 and collider.collided_with_target_1)

        # 5) Allow time to hit trigger
        general.idle_wait_frames(PROPAGATION_FRAMES)

        # 6) Exit Game Mode
        helper.exit_game_mode(get_test("exit_game_mode_", str(index)))

    # Main Script
    helper.init_idle()
    # 1) Load the level
    helper.open_level("Physics", "Material_DefaultMaterialLibraryChangesWork")

    # 2) Setup targets and colliders
    terrain                   = Entity("terrain")
    target_character_rubber   = Entity("target_character_rubber")
    target_character_concrete = Entity("target_character_concrete")

    rubber_sphere      = Collider(name="rubber_sphere",      target=terrain)
    concrete_sphere    = Collider(name="concrete_sphere",    target=terrain)
    character_rubber   = Collider(name="character_rubber",   target=target_character_rubber)
    character_concrete = Collider(name="character_concrete", target=target_character_concrete)
    terrain_rubber     = Collider(name="terrain_rubber",     target=terrain)
    terrain_concrete   = Collider(name="terrain_concrete",   target=terrain)
    ragdoll_rubber     = Collider(name="ragdoll_rubber",     target=terrain)
    ragdoll_concrete   = Collider(name="ragdoll_concrete",   target=terrain)

    rubber_test_entities = [rubber_sphere, character_rubber, terrain_rubber, ragdoll_rubber]
    concrete_test_entities = [concrete_sphere, character_concrete, terrain_concrete, ragdoll_concrete]
    test_entities = rubber_test_entities + concrete_test_entities

    # 3) Run test 0
    test_run(index=0, all_colliders=test_entities)

    # 4) Edit Material Library
    Report.critical_result(Tests.material_library_updated, edit_material_library())

    # Wait for material library changes to propagate
    general.idle_wait_frames(PROPAGATION_FRAMES)

    # 5) Run test 1
    test_run(index=1, all_colliders=test_entities)

    # 6) Validate Results
    Report.result(Tests.concrete_material_changed, check_concrete_material_updated(concrete_test_entities))
    Report.result(Tests.rubber_material_changed, check_rubber_material_updated(rubber_test_entities))



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_DefaultMaterialLibraryChangesWork)
