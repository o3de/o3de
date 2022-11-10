#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT+

from editor_python_test_tools.utils import Report
from editor_python_test_tools.editor_entity_utils import EditorComponent
from consts.general import ComponentPropertyVisibilityStates as PropertyVisibility

def _validate_xyz_is_float(x: float, y: float, z: float, error_message: str) -> None:
    '''
    Helper Function for Editor Components to test if passed in X, Y, Z used in editor component tests are
       all actually floats.

    Note: This function is not intended to be used at test execution time, and is structured as such.

    x: The x-point position to validate if it's a float.
    y: The y-point position to validate if it's a float.
    z: The z-point position to validate if it's a float.
    error_message: The error message to assert if values are not float.

    return: boolean result of testing whether each input is a float.
    '''
    assert isinstance(x, float) and isinstance(y, float) and isinstance(z, float), error_message

def _validate_property_visibility(component: EditorComponent, component_property_path: str, expected: str) -> None:
    """
    Helper function for Editor Components to validate the current visibility of the property is expected value.

    Valid values for Property Visibility found in EditorPythonTestTools.consts.general ComponentPropertyVisibilityStates
    """
    Report.info(f"Validating that the visibility for component property for a {component.get_component_name()} at "
                f"property path {component_property_path} is \"{expected}\"")
    assert expected in vars(PropertyVisibility).values(), \
        f"Expected value of {expected} was not an expected visibility state of: {vars(PropertyVisibility).values()}"

    visibility = component.get_property_visibility(component_property_path)
    assert visibility == expected, \
        f"Error: {component.get_component_name()}'s component property visibility found at {component_property_path} " \
        f"was set to \"{visibility}\" when \"{expected}\" was expected."
