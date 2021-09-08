"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt: off
class Tests():
    enter_game_mode            = ("Entered game mode",                       "Failed to enter game mode")
    find_destructable          = ("Destructable entity found",               "Destructable entity not found")
    find_root_actor            = ("Root actor of destructable entity found", "Root actor of destructable entity not found")
    root_actor_destroyed       = ("Root actor destroyed",                    "Root actor was not destroyed")
    child_actors_created       = ("Child actors created",                    "Child actors were not created")
    exit_game_mode             = ("Exited game mode",                        "Couldn't exit game mode")
# fmt: on


def base_run(damage_func):

    """
    Summary:
    Open a Project that already has 1 destructable entity and verify damage destructs it.

    Level Description:
    Destructable entity is in a level.

    Expected Behavior:
    Damage is applied to destructable entity. Destructable entity root actor
    should be destroyed and 2 child actors should be created to replace it.

    Test Steps:
     1) Open level and Enter game mode
     2) Retrieve and validate Entities
     3) Get the entity representing root actor of destructable entity
     4) Apply damage to the destructable entity
     5) Check root actor is destroyed
     6) Check child actors of root actor are created
     7) Exit game mode
     8) Close the editor

    :return: None
    """
    
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus

    from BlastUtils import BlastNotificationHandler

    # Constants
    TIMEOUT = 2.0

    helper.init_idle()

    # 1) Open level and Enter game mode
    helper.open_level("Blast", "Blast_ActorSplitsAfterDamage")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve and validate Entities
    destructable_id = general.find_game_entity("Destructable_Box")
    Report.critical_result(Tests.find_destructable, destructable_id.IsValid())

    # 3) Get the entity representing root actor of destructable entity
    destructable_initial_actors = azlmbr.destruction.BlastFamilyComponentRequestBus(azlmbr.bus.Event, "Get Actors Data", destructable_id)
    Report.critical_result(Tests.find_root_actor, len(destructable_initial_actors) == 1)
    destructable_initial_actor_id = destructable_initial_actors[0].EntityId
    initial_actor_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldTranslation", destructable_initial_actor_id)

    # 4) Apply damage to the destructable entity
    damage_func(destructable_id, initial_actor_position)

    family_handler = BlastNotificationHandler(destructable_id)

    # 5) Check root actor is destroyed
    helper.wait_for_condition(lambda: family_handler.actors_destroyed == 1, TIMEOUT)
    Report.result(Tests.root_actor_destroyed, family_handler.actors_destroyed == 1)

    # 6) Check child actors of root actor are created
    helper.wait_for_condition(lambda: family_handler.actors_created == 2, TIMEOUT)
    Report.result(Tests.child_actors_created, family_handler.actors_created == 2)

    # 7) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)

    # 8) Close the editor
    helper.close_editor()
