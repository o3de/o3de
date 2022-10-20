"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

We are deprecating this file and moving the constants to area specific files. See the EditorPythonTestTools > consts
folder for more info
"""
import editor_python_test_tools.hydra_editor_utils as hydra
import azlmbr.scriptcanvas as scriptcanvas
from enum import Enum
import azlmbr.math as math

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
        self.hydra_entity = None
        self.sc_component = None

    def __init__(self, hydra_entity: hydra.Entity, sc_file_path: str):
        self.hydra_entity = None
        self.sc_component = None
        self.setup_SC_component_existing_entity(hydra_entity, sc_file_path)

    def setup_SC_component_new_entity(self, hydra_entity_name: str, sc_file_path: str,
                                      position=math.Vector3(512.0, 512.0, 32.0)) -> None:
        """
        Function for constructing the SCComponent object. This will make a new entity, add a script canvas component and
        then open a script canvas file from disk onto the component's source handle value.

        param hydra_entity_name: The name you want the entity to have
        param sc_file_path: The path on disk to the script canvas file
        param position: optional parameter for the position of the entity

        Returns None
        """
        sourcehandle = scriptcanvas.SourceHandleFromPath(sc_file_path)

        self.hydra_entity = hydra.Entity(hydra_entity_name)
        self.hydra_entity.create_entity(position, ["Script Canvas"])

        self.sc_component = self.hydra_entity.components[0]
        hydra.set_component_property_value(self.sc_component, SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH, sourcehandle)

    def setup_SC_component_existing_entity(self, hydra_entity: hydra.Entity, sc_file_path: str) -> None:
        """
        Function for constructing the SCComponent object. This uses an existing entity and loads a new script canvas
        file into the source handle value

        param hydra_entity_name: The name of the entity we want to configure
        param sc_file_path: location on disk to the script canvas file

        returns None
        """
        sourcehandle = scriptcanvas.SourceHandleFromPath(sc_file_path)

        self.hydra_entity = hydra_entity
        self.sc_component = self.hydra_entity.components[0]

        hydra.set_component_property_value(self.sc_component, SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH, sourcehandle)

    def set_variable_value(self, variable_name: str, variable_state: VariableState, variable_value) -> None:
        """
        Function for setting a value to a variable on the script canvas component.

        Note: The variable must be exposed to the component. this can be done through the script canvas node inspector.
        variables exposed to the component will be visible by name rather than type.

        param variable_name: the name of the variable
        param variable_state: used/unused variable
        param variable_value: the value to assign to the variable

        returns None
        """
        component_property_path = self.__make_variable_component_property_path(variable_name, variable_state)
        print(variable_value)
        hydra.set_component_property_value(self.sc_component, component_property_path, variable_value)

        #validate the change
        set_value = hydra.get_component_property_value(self.sc_component, component_property_path)
        assert set_value == variable_value, f"Component variable {variable_name} was not set properly"

    def __make_variable_component_property_path(self, variable_name: str, variable_state: VariableState) -> str:
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

        # test to see if this is a valid path
        valid_path = hydra.get_component_property_value(self.sc_component, component_property_path) is not None
        assert valid_path, "Path to variable was invalid! Check use/unused state or variable name"

        return component_property_path
