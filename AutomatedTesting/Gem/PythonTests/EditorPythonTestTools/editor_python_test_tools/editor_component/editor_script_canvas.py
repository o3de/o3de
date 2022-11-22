"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

We are deprecating this file and moving the constants to area specific files. See the EditorPythonTestTools > consts
folder for more info
"""
from editor_python_test_tools.editor_entity_utils import EditorEntity, EditorComponent
import azlmbr.scriptcanvas as scriptcanvas
from enum import Enum
import azlmbr.math as math
from consts.scripting import (SCRIPT_CANVAS_UI)
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.legacy.general as general
import azlmbr.object

# component property path
SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH = "Configuration|Source"

# path parts
PROPERTIES = "Configuration|Properties|"
UNUSED_VARIABLES = "Unused Variables|"
VARIABLES = "Variables|"
DATUM_PATH = "Datum|Datum|value|"


class VariableState(Enum):
    UNUSEDVARIABLE = 0
    VARIABLE = 1


class ScriptCanvasComponent:

    def __init__(self):
        self.script_canvas_component = None

    def get_script_canvas_component(self) -> EditorComponent:
        return self.script_canvas_component

    def add_component_to_existing_entity(self, editor_entity: EditorEntity, sc_file_path: str) -> None:
        """
        Function for initializing a script canvas component object if you already have an editor entity you want to use.

        param editor_entity: The entity we want to configure
        param sc_file_path: location on disk to the script canvas file

        returns None
        """
        sourcehandle = scriptcanvas.SourceHandleFromPath(sc_file_path)

        self.script_canvas_component = editor_entity.add_component(SCRIPT_CANVAS_UI)
        self.script_canvas_component.set_component_property_value(SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH, sourcehandle)

    def set_component_graph_file_from_path(self, sc_file_path: str) -> None:
        """
        Function for updating the script canvas file being used by the component

        param sc_file_path : the string path the new graph file on disk

        returns None
        """
        sourcehandle = scriptcanvas.SourceHandleFromPath(sc_file_path)

        self.script_canvas_component.set_component_property_value(SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH, sourcehandle)
    def get_component_graph_file(self) -> str:
        """
        function for getting the component property value of the graph file in the script canvas component

        returns the component property value of the script canvas source handle (string)
        """

        return self.script_canvas_component.get_component_property_value(SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH)

    def set_variable_value(self, variable_name: str, variable_state: VariableState, variable_value) -> str:
        """
        Function for setting a value to a variable on the script canvas component.

        Note: The variable must be exposed to the component. this can be done through the script canvas node inspector.
        variables exposed to the component will be visible by name rather than type.

        param variable_name: the name of the variable
        param variable_state: used/unused variable
        param variable_value: the value to assign to the variable

        returns component property path as a string
        """
        component_property_path = self.__construct_variable_component_property_path(variable_name, variable_state)

        self.script_canvas_component.set_component_property_value(component_property_path, variable_value)

        return component_property_path

    def get_variable_value(self, variable_name: str, variable_state: VariableState):
        """
        Function for getting the value of an exposed variable off the script canvas component. The variable type
        can vary but only 2 types are currently supported: String adn Boolean. See github GH-13344 for more info.
        """

        component_property_path = self.__construct_variable_component_property_path(variable_name, variable_state)
        variable_value = self.script_canvas_component.get_component_property_value(component_property_path)

        return variable_value

    def __construct_variable_component_property_path(self, variable_name: str, variable_state: VariableState) -> str:
        """
        helper function for constructing a component property path for the value setting function

        param variable_name: the name of the variable being assigned
        param variable_state: used/unused variable

        returns a string for the component property path
        """

        component_property_path = PROPERTIES

        match variable_state:
            case VariableState.UNUSEDVARIABLE:
                component_property_path += UNUSED_VARIABLES
            case VariableState.VARIABLE:
                component_property_path += VARIABLES

        component_property_path += variable_name + "|"
        component_property_path += DATUM_PATH
        component_property_path += variable_name

        return component_property_path
