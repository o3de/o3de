#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT+
import typing
import azlmbr.asset as AzAsset
import azlmbr.math as Math
from editor_python_test_tools.utils import Report
from editor_python_test_tools.editor_entity_utils import EditorComponent
from consts.general import ComponentPropertyVisibilityStates as PropertyVisibility
from editor_python_test_tools.asset_utils import Asset
from editor_python_test_tools.editor_component.editor_script_canvas import VariableState, Path
from typing import Any

def compare_vec3(expected: Math.Vector3, actual: Math.Vector3) -> bool:
    """
    Helper function to compare two Vector3s. This is useful due to floating point math.
    expected: The expected Vector3 values
    actual: The actual Vector3 values

    return: boolean result of whether or not the two vector3s are the same.
    """
    x_is_close = Math.Math_IsClose(expected.get_property('x'), actual.get_property('x'), 0.01)
    y_is_close = Math.Math_IsClose(expected.get_property('y'), actual.get_property('y'), 0.01)
    z_is_close = Math.Math_IsClose(expected.get_property('z'), actual.get_property('z'), 0.01)

    return x_is_close and y_is_close and z_is_close


def _validate_xyz_is_float(x: float, y: float, z: float, error_message: str) -> None:
    """
    Helper Function for Editor Components to test if passed in X, Y, Z used in editor component tests are
       all actually floats.

    Note: This function is not intended to be used at test execution time, and is structured as such.

    x: The x-point position to validate if it's a float.
    y: The y-point position to validate if it's a float.
    z: The z-point position to validate if it's a float.
    error_message: The error message to assert if values are not float.

    return: boolean result of testing whether each input is a float.
    """
    assert isinstance(x, float) and isinstance(y, float) and isinstance(z, float), error_message


def _validate_property_visibility(component: EditorComponent, component_property_path: str, expected: str) -> None:
    """
    Helper function for Editor Components to validate the current visibility of the property is expected value.

    Valid values for Property Visibility found in EditorPythonTestTools.consts.general ComponentPropertyVisibilityStates
    """
    Report.info(f"Validating visibility for {component.get_component_name()} "
                f"component at property path {component_property_path} is \"{expected}\"")
    assert expected in vars(PropertyVisibility).values(), \
        f"Expected value of {expected} was not an expected visibility state of: {vars(PropertyVisibility).values()}"

    visibility = component.get_property_visibility(component_property_path)
    assert visibility == expected, \
        f"Error: {component.get_component_name()}'s component property visibility found at {component_property_path} " \
        f"was set to \"{visibility}\" when \"{expected}\" was expected."


def validate_integer_property(property_name: str, get_int_value: typing.Callable, set_int_value: typing.Callable,
                              component_name: str, tests: typing.Dict[str, typing.Tuple[int, bool]]) -> None:
    """
    Function to validate the behavior of a property that gets and sets an Integer value.
    :get_int_value: The Editor Entity Component Property's get method.
    :set_int_value: The Editor Entity Component Property's set method.
    :component_name: The name of the Editor Entity Component under test.
    :property_name: The name of the Editor Entity Component Property under test.
    :tests: A dictionary that stores an Int value and the intent of the values being passed. The following
    format is expected
     {
        "test_description": (int_value, expect_pass),
        "Zero value Test": (0, True)
     }
    """
    Report.info(f"Validating {component_name}'s {property_name} Integer property can be set or fails gracefully.")

    for test_name, test in tests.items():
        expected_value, expect_pass = test

        set_int_value(expected_value)
        set_value = get_int_value()

        assert (expected_value == set_value) is expect_pass, \
            f"Error: The {component_name}'s  {property_name} property failed to the \"{test_name}\" test. " \
            f"{expected_value} was expected but {set_value} was retrieved. Negative Scenario Test: {not expect_pass}."


def validate_float_property(property_name: str, get_float_value: typing.Callable, set_float_value: typing.Callable,
                            component_name: str, tests: typing.Dict[str, typing.Tuple[float, bool]]) -> None:
    """
    Function to validate the behavior of a property that gets and sets a Float value.

    :get_float_value: The Editor Entity Component Property's get method.
    :set_float_value: The Editor Entity Component Property's set method.
    :component_name: The name of the Editor Entity Component under test.
    :property_name: The name of the Editor Entity Component Property under test.
    :tests: A dictionary that stores a vector3 value and the intent of the values being passed. The following
    format is expected
     {
        "test_description": (float_value, expect_pass),
        "Zero value Test": (0.0, True)
     }
    """
    Report.info(f"Validating {component_name}'s {property_name} Float property can be set or fails gracefully.")

    for test_name, test in tests.items():
        expected_value, expect_pass = test

        set_float_value(expected_value)
        set_value = get_float_value()

        assert Math.Math_IsClose(expected_value, set_value, 0.001) is expect_pass, \
            f"Error: The {component_name}'s  {property_name} property failed to the \"{test_name}\" test. " \
            f"{expected_value} was expected but {set_value} was retrieved. Negative Scenario Test: {not expect_pass}."


def validate_vector3_property(property_name: str, get_vector3_value: typing.Callable,
                              set_vector3_value: typing.Callable, component_name: str,
                              tests: typing.Dict[str, typing.Tuple[float, float, float, bool]]) -> None:
    """
    Function to validate the behavior of a property that gets and sets a vector3.

    :get_vector3_value: The Editor Entity Component Property's get method.
    :set_vector3_value: The Editor Entity Component Property's set method.
    :component_name: The name of the Editor Entity Component under test.
    :property_name: The name of the Editor Entity Component Property under test.
    :tests: A dictionary that stores a vector3 value and the intent of the values being passed. The following
    format is expected
     {
        "test_description": (x_float_value, y_float_value, z_float_value, expect_pass),
        "Zero Value Test": (0.0, 0.0, 0.0, True)
     }
    """
    Report.info(f"Validating {component_name}'s {property_name} Vector3 property can be set or fails gracefully.")

    for test_name, test in tests.items():
        x, y, z, expect_pass = test
        expected_value = Math.Vector3(x, y, z)

        set_vector3_value(x, y, z)
        set_value = get_vector3_value()

        assert compare_vec3(expected_value, set_value) is expect_pass, \
            f"Error: The {component_name}'s  {property_name} property failed to the \"{test_name}\" test. " \
            f"{expected_value} was expected but {set_value} was retrieved. " \
            f"Negative Scenario Test: {not expect_pass}."


def validate_property_switch_toggle(property_name: str, get_toggle_value: typing.Callable,
                                    set_toggle_value: typing.Callable, component_name: str,
                                    restore_default: bool=True) -> None:
    """
    Used to toggle a property switch and validate that it toggled.
    param component_property_path: String of component property. (e.g. 'Settings|Visible')
    """
    Report.info(f"Validating {component_name}'s {property_name} property toggle switch toggles.")

    start_value = get_toggle_value()
    set_toggle_value(not start_value)

    end_value = get_toggle_value

    assert (start_value != end_value), \
        f"Error: {component_name}'s {property_name} property toggle switch did not toggle to " \
        f"{end_value} when it started at {start_value}"

    if restore_default:
        set_toggle_value(start_value)

def validate_asset_property(property_name: str, get_asset_value: typing.Callable, set_asset_value: typing.Callable,
                            component_name: str, asset_path: str) -> None:
    """
    Function to validate the behavior of a property that gets and sets an Asset value.

    :get_asset_value: The Editor Entity Component Property's get method.
    :set_asset_value: The Editor Entity Component Property's set method.
    :component_name: The name of the Editor Entity Component under test.
    :property_name: The name of the Editor Entity Component Property under test.
    :asset_path: The relative path to an asset in the asset cache.
    """
    Report.info(f"Validating {component_name}'s {property_name} Asset property can be set.")

    set_asset_value(asset_path)

    set_value = get_asset_value()
    expected_asset = Asset.find_asset_by_path(asset_path)

    assert expected_asset.id.is_equal(expected_asset.id, set_value), \
        f"Error: The {component_name}'s {property_name} property failed to properly set the mesh. Asset Id: " \
        f"{expected_asset.id} was expected but Asset Id: {set_value} was retrieved from \"{expected_asset.get_path()}\""


def validate_script_canvas_graph_file(get_script_canvas_graph_file: typing.Callable,
                                      set_script_canvas_graph_file: typing.Callable, sc_file_path: str) -> None:
    """
    Function to validate the setting of a script canvas graph file to the file source field in the script canvas component

    get_script_canvas_graph_file: getter for the script canvas component file source handle field
    set_script_canvas_graph_file: setter for the script canvas component's file source field.
    sc_file_path: the path on disk where the graph file exists

    """
    Report.info(f"Validating Script Canvas component's file source field can be set.")

    old_value = get_script_canvas_graph_file()

    set_script_canvas_graph_file(sc_file_path)

    set_value = get_script_canvas_graph_file()
    assert (set_value != old_value) and set_value is not None, f"Graph file could not be set!"


def validate_script_canvas_variable_changed(get_variable_value: typing.Callable, set_variable_value: typing.Callable,
                                            variable_name: str, variable_state: VariableState, expected_variable_value: Any) -> None:
    """
    Function for validating that a script canvas component variable can be changed.

    get_variable_value: getter for variables on the script canvas component
    set_variable_value: setter for variables on the script canvas component
    variable_name: the name of the variable to change
    variable_state: whether the variable is initialized or not within graph file(initialized by the script canvas editor).
    variable_value: the value to set the variable to. has no rigid type since variables can be primitives or user created types.
    o3de/o3de#13344. We currently only support strings and boolean variables.


    """
    Report.info(f"Validating Script Canvas component's variable can be set. This will seek out the variable in the "
                f"script canvas component's exposed variable list and set data to it.")

    set_variable_value(variable_name, variable_state, expected_variable_value)

    new_variable_value = get_variable_value(variable_name, variable_state)

    assert expected_variable_value == new_variable_value, f"Component variable {variable_name} was not set properly. " \
                                                          f"check the variable's name, state or incomming type being set."
