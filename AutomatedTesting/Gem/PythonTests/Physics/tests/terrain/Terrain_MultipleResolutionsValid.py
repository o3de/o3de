"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID : C6032082
# Test Case Title : Verify multiple terrain resolutions are supported



# fmt: off
class Tests():
    enter_game_mode_128x128_1m                   = ("Entered game mode for 128x128, 1m resolution",                       "Failed to enter game mode for 128x128, 1m resolution")
    find_entities_128x128_1m                     = ("Entities found for 128x128, 1m resolution",                          "Entities not found for 128x128, 1m resolution")
    ball_on_terrain_collision_128x128_1m         = ("Ball on terrain collided for 128x128, 1m resolution",                "Ball on terrain did not collide for 128x128, 1m resolution")
    ball_outside_terrain_collision_128x128_1m    = ("Ball outside terrain did not collide for 128x128, 1m resolution",    "Ball outside terrain collided for 128x128, 1m resolution")
    exit_game_mode_128x128_1m                    = ("Exited game mode for 128x128, 1m resolution",                        "Couldn't exit game mode for 128x128, 1m resolution")
    
    enter_game_mode_512x512_2m                   = ("Entered game mode for 512x512, 2m resolution",                       "Failed to enter game mode for 512x512, 2m resolution")
    find_entities_512x512_2m                     = ("Entities found for 512x512, 2m resolution",                          "Entities not found for 512x512, 2m resolution")
    ball_on_terrain_collision_512x512_2m         = ("Ball on terrain collided for 512x512, 2m resolution",                "Ball on terrain did not collide for 512x512, 2m resolution")
    ball_outside_terrain_collision_512x512_2m    = ("Ball outside terrain did not collide for 512x512, 2m resolution",    "Ball outside terrain collided for 512x512, 2m resolution")
    exit_game_mode_512x512_2m                    = ("Exited game mode for 512x512, 2m resolution",                        "Couldn't exit game mode for 512x512, 2m resolution")
    
    enter_game_mode_1024x1024_32m                = ("Entered game mode for 1024x1024, 32m resolution",                    "Failed to enter game mode for 1024x1024, 32m resolution")
    find_entities_1024x1024_32m                  = ("Entities found for 1024x1024, 32m resolution",                       "Entities not found for 1024x1024, 32m resolution")
    ball_on_terrain_collision_1024x1024_32m      = ("Ball on terrain collided for 1024x1024, 32m resolution",             "Ball on terrain did not collide for 1024x1024, 32m resolution")
    ball_outside_terrain_collision_1024x1024_32m = ("Ball outside terrain did not collide for 1024x1024, 32m resolution", "Ball outside terrain collided for 1024x1024, 32m resolution")
    exit_game_mode_1024x1024_32m                 = ("Exited game mode for 1024x1024, 32m resolution",                     "Couldn't exit game mode for 1024x1024, 32m resolution")
    
    enter_game_mode_4096x4096_16m                = ("Entered game mode for 4096x4096, 16m resolution",                    "Failed to enter game mode for 4096x4096, 16m resolution")
    find_entities_4096x4096_16m                  = ("Entities found for 4096x4096, 16m resolution",                       "Entities not found for 4096x4096, 16m resolution")
    ball_on_terrain_collision_4096x4096_16m      = ("Ball on terrain collided for 4096x4096, 16m resolution",             "Ball on terrain did not collide for 4096x4096, 16m resolution")
    ball_outside_terrain_collision_4096x4096_16m = ("Ball outside terrain did not collide for 4096x4096, 16m resolution", "Ball outside terrain collided for 4096x4096, 16m resolution")
    exit_game_mode_4096x4096_16m                 = ("Exited game mode for 4096x4096, 16m resolution",                     "Couldn't exit game mode for 4096x4096, 16m resolution")
# fmt: on


def Terrain_MultipleResolutionsValid():

    """
    Summary:
    Verify multiple terrain resolutions are supported

    Level Description:
    Multiple levels with different resolutions are created with terrain components
    128x128, 1m
    512x512, 2m
    1024x1024, 32m
    4096x4096, 16m
    Terrain (entity) - Entity with PhysX Terrain component
    Ball On Terrain (entity) - Entity with rigid body, mesh and collider placed above the terrain
    Ball Outside Terrain (entity) - Entity with rigid body, mesh and collider placed outside the terrain

    Expected Behavior:
    Editor doesn't crash, and the terrain is successfully created.
    The level opens and game mode is entered. The terrain entity is loaded in and then validation occurs on that entity. We are checking
    the terrain by adding objects on the terrain and outside the terrain and verifying the collision between terrain and the object above it.
    Game mode is then exited. After all levels are checked the editor is closed.

    Test Steps:
    1) Repeat the below steps for each level
        1.1) Open level
        1.2) Enter game mode
        1.3) Retrieve and validate Terrain entity
        1.4) Verify collision between objects and the terrain
        1.5) Exit game mode
    2) Close the editor

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

    # Constants
    TIME_OUT = 3.0  # Wait maximum of 3 seconds for collsion to occur

    helper.init_idle()

    class Level:
        def __init__(
            self,
            name,
            enter_game_test,
            validate_entities_test,
            ball_on_terrain_collision,
            ball_outside_terrain_collision,
            exit_game_test,
        ):
            self.name = name
            self.test_enter_game_mode = enter_game_test
            self.test_validate_entities = validate_entities_test
            self.test_ball_on_terrain_collision = ball_on_terrain_collision
            self.test_ball_outside_terrain_collision = ball_outside_terrain_collision
            self.test_exit_game_mode = exit_game_test

    level_128 = Level(
        "Terrain_MultipleResolutionsValid_128x128_1m",
        Tests.enter_game_mode_128x128_1m,
        Tests.find_entities_128x128_1m,
        Tests.ball_on_terrain_collision_128x128_1m,
        Tests.ball_outside_terrain_collision_128x128_1m,
        Tests.exit_game_mode_128x128_1m,
    )
    level_512 = Level(
        "Terrain_MultipleResolutionsValid_512x512_2m",
        Tests.enter_game_mode_512x512_2m,
        Tests.find_entities_512x512_2m,
        Tests.ball_on_terrain_collision_512x512_2m,
        Tests.ball_outside_terrain_collision_512x512_2m,
        Tests.exit_game_mode_512x512_2m,
    )
    level_1024 = Level(
        "Terrain_MultipleResolutionsValid_1024x1024_32m",
        Tests.enter_game_mode_1024x1024_32m,
        Tests.find_entities_1024x1024_32m,
        Tests.ball_on_terrain_collision_1024x1024_32m,
        Tests.ball_outside_terrain_collision_1024x1024_32m,
        Tests.exit_game_mode_1024x1024_32m,
    )
    level_4096 = Level(
        "Terrain_MultipleResolutionsValid_4096x4096_16m",
        Tests.enter_game_mode_4096x4096_16m,
        Tests.find_entities_4096x4096_16m,
        Tests.ball_on_terrain_collision_4096x4096_16m,
        Tests.ball_outside_terrain_collision_4096x4096_16m,
        Tests.exit_game_mode_4096x4096_16m,
    )

    class Collision:
        ball_on_terrain_collision = False
        ball_outside_terrain_collision = False

    class CollisionHandler:
        def __init__(self, terrain_id, ball_inside_id, ball_outside_id):
            self.terrain_id = terrain_id
            self.ball_inside_id = ball_inside_id
            self.ball_outside_id = ball_outside_id
            self.create_collision_handler()

        def on_collision_begin(self, args):
            collider_id = args[0]
            if collider_id.Equal(self.ball_inside_id):
                Collision.ball_on_terrain_collision = True
            elif collider_id.Equal(self.ball_outside_id):
                Collision.ball_outside_terrain_collision = True

        def create_collision_handler(self):
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(self.terrain_id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)

    levels = (
        level_128,
        level_512,
        level_1024,
        level_4096,
    )

    # 1) Repeat the below steps for each level
    for level in levels:
        # 1.1) Open level
        helper.open_level("Physics", level.name)

        # 1.2) Enter game mode
        helper.enter_game_mode(level.test_enter_game_mode)

        # 1.3) Retrieve and validate Terrain entity
        terrain_id = general.find_game_entity("Terrain")
        ball_on_terrain_id = general.find_game_entity("Ball On Terrain")
        ball_outside_terrain_id = general.find_game_entity("Ball Outside Terrain")
        Report.critical_result(
            level.test_validate_entities,
            terrain_id.IsValid() and ball_on_terrain_id.IsValid() and ball_outside_terrain_id.IsValid(),
        )

        # 1.4) Verify collision between objects and the terrain
        CollisionHandler(terrain_id, ball_on_terrain_id, ball_outside_terrain_id)
        helper.wait_for_condition(lambda: Collision.ball_on_terrain_collision, TIME_OUT)
        Report.critical_result(level.test_ball_on_terrain_collision, Collision.ball_on_terrain_collision)
        Report.critical_result(level.test_ball_outside_terrain_collision, not Collision.ball_outside_terrain_collision)

        # Resetting collision values to false for the next level
        Collision.ball_on_terrain_collision = False
        Collision.ball_outside_terrain_collision = False

        # 1.5) Exit game mode
        helper.exit_game_mode(level.test_exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Terrain_MultipleResolutionsValid)
