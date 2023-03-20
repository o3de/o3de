"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C29032500
# Test Case Title : Check that WorldRequestBus works with editor components



# fmt: off
class Tests():
    find_staticshapebox       = ("Found StaticShapeBox",                 "Failed to find StaticShapeBox")
    find_staticsphere         = ("Found StaticSphere",                   "Failed to find StaticSphere")
    find_staticbox            = ("Found StaticBox",                      "Failed to find StaticBox")
    find_staticcapsule        = ("Found StaticCapsule",                  "Failed to find StaticCapsule")
    find_staticmesh           = ("Found StaticMesh",                     "Failed to find StaticMesh")
    find_shapebox             = ("Found ShapeBox",                       "Failed to find ShapeBox")
    find_sphere               = ("Found Sphere",                         "Failed to find Sphere")
    find_box                  = ("Found Box",                            "Failed to find Box")
    find_capsule              = ("Found Capsule",                        "Failed to find Capsule")
    find_mesh                 = ("Found Mesh",                           "Failed to find Mesh")
    
    aabb_staticshapebox       = ("Correct AABB for StaticShapeBox",      "Incorrect AABB for StaticShapeBox")
    aabb_staticsphere         = ("Correct AABB for StaticSphere",        "Incorrect AABB for StaticSphere")
    aabb_staticbox            = ("Correct AABB for StaticBox",           "Incorrect AABB for StaticBox")
    aabb_staticcapsule        = ("Correct AABB for StaticCapsule",       "Incorrect AABB for StaticCapsule")
    aabb_staticmesh           = ("Correct AABB for StaticMesh",          "Incorrect AABB for StaticMesh")
    aabb_shapebox             = ("Correct AABB for ShapeBox",            "Incorrect AABB for ShapeBox")
    aabb_sphere               = ("Correct AABB for Sphere",              "Incorrect AABB for Sphere")
    aabb_box                  = ("Correct AABB for Box",                 "Incorrect AABB for Box")
    aabb_capsule              = ("Correct AABB for Capsule",             "Incorrect AABB for Capsule")
    aabb_mesh                 = ("Correct AABB for Mesh",                "Incorrect AABB for Mesh")
        
    raycast_staticshapebox    = ("Correct raycast for StaticShapeBox",   "Incorrect raycast for StaticShapeBox")
    raycast_staticsphere      = ("Correct raycast for StaticSphere",     "Incorrect raycast for StaticSphere")
    raycast_staticbox         = ("Correct raycast for StaticBox",        "Incorrect raycast for StaticBox")
    raycast_staticcapsule     = ("Correct raycast for StaticCapsule",    "Incorrect raycast for StaticCapsule")
    raycast_staticmesh        = ("Correct raycast for StaticMesh",       "Incorrect raycast for StaticMesh")
    raycast_shapebox          = ("Correct raycast for ShapeBox",         "Incorrect raycast for ShapeBox")
    raycast_sphere            = ("Correct raycast for Sphere",           "Incorrect raycast for Sphere")
    raycast_box               = ("Correct raycast for Box",              "Incorrect raycast for Box")
    raycast_capsule           = ("Correct raycast for Capsule",          "Incorrect raycast for Capsule")
    raycast_mesh              = ("Correct raycast for Mesh",             "Incorrect raycast for Mesh")
# fmt: on


def Physics_WorldBodyBusWorksOnEditorComponents():
    r"""
    Summary:
    Runs a test to make sure that a WorldBodyBus works property for components

    Level Description:
    - Dynamic
        - Sphere: Sphere with Rigid body 
        - Box: Box with Rigid body
        - Capsule: Capsule with Rigid body
        - Mesh: Sedan car Mesh with Rigid body
        - ShapeBox: Shape Collider component + Box with rigidBody
    - Static
        - StaticSphere: Sphere with only Coollider component 
        - StaticBox: Box with only Coollider component
        - StaticCapsule: Capsule with only Coollider component
        - StaticMesh: Sedan car Mesh with only Coollider component
        - StaticShapeBox: Only Shape Collider component + Box
    
    TopDown view:
                                                                  ____
          [!]               o          [ ]           ( )         (____)
       ShapeBox           Sphere       Box         Capsule        Mesh 
                                                                  ____
          [!]               o          [ ]           ( )         (____)
     StaticShapeBox    StaticSphere  StaticBox   StaticCapsule  StaticMesh 
                    
    Expected Outcome:
    Checks AABB and RayCast functions of WorldBodyBus against the All the entities in the level
    
    :return:
    """
    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.bus
    import math
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import vector3_str, aabb_str

    AABB_THRESHOLD = 0.01 # Entities won't move in the simulation 
                                                      
    helper.init_idle()
    helper.open_level("Physics", "Physics_WorldBodyBusWorksOnEditorComponents")
    
    def create_aabb(aabb_min_tuple, aabb_max_tuple):
        return azlmbr.math.Aabb_CreateFromMinMax(azlmbr.math.Vector3(aabb_min_tuple[0], aabb_min_tuple[1], aabb_min_tuple[2]),
                                                 azlmbr.math.Vector3(aabb_max_tuple[0], aabb_max_tuple[1], aabb_max_tuple[2]))
    
    class EntityData:
        def __init__(self, name, target_aabb):
            self.name = name
            self.target_aabb = target_aabb
    
    def get_test_tuple_for_entity(testprefix, entity_name):
        return Tests.__dict__[testprefix.lower() + "_" + entity_name.lower()]
    
    ENTITY_DATA = [ EntityData("ShapeBox",          create_aabb((509.82, 523.08, 32.81), (510.82, 524.08, 33.81))),
                    EntityData("Sphere",            create_aabb((509.82, 526.39, 32.81), (510.82, 527.39, 33.81))),
                    EntityData("Box",               create_aabb((509.82, 529.66, 32.81), (510.82, 530.66, 33.81))),
                    EntityData("Capsule",           create_aabb((510.07, 533.70, 32.81), (510.57, 534.20, 33.81))),
                    EntityData("Mesh",              create_aabb((509.48, 536.30, 33.31), (511.16, 540.38, 34.38))),
                    EntityData("StaticShapeBox",    create_aabb((512.08, 523.08, 32.81), (513.08, 524.08, 33.81))),
                    EntityData("StaticSphere",      create_aabb((512.08, 526.39, 32.81), (513.08, 527.39, 33.81))),
                    EntityData("StaticBox",         create_aabb((512.08, 529.66, 32.81), (513.08, 530.66, 33.81))),
                    EntityData("StaticCapsule",     create_aabb((512.33, 533.70, 32.81), (512.83, 534.20, 33.81))),
                    EntityData("StaticMesh",        create_aabb((511.74, 536.30, 33.31), (513.42, 540.38, 34.38))) ] # AABB data obtained by observation
                    
              
    for entity_data in ENTITY_DATA:
        entity_id = general.find_editor_entity(entity_data.name);
        Report.result(get_test_tuple_for_entity("find", entity_data.name), entity_id.IsValid())
        if entity_id.IsValid():            
            # AABB test
            aabb = azlmbr.physics.WorldBodyRequestBus(azlmbr.bus.Event, "GetAabb", entity_id)
            Report.info("%s AABB -> %s" % (entity_data.name, aabb_str(aabb)))
            Report.info("%s expected AABB -> %s" % (entity_data.name, aabb_str(entity_data.target_aabb)))
            is_expected_aabb_size = aabb.min.IsClose(entity_data.target_aabb.min, AABB_THRESHOLD) and aabb.max.IsClose(entity_data.target_aabb.max, AABB_THRESHOLD)
            Report.result(get_test_tuple_for_entity("aabb", entity_data.name), is_expected_aabb_size)
            
            # Raycast test
            entity_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTM", entity_id).GetPosition()
            raycast_request = azlmbr.physics.RayCastRequest()
            raycast_request.Start = entity_pos.Add(azlmbr.math.Vector3(0.0, 0.0, 100.0))
            raycast_request.Distance = 500.0
            raycast_request.Direction = azlmbr.math.Vector3(0.0, 0.0, -1.0)
            result = azlmbr.physics.WorldBodyRequestBus(azlmbr.bus.Event, "RayCast", entity_id, raycast_request)
            if result:
                # Following line crashes due to a hydra bug, use distance for now
                # has_hit = ragdoll_id.Equal(result.EntityId)
                has_hit = result.Distance > 0.1 and math.isclose(result.Position.x, entity_pos.x) and math.isclose(result.Position.y, entity_pos.y)
                Report.info("Hit: %s" % vector3_str(result.Position))
                Report.result(get_test_tuple_for_entity("raycast", entity_data.name), has_hit)
            else:
                Report.failure(get_test_tuple_for_entity("raycast", entity_data.name))
        
        


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Physics_WorldBodyBusWorksOnEditorComponents)
