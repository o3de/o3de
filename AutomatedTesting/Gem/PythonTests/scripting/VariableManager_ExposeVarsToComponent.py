"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# stub for when save file code is available
SCRIPT_CANVAS_FILE_PATH = os.path.join(paths.projectroot, "ScriptCanvas", "test_file.scriptcanvas")
ENTITY_NAME = "SC_Component_Variable_Entity"
COMPONENT_LIST = ["Script Canvas"]

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
    from PySide2 import QtWidgets, QtTest, QtCore
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.QtPyO3DEEditor import QtPyO3DEEditor
    from scripting_utils.script_canvas_component import ScriptCanvasComponent, VariableState
    from scripting_utils.scripting_constants import (WAIT_TIME_3, SCRIPT_CANVAS_UI, VARIABLE_MANAGER_UI,
                                                     VARIABLE_MANAGER_QT, VARIABLE_PALETTE_QT, GRAPH_VARIABLES_QT,
                                                     ADD_BUTTON_QT, VARIABLE_TYPES)
    from editor_python_test_tools.QtPyScriptCanvasNodeInspector import BooleanCheckBoxValues
    import azlmbr.legacy.general as general
    import pyside_utils
    import azlmbr.math as math

    general.idle_enable(True)

    # 1) Open Script Canvas window
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
    boolean_type = VARIABLE_TYPES[0]
    variable_manager.create_new_variable(boolean_type)
    node_inspector.change_variable_initial_value_source(BooleanCheckBoxValues.On)

    # 6) Save the file to disk
    #imagine a save file function call here

    # 7) Create an entity w/ component in the O3DE editor and attach the file
    hydra_entity = qtpy_o3de_editor.make_new_hydra_entity(ENTITY_NAME, COMPONENT_LIST)
    #do we want an entity class that can contain component types?
    script_canvas_component = ScriptCanvasComponent(hydra_entity, SCRIPT_CANVAS_FILE_PATH)

    # 8) Verify the new variables are exposed properly by modifying one of them
    #need to work on variable manager for extracting variable name
    script_canvas_component.set_variable_value("Variable Name", VariableState.UNUSEDVARIABLE, BooleanCheckBoxValues.On)


test = VariableManager_ExposeVarsToComponent()
test.run_test()
