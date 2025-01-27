#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import os

import azlmbr.legacy.general as azgeneral

def RunLevelReloadTest(levels: list[str],
                       iterationsCount: int = 3,
                       iterationsEnterExit: int = 1,
                       idleFramesOpen: int = 3,
                       idleFramesEnter: int = 3,
                       idleFramesExit: int = 3):
    """
    For each level name listed in @levels:
    1- Loads the level
    2- Waits (if requested)
    3- Enters/Exit Game mode as many times defined in @iterationsEnterExit
    
    Repeats all of the above @iterationsCount times.
    """
    for idx in range(iterationsCount):
        print(f"Iteration {idx}")
        for levelName in levels:
            azgeneral.open_level_no_prompt(levelName)
            if idleFramesOpen > 0:
                azgeneral.idle_wait_frames(idleFramesOpen)
            for jdx in range(iterationsEnterExit):
                print(f"Iteration {idx}. Enter/Exit {jdx}")
                azgeneral.enter_game_mode()
                if idleFramesEnter > 0:
                    azgeneral.idle_wait_frames(idleFramesEnter)
                azgeneral.exit_game_mode()
                if idleFramesExit > 0:
                    azgeneral.idle_wait_frames(idleFramesExit)


# Quick Example on how to run this test using default levels from AutomatedTesting for 10 iterations:
# pyRunFile C:\GIT\o3de\Gems\EditorPythonBindings\Editor\Scripts\level_load_unload_stress.py -i 10
# pyRunFile C:\GIT\o3de\Gems\EditorPythonBindings\Editor\Scripts\level_load_unload_stress.py -i 10 --level CommsCenter,CypunkAptInterior,PoliceStation,TempleOfEnlightment
# pyRunFile C:\GIT\o3de\Gems\EditorPythonBindings\Editor\Scripts\level_load_unload_stress.py -n 1 -x 1 --level CommsCenter,CypunkAptInterior,PoliceStation,TempleOfEnlightment
def MainFunc():
    import argparse

    parser = argparse.ArgumentParser(
        description="Level loading test that validates that there are no crashes due to changing levels quickly."
    )

    parser.add_argument(
        "-i",
        "--iterations",
        type=int,
        default=3,
        help="How many times will load all levels in the list",
    )

    parser.add_argument(
        "-e",
        "--enter_exit_iterations",
        type=int,
        default=2,
        help="How many times will enter and exit game mode, for each level in the list.",
    )

    parser.add_argument(
        "-o",
        "--idle_frames_open",
        type=int,
        default=3,
        help="How many frames to wait after opening a level",
    )

    parser.add_argument(
        "-n",
        "--idle_frames_enter",
        type=int,
        default=3,
        help="How many frames to wait after entering game mode.",
    )

    parser.add_argument(
        "-x",
        "--idle_frames_exit",
        type=int,
        default=3,
        help="How many frames to wait after exiting game mode.",
    )

    parser.add_argument(
        "--levels",
        default="Graphics/hermanubis,Graphics/macbeth_shaderballs",
        help="Comma seperated list of level names.",
    )

    args = parser.parse_args()
    iterations = args.iterations
    iterationsEnterExit = args.enter_exit_iterations
    idleFramesOpen = args.idle_frames_open
    idleFramesEnter = args.idle_frames_enter
    idleFramesExit = args.idle_frames_exit
    levels = args.levels.split(",")
    print(f"Original levels list:\n{levels}")
    nonEmptyNames = []
    for levelName in levels:
        if levelName:
            nonEmptyNames.append(levelName)
    print(f"Valid levels list:\n{nonEmptyNames}")
    RunLevelReloadTest(nonEmptyNames, iterations, iterationsEnterExit, idleFramesOpen, idleFramesEnter, idleFramesExit)
    print(f"PASSED. Completed {iterations} iterations!")

if __name__ == "__main__":
    MainFunc()
