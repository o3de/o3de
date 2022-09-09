"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Checks:
    enter_game_mode                = ("Entered game mode",              "Failed to enter game mode")
    no_rhi_validation_errors_found = ("No RHI validation errors found", "Found RHI validation errors")
    exit_game_mode                 = ("Exited game mode",               "Failed to exit game mode")

def RHIValidation_Vulkan_DefaultLevel():
    
    import azlmbr.legacy.general as general
    
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer
    
    from vulkan_skip_errors import VulkanValidationErrors
    
    # Constants
    FRAMES_IN_GAME_MODE = 200

    # 1) Start the Tracer to catch any errors and warnings
    with Tracer() as section_tracer:
        helper.init_idle()

        # 2) Load the level
        helper.open_level("", "DefaultLevel")

        # 3) Enter game mode
        helper.enter_game_mode(Checks.enter_game_mode)
        
        # 4) Wait in game mode some frames to let cloth simulation run
        general.idle_wait_frames(FRAMES_IN_GAME_MODE)
        
        # 5) Exit game mode
        helper.exit_game_mode(Checks.exit_game_mode)

    # 5) Verify there are no rhi validation errors in the logs
    has_rhi_validation_errors = False
    for error_msg in section_tracer.errors:
        if VulkanValidationErrors.CheckError(error_msg.window, f"{error_msg}"):
            has_rhi_validation_errors = True
            Report.info(f"RHI Validation error found: {error_msg}")
    Report.result(Checks.no_rhi_validation_errors_found, not has_rhi_validation_errors)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RHIValidation_Vulkan_DefaultLevel)
