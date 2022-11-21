"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# stub for when save file code is available
ENTITY_NAME = "SC_Component_Variable_Entity"
COMPONENT_LIST = ["Script Canvas"]
VARIABLE_NAME = "Test Boolean"

def VariableManager_ExposeVarsToComponent():
    """
    Summary:
     Test for verifying variables exposed to component can be accessed and modified in Editor

    Expected Behavior:
     Each variable type can be created and deleted in variable manager.

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Get the SC window object
     3) Open Variable Manager if not opened already
     4) Create Graph
     5) Create variable of each type and change their initial value source to from component
     6) Save the file
     7) Create an entity w/ component in the O3DE editor and attach the file
     8) Verify the new variables are accessible and can be modified.

    Note:
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    # Preconditions
    from editor_python_test_tools.QtPyO3DEEditor import QtPyO3DEEditor
    from scripting_utils.script_canvas_component import ScriptCanvasComponent, VariableState
    from editor_python_test_tools.QtPyCommon import CheckBoxStates
    import azlmbr.legacy.general as general
    from consts.scripting import (SCRIPT_CANVAS_TEST_FILE_PATH)
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.math as math

    general.idle_enable(True)

    # 1) Open the base level and then open Script Canvas editor
    TestHelper.open_level("", "Base")

    qtpy_o3de_editor = QtPyO3DEEditor()
    sc_editor = qtpy_o3de_editor.open_script_canvas()

    # 2) Get the SC window, variable manager and node inspector objects
    sc_editor.create_new_script_canvas_graph()
    variable_manager = sc_editor.variable_manager
    node_inspector = sc_editor.node_inspector

    # 3) Create Graph
    sc_editor.create_new_script_canvas_graph()

    # 5) Create variable of each type and change their initial value source to from component
    # The names of the variables will default to their type
    boolean_type = variable_manager.get_basic_variable_types().Boolean
    selection_index = variable_manager.create_new_variable(VARIABLE_NAME, boolean_type)

    # 6) Select the new variable in the variable manager so node inspector gets a target
    variable_manager.select_variable_from_table(selection_index)

    # 7) change the selected variable's intial value source to from Component (on)
    node_inspector.change_variable_initial_value_source(CheckBoxStates.On)

    # 8) Save the file to disk
    # This step requires engineering support. See github GH-12668

    # 8) Create an entity w/ component in the O3DE editor and attach the file
    position = math.Vector3(512.0, 512.0, 32.0)
    editor_entity = EditorEntity.create_editor_entity_at(position, ENTITY_NAME)
    editor_entity.add_components(COMPONENT_LIST)
    script_canvas_component = ScriptCanvasComponent(editor_entity, SCRIPT_CANVAS_TEST_FILE_PATH)

    # 9) Verify the new variables are exposed properly by modifying one of them
    script_canvas_component.set_variable_value(VARIABLE_NAME, VariableState.UNUSEDVARIABLE, True)


if __name__ == "__main__":

    import ImportPathHelper as imports

    imports.init()
    from editor_python_test_tools.utils import Report

    Report.start_test(VariableManager_ExposeVarsToComponent)
