"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C13508019
# Test Case Title : Verify terrain materials are updated after using terrain texture layer painter.


# fmt: off
class Tests:
    enter_game_mode           = ("Entered game mode",            "Failed to enter game mode")
    find_box_bounce           = ("Entity Bounce Box found",      "Bounce Box not found")
    find_box_no_bounce        = ("Entity No Bounce Box found",   "No Bounce Box not found")
    find_terrain              = ("Terrain found",                "Terrain not found")
    entities_actions_finished = ("Entity actions completed",     "Entity actions not completed")
    nonbouncy_box_not_bounced = ("Non-Bouncy Box didn't bounce", "Non-Bouncy Box did bounce")
    bouncy_box_not_bounced    = ("Bouncy Box did bounce",        "Bouncy Box didn't bounce")
    exit_game_mode            = ("Exited game mode",             "Couldn't exit game mode")
# fmt: on


def Terrain_TerrainTexturePainterWorks():
    # run() will open a a level and validate terrain material are updated after using terrain texture painter
    # It does this by:
    #   1) Opens level with a terrain that has been painted by two different physical materials with a box above each
    #   2) Enters Game mode
    #   3) Finds the entities in the scene
    #   5) Listens for boxes colliding with terrain
    #   7) Listens for boxes to exit(bounce) the collision or not (not bounce)
    #   8) Validate the results (The box above the blue terrain does not bounce and the box above the red terrain does)
    #   9) Exits game mode and editor

    # Expected Result: Both the boxes will be affected by the physical material on the terrain. One box will
    # bounce and the other wont.

    # Setup path
    import os, sys



    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    import azlmbr.bus

    TIME_OUT = 5.0  # Max time to complete test

    class Box:  # Holds box object attributes
        def __init__(self, boxID):
            self.box_id = boxID
            self.collided_with_terrain = False
            self.bounced = False

    helper.init_idle()

    # 1) Open level
    helper.open_level("Physics", "Terrain_TerrainTexturePainterWorks")

    # 2) Enter game mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # Creat box objects and set id's
    # Box that is above terrain that will bounce
    bounce_box = Box(general.find_game_entity("Box_Yes_Bounce"))
    Report.result(Tests.find_box_bounce, bounce_box.box_id.IsValid())

    # Box that is above terrain that will not bounce
    no_bounce_box = Box(general.find_game_entity("Box_No_Bounce"))
    Report.result(Tests.find_box_no_bounce, no_bounce_box.box_id.IsValid())

    terrain_id = general.find_game_entity("Terrain")
    Report.result(Tests.find_terrain, terrain_id.IsValid())

    #  Listen for a collision to begin
    def on_collision_begin(args):
        other_id = args[0]
        if other_id.Equal(no_bounce_box.box_id) and no_bounce_box.collided_with_terrain is False:
            Report.info("Non-Bouncy box collided with terrain ")
            no_bounce_box.collided_with_terrain = True
        if other_id.Equal(bounce_box.box_id) and bounce_box.collided_with_terrain is False:
            Report.info("Bouncy box collided with terrain ")
            bounce_box.collided_with_terrain = True

    #  Listen for a collision to end
    def on_collision_end(args):
        other_id = args[0]
        if other_id.Equal(no_bounce_box.box_id):
            no_bounce_box.bounced = True
        if other_id.Equal(bounce_box.box_id):
            bounce_box.bounced = True

    handler = azlmbr.physics.CollisionNotificationBusHandler()
    handler.connect(terrain_id)
    handler.add_callback("OnCollisionBegin", on_collision_begin)
    handler.add_callback("OnCollisionEnd", on_collision_end)

    # The test_entities_actions_completed function below returns a boolean of if the following actions happened:
    #   Did the bounce box hit the terrain
    #   Did the no bounce box hit the terrain
    #   Did the bounce box bounce
    #   Did the no bounce box not bounce
    def test_entities_actions_completed():
        return (
            bounce_box.collided_with_terrain
            and no_bounce_box.collided_with_terrain
            and bounce_box.bounced
            and not no_bounce_box.bounced  # not here because the no bounce box should not have bounced
        )

    entities_actions_finished = helper.wait_for_condition(test_entities_actions_completed, TIME_OUT)

    # Report is the entities in level finished their actions
    Report.result(Tests.entities_actions_finished, entities_actions_finished)

    # Report Info
    Report.info("Bouncy Box hit terrain: Expected = True  Actual = {}".format(bounce_box.collided_with_terrain))
    Report.info(
        "Non-Bouncy Box hit terrain: Expected = True  Actual = {}".format(no_bounce_box.collided_with_terrain)
    )
    Report.info("Did Bouncy Box bounce: Expected = True  Actual = {}".format(bounce_box.bounced))
    Report.info("Did Non-Bounce Box bounce: Expected = False  Actual = {}".format(no_bounce_box.bounced))

    # Check if the above test completed.
    if entities_actions_finished:
        Report.result(Tests.bouncy_box_not_bounced, bounce_box.bounced)
        Report.result(Tests.nonbouncy_box_not_bounced, not no_bounce_box.bounced)

    # 4) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Terrain_TerrainTexturePainterWorks)
