"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import time

ENTER_MSG = ("Entered game mode", "Failed to enter game mode")
EXIT_MSG = ("Exited game mode",  "Couldn't exit game mode")

class Timer:
    unit_divisor = 60
    hour_divisor = unit_divisor * unit_divisor

    def start(self):
        self._start_time = time.perf_counter()

    def log_time(self, message):
        from editor_python_test_tools.utils import Report

        elapsed_time = time.perf_counter() - self._start_time
        hours = int(elapsed_time / Timer.hour_divisor)
        minutes = int(elapsed_time % Timer.hour_divisor / Timer.unit_divisor)
        seconds = elapsed_time % Timer.unit_divisor

        Report.info(f'{message}: {hours:0>2d}:{minutes:0>2d}:{seconds:0>5.2f}\n')

def time_editor_level_loading(level_dir, level_name):

    """
    Summary:
    Time how long it takes to load an arbitrary level, entering game mode, and exiting game mode

    Level Description:
    Preferably a level with a large number of entities

    Expected Behavior:
    Level loads within a reasonable time frame e.i. doesn't trip the framework timeout

    Benchmark Steps:
     1) Time opening the level
     2) Time entering game mode
     3) Time exiting game mode
     4) Close the editor

    :return: None
    """
    from editor_python_test_tools.utils import TestHelper as helper

    timer = Timer()

    helper.init_idle()

    # 1) Open level
    timer.start()
    helper.open_level(level_dir, level_name)
    timer.log_time('Level load time')

    # 2) Time how long it takes to enter game mode
    timer.start()
    helper.enter_game_mode(ENTER_MSG)
    timer.log_time('Enter game mode')

    # 3) Exit game mode
    timer.start()
    helper.exit_game_mode(EXIT_MSG)
    timer.log_time('Exit game mode')

    # 4) Close the editor
    helper.close_editor()
