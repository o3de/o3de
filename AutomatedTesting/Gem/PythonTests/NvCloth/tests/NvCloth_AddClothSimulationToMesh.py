"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C18977329
# Test Case Title : Add cloth simulation to a Mesh

# fmt: off
class Tests:
    enter_game_mode              = ("Entered game mode",              "Failed to enter game mode")
    no_errors_and_warnings_found = ("No errors and warnings found",   "Found errors and warnings")
    exit_game_mode               = ("Exited game mode",               "Failed to exit game mode")
# fmt: on

def NvCloth_AddClothSimulationToMesh():
    """
    Summary:
    Load level with Entity having Mesh and Cloth components already setup. Verify that editor remains stable in Game mode.

    Expected Behavior:
    The Editor is stable there are no warnings or errors.

    Test Steps:
     1) Load the level
     2) Start the Tracer to catch any errors and warnings
     3) Enter game mode
     4) Wait in game mode some frames to let cloth simulation run
     5) Verify there are no errors and warnings in the logs
     6) Exit game mode
     7) Close the editor

    Note:
    At the time of writing this test it was not possible to set a component property
    with a dropdown, which is necessary to setup a Cloth component. Because of this limitation
    the components of this test are not setup by the script and a level was provided instead.

    :return: None
    """
    
    import azlmbr.legacy.general as general
    
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report
    
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer
    
    # Constants
    FRAMES_IN_GAME_MODE = 200

    helper.init_idle()
    # 1) Load the level
    helper.open_level("NvCloth", "NvCloth_AddClothSimulationToMesh")

    # 2) Start the Tracer to catch any errors and warnings
    with Tracer() as section_tracer:
        # 3) Enter game mode
        helper.enter_game_mode(Tests.enter_game_mode)
        
        # 4) Wait in game mode some frames to let cloth simulation run
        general.idle_wait_frames(FRAMES_IN_GAME_MODE)

    # 5) Verify there are no errors and warnings in the logs
    success_condition = not (section_tracer.has_errors or section_tracer.has_warnings)
    Report.result(Tests.no_errors_and_warnings_found, success_condition)
    if not success_condition:
        if section_tracer.has_warnings:
            Report.info(f"Warnings found: {section_tracer.warnings}")
        if section_tracer.has_errors:
            Report.info(f"Errors found: {section_tracer.errors}")
        Report.failure(Tests.no_errors_and_warnings_found)

    # 6) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)

    # 7) Close the editor
    helper.close_editor()

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(NvCloth_AddClothSimulationToMesh)
