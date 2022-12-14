"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    game_mode_entered    = ("Successfully entered game mode",         "Failed to enter game mode")
    lines_found          = ("Successfully found expected prints",    "Failed to find expected prints")
    game_mode_exited     = ("Successfully exited game mode", "Failed to exit game mode")
# fmt: on


def setup_level_entities(dictionaries):
    """
    Helper function for setting up the 3 entities and their script canvas components needed for this test

    Incomming dictionaries use the format String : String and use the keys; name, status, path.
    name - the name of the test entity
    status - the active status of the entity
    path - the file path on disk to the script canvas file which will be added to the entity
    """
    import azlmbr.math as math
    import scripting_utils.scripting_tools as scripting_tools
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.editor_component.editor_script_canvas import ScriptCanvasComponent
    from editor_python_test_tools.editor_component.editor_script_canvas import VariableState
    from editor_python_test_tools.utils import TestHelper as TestHelper
    from scripting_utils.scripting_constants import (WAIT_TIME_3)

    position = math.Vector3(512.0, 512.0, 32.0)

    for entity_dictionary in dictionaries:

        #create entity, give it a script canvas component and set its active status
        editor_entity = EditorEntity.create_editor_entity_at(position, entity_dictionary["name"])
        scriptcanvas_component = ScriptCanvasComponent(editor_entity)
        scriptcanvas_component.set_component_graph_file_from_path(entity_dictionary["path"])
        scripting_tools.change_entity_start_status(entity_dictionary["name"], entity_dictionary["status"])
        TestHelper.wait_for_condition(lambda: editor_entity is not None, WAIT_TIME_3)

        # the controller entity needs extra steps to be set up properly
        if entity_dictionary["name"] == "Controller":

            # get the ids of the two other entities we created
            activated_entity_id = EditorEntity.find_editor_entity("ActivationTest").id
            deactivated_entity_id = EditorEntity.find_editor_entity("DeactivationTest").id
            # set the two variables on the sc component to point to the activated and deactivated entities
            activated_entity_var = "EntityToActivate"
            deactivated_entity_var = "EntityToDeactivate"

            scriptcanvas_component.set_variable_value(activated_entity_var, VariableState.VARIABLE, activated_entity_id)
            scriptcanvas_component.set_variable_value(deactivated_entity_var, VariableState.VARIABLE,
                                                          deactivated_entity_id)

def ScriptCanvasComponent_OnEntityActivatedDeactivated_PrintMessage():
    """
    Summary:
     Verify that the On Entity Activated/On Entity Deactivated nodes are working as expected

    Expected Behavior:
     Upon entering game mode, the Controller entity will wait 1 second and then activate the ActivationTest
     entity. The script attached to ActivationTest will print out a message on activation. The Controller
     will also deactivate the DeactivationTest entity, which should print a message.

    Test Steps:
     1) Create temp level
     2) Create all the entities we need for the test
     3) Start the Tracer
     4) Enter Game Mode
     5) Wait one second for graph timers then exit game mode
     6) Validate Print message

    Note:
     - Any passed and failed tests are written to the Editor.log file.

    :return: None
    """
    import os
    import azlmbr.paths as paths
    import scripting_utils.scripting_tools as scripting_tools
    from editor_python_test_tools.utils import TestHelper as TestHelper
    from editor_python_test_tools.utils import Report, Tracer
    import azlmbr.legacy.general as general
    from scripting_utils.scripting_constants import (WAIT_TIME_3, WAIT_TIME_1)

    controller_dict = {
        "name": "Controller",
        "status": "active",
        "path": os.path.join(paths.projectroot, "ScriptCanvas", "OnEntityActivatedScripts", "controller.scriptcanvas"),
    }
    activated_dict = {
        "name": "ActivationTest",
        "status": "inactive",
        "path": os.path.join(paths.projectroot, "ScriptCanvas", "OnEntityActivatedScripts", "activator.scriptcanvas"),
    }
    deactivated_dict = {
        "name": "DeactivationTest",
        "status": "active",
        "path": os.path.join(paths.projectroot, "ScriptCanvas", "OnEntityActivatedScripts", "deactivator.scriptcanvas"),
    }

    EXPECTED_LINES = ["Activator Script: Activated", "Deactivator Script: Deactivated"]

    # Preconditions
    general.idle_enable(True)

    # 1) Create temp level
    TestHelper.open_level("", "Base")

    # 2) create all the entities we need for the test
    dictionaries = [activated_dict, deactivated_dict, controller_dict]
    setup_level_entities(dictionaries)

    # 3) Start the Tracer
    with Tracer() as section_tracer:

        # 4) Enter Game Mode
        TestHelper.enter_game_mode(Tests.game_mode_entered)

        # 5) Wait one second for graph timers then exit game mode
        general.idle_wait(WAIT_TIME_1)
        TestHelper.exit_game_mode(Tests.game_mode_exited)

        # 6) Validate Print message
        found_expected_lines = scripting_tools.located_expected_tracer_lines(section_tracer, EXPECTED_LINES)
        TestHelper.wait_for_condition(lambda: found_expected_lines is not None, WAIT_TIME_3)

    Report.result(Tests.lines_found, found_expected_lines)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(ScriptCanvasComponent_OnEntityActivatedDeactivated_PrintMessage)
