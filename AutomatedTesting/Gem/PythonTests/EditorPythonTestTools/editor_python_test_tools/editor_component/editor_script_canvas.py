"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

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


class VariableState(Enum):
    UNUSEDVARIABLE = 0
    VARIABLE = 1


class Path:

    # component property path
    SCRIPT_CANVAS_ASSET_PROPERTY = "Configuration|Source"

    # path parts
    PROPERTIES = "Configuration|Properties|"
    UNUSED_VARIABLES = "Unused Variables|"
    VARIABLES = "Variables|"
    DATUM_PATH = "Datum|Datum|value|"

    def construct_variable_component_property_path(self, variable_name: str, variable_state: VariableState) -> str:
        """
        Function for constructing a component property path for the value setting function

        param variable_name: the name of the variable being assigned
        param variable_state: used/unused variable

        returns a string for the component property path
        """

        component_property_path = self.PROPERTIES

        match variable_state:
            case VariableState.UNUSEDVARIABLE:
                component_property_path += self.UNUSED_VARIABLES
            case VariableState.VARIABLE:
                component_property_path += self.VARIABLES

        component_property_path += variable_name + "|"
        component_property_path += self.DATUM_PATH
        component_property_path += variable_name

        return component_property_path

class ScriptCanvasComponent:

    def __init__(self, editor_entity: EditorEntity):
        self.script_canvas_component = editor_entity.add_component(SCRIPT_CANVAS_UI)

    def get_script_canvas_component(self) -> EditorComponent:
        return self.script_canvas_component

    def set_component_graph_file_from_path(self, sc_file_path: str) -> None:
        """
        Function for updating the script canvas file being used by the component

        param sc_file_path : the string path the new graph file on disk

        returns None
        """
        sourcehandle = scriptcanvas.SourceHandleFromPath(sc_file_path)

        self.script_canvas_component.set_component_property_value(Path.SCRIPT_CANVAS_ASSET_PROPERTY, sourcehandle)

    def get_component_graph_file(self) -> str:
        """
        function for getting the component property value of the graph file in the script canvas component

        returns the component property value of the script canvas source handle (string)
        """

        return self.script_canvas_component.get_component_property_value(Path.SCRIPT_CANVAS_ASSET_PROPERTY)

    def set_variable_value(self, variable_name: str, variable_state: VariableState, variable_value) -> str:
        """
        Function for setting a value to a variable on the script canvas component.
        o3de/o3de#13344: We currently only support strings, entity.id and boolean variables.

        Note: The variable must be exposed to the component. this can be done through the script canvas node inspector.
        variables exposed to the component will be visible by name rather than type.

        param variable_name: the name of the variable
        param variable_state: used/unused variable
        param variable_value: the value to assign to the variable

        returns component property path as a string
        """
        component_property_path = Path.construct_variable_component_property_path(Path, variable_name, variable_state)

        self.script_canvas_component.set_component_property_value(component_property_path, variable_value)

        return component_property_path

    def get_variable_value(self, variable_name: str, variable_state: VariableState):
        """
        o3de/o3de#13344. We currently only support strings and boolean variables.
        Function for getting the value of an exposed variable off the script canvas component. .
        """

        component_property_path = Path.construct_variable_component_property_path(Path, variable_name, variable_state)
        variable_value = self.script_canvas_component.get_component_property_value(component_property_path)

        return variable_value
