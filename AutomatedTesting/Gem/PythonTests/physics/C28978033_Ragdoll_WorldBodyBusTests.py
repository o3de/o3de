"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""


# Test case ID : C28978033
# Test Case Title : Check that WorldRequestBus works with PhysX ragdoll
# URL of the test case : https://testrail.agscollab.com/index.php?/cases/view/28978033


# fmt: off
class Tests():
    enter_game_mode        = ("Entered game mode",                    "Failed to enter game mode")
    find_ragdoll           = ("Ragdoll found",                        "Ragdoll not found")
    aabb_correct           = ("Ragdoll AABB is correct",              "Ragdoll AABB is incorrect")
    raycast_ref_found      = ("Raycast reference entities found",     "Raycast reference entities not found")
    raycast_hit            = ("Raycast hit the ragdoll",              "Raycast didn't hit the ragdoll")
    exit_game_mode         = ("Exited game mode",                     "Failed to exit game mode")
# fmt: on


def C28978033_Ragdoll_WorldBodyBusTests():
    r"""
    Summary:
    Runs a test to make sure that a WorldBodyBus works property for ragdolls

    Level Description:
    - TestRagdoll: ragdoll entity in the position(500, 500, 50)
    - RayStart: entity that points where to start the raycast_correct
    - RayEnd: entity that points where to end the raycast_correct

                      | ( ) |
         o    - - -   |  ¦  |  - - > o
      RayStart        | / \ |      RayEnd
                    TestRagdoll
                    
    Expected Outcome:
    Once game mode is entered, the gravity is disabled, ragdoll AABB is checked and
    Raycast is done from RayStart to RayEnd. It should it hit the ragdoll.
    
    :return:
    """
    import os
    import sys

    import ImportPathHelper as imports

    imports.init()

    import azlmbr.legacy.general as general
    import azlmbr.bus
    from utils import Report
    from utils import TestHelper as helper
    from utils import vector3_str, aabb_str

    # Global time out
    TIME_OUT = 1.0
    
    EXPECTED_AABB = azlmbr.math.Aabb_CreateFromMinMax(azlmbr.math.Vector3(499.413513, 499.843201, 49.9907875),
                                                      azlmbr.math.Vector3(500.586487, 500.178009, 51.7091103)) # By observation

    AABB_THRESHOLD = 0.2 # Big threshold, even if we waited a single frame only, it can vary a lot in the simulation                                                 
                                                      
    # 1) Open level / Enter game mode
    helper.init_idle()
    helper.open_level("Physics", "C28978033_Ragdoll_WorldBodyBusTests")
    
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
    import ImportPathHelper as imports
    imports.init()

    from utils import Report
    Report.start_test(C28978033_Ragdoll_WorldBodyBusTests)
