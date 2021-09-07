"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C14861498
# Test Case Title : Confirm that when a PhysXCollider has no physics asset, the physics asset collider \
#   shape throw an error



# fmt:off
class Tests():
    enter_game_mode        = ("Entered game mode",               "Failed to enter game mode")
    found_entity           = ("Entity was found",                "Entity WAS NOT found")
    warning_message_logged = ("The expected warning was logged", "The expected message was not logged")
    exit_game_mode         = ("Exited game mode",                "Couldn't exit game mode")
# fmt:on


def Collider_PxMeshErrorIfNoMesh():
    """
    Summary:
    This test looks for the presence of an error when an entity with a PhysXCollider has no physics mesh, but has its
    collider shape set to a physics mesh.

    Level Description:
    One entity with a PhysXCollider with the collider shape set to "physics asset" and no actual physics mesh.
    That's it!

    Steps:
    1) Load the level / enter game mode
    2) Find the entity
    3) Look for warning
    4) Exit game mode
    5) Close the editor

    [Log Monitor] make sure error lines are present in the log

    Expected Behavior:
    The editor should open, load the level and (seemingly) instantly close. The two error lines specified should
    print to the log.

    :return: None
    """
    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    import azlmbr.legacy.general as general

    helper.init_idle()

    with Tracer() as warning_tracer:
        def has_physx_warning():
            return warning_tracer.has_warnings and any(
                'PhysX' in warningInfo.window and
                'EditorColliderComponent' in warningInfo.message for warningInfo in warning_tracer.warnings)

        # 1) Load level / enter game mode
        helper.open_level("Physics", "Collider_PxMeshErrorIfNoMesh")
        helper.enter_game_mode(Tests.enter_game_mode)

        # 2) Find game entity
        id = general.find_game_entity("test_entity")
        Report.result(Tests.found_entity, id.IsValid())

        # 3) Look for warning
        helper.wait_for_condition(has_physx_warning, 1.0)
        Report.result(Tests.warning_message_logged, has_physx_warning())

        # 4) Exit game mode
        helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_PxMeshErrorIfNoMesh)
