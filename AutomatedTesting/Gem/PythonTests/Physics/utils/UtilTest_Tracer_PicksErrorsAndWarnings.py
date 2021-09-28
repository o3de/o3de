"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt: off
class Tests():
    enter_game_mode = ("Entered game mode",                                                       "Failed to enter game mode")
    warning_found   = ("ScriptCanvas Warning found when running level containing warning entity", "ScriptCanvas Warning NOT found when running level containing warning entity")
    error_found     = ("PhysX Error found after openning level containing error entity",          "PhysX Error NOT found after openning level containing error entity")
    exit_game_mode  = ("Exited game mode",                                                        "Failed to exit game mode")
# fmt: on

def run():
    """
    Summary: 
    Checks that Tracer and the Trace notification EBus works propoerly.
    
    Level Tracer_WarningEntity:
        Contains an Entity with a Script Canvas component with non-connected nodes.
        When entering into gamemode, Script Canvas should raise a Warning.
        
    Level Tracer_ErrorEntity:
        Contains an Entity with a PhysX Collider Box with invalid extent (0, 1, 1).
        When opening the level, the collider should raise an Error.
        
    """
    
    import os
    import sys
    import azlmbr.legacy.general as general

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    helper.init_idle()
    
    with Tracer() as entity_warning_tracer:
        def has_scriptcanvas_warning():
            return entity_warning_tracer.has_warnings and any('Script Canvas' in warningInfo.window for warningInfo in entity_warning_tracer.warnings)
    
        helper.open_level("Utils", "Tracer_WarningEntity")
        helper.enter_game_mode(Tests.enter_game_mode)
        helper.wait_for_condition(has_scriptcanvas_warning, 1.0)
        helper.exit_game_mode(Tests.exit_game_mode)
    
    Report.result(Tests.warning_found, has_scriptcanvas_warning())
    
    with Tracer() as entity_error_tracer:
        def has_physx_error():
            return entity_error_tracer.has_errors and any('PhysX' in errorInfo.window for errorInfo in entity_error_tracer.errors)
    
        helper.open_level("Utils", "Tracer_ErrorEntity")
        helper.wait_for_condition(has_physx_error, 1.0)
    
    Report.result(Tests.error_found, has_physx_error())
    
if __name__ == "__main__":
    run()