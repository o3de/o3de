"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C28978033
# Test Case Title : Check that WorldRequestBus works with PhysX ragdoll



# fmt: off
class Tests():
    enter_game_mode        = ("Entered game mode",                    "Failed to enter game mode")
    find_ragdoll           = ("Ragdoll found",                        "Ragdoll not found")
    aabb_correct           = ("Ragdoll AABB is correct",              "Ragdoll AABB is incorrect")
    raycast_ref_found      = ("Raycast reference entities found",     "Raycast reference entities not found")
    raycast_hit            = ("Raycast hit the ragdoll",              "Raycast didn't hit the ragdoll")
    exit_game_mode         = ("Exited game mode",                     "Failed to exit game mode")
# fmt: on


def Ragdoll_WorldBodyBusWorks():
    r"""
    Summary:
    Runs a test to make sure that a WorldBodyBus works property for ragdolls

    Level Description:
    - TestRagdoll: ragdoll entity in the position(500, 500, 50)
    - RayStart: entity that points where to start the raycast_correct
    - RayEnd: entity that points where to end the raycast_correct

                      | ( ) |
         o    - - -   |  |  |  - - > o
      RayStart        | / \ |      RayEnd
                    TestRagdoll
                    
    Expected Outcome:
    Once game mode is entered, the gravity is disabled, ragdoll AABB is checked and
    Raycast is done from RayStart to RayEnd. It should it hit the ragdoll.
    
    :return:
    """
    import os
    import sys
    import azlmbr.legacy.general as general
    import azlmbr.bus
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import vector3_str, aabb_str

    # Global time out
    TIME_OUT = 1.0
    
    EXPECTED_AABB = azlmbr.math.Aabb_CreateFromMinMax(azlmbr.math.Vector3(499.413513, 499.843201, 49.9907875),
                                                      azlmbr.math.Vector3(500.586487, 500.178009, 51.7091103)) # By observation

    AABB_THRESHOLD = 0.2 # Big threshold, even if we waited a single frame only, it can vary a lot in the simulation                                                 
                                                      
    # 1) Open level / Enter game mode
    helper.init_idle()
    helper.open_level("Physics", "Ragdoll_WorldBodyBusWorks")
    
    # Needs better solution
    general.idle_wait(6) # wait a little bit for ragdoll data to load
    
    helper.enter_game_mode(Tests.enter_game_mode)
    
    general.idle_wait_frames(1)
    
    gravity = azlmbr.math.Vector3(0.0, 0.0, 0.0)
    azlmbr.physics.WorldRequestBus(azlmbr.bus.Broadcast, "SetGravity", gravity)
    

    # 2) Retrieve entities and positions
    ragdoll_id = general.find_game_entity("TestRagdoll")
    Report.critical_result(Tests.find_ragdoll, ragdoll_id.IsValid())
    
    aabb = azlmbr.physics.WorldBodyRequestBus(azlmbr.bus.Event, "GetAabb", ragdoll_id)
    Report.info("Ragdoll AABB:" + aabb_str(aabb))
    Report.info("Target Ragdoll AABB:" + aabb_str(EXPECTED_AABB))
    
    is_expected_aabb_size = aabb.min.IsClose(EXPECTED_AABB.min, AABB_THRESHOLD) and aabb.max.IsClose(EXPECTED_AABB.max, AABB_THRESHOLD)
    Report.result(Tests.aabb_correct, is_expected_aabb_size)
    
    raystart_id = general.find_game_entity("RayStart")
    rayend_id = general.find_game_entity("RayEnd")
    Report.critical_result(Tests.raycast_ref_found, raystart_id.IsValid(), rayend_id.IsValid())
    
    raystart_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTM", raystart_id).GetPosition()
    rayend_pos = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTM", rayend_id).GetPosition()
    
    raycast_request = azlmbr.physics.RayCastRequest()
    raycast_request.Start = raystart_pos
    raycast_request.Distance = (rayend_pos.Subtract(raystart_pos)).GetLength()
    raycast_request.Direction = (rayend_pos.Subtract(raystart_pos)).GetNormalized()
    
    result = azlmbr.physics.WorldBodyRequestBus(azlmbr.bus.Event, "RayCast", ragdoll_id, raycast_request)
    
    # Following line crashes due to a hydra bug, use distance for now
    # has_hit_ragdoll = ragdoll_id.Equal(result.EntityId)
    has_hit_ragdoll = result.Distance > 0.1
    
    Report.result(Tests.raycast_hit, has_hit_ragdoll)
    
    # 4) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Ragdoll_WorldBodyBusWorks)
