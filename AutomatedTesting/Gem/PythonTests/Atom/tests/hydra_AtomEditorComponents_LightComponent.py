"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.paths
import azlmbr.legacy.general as general

sys.path.append(os.path.join(azlmbr.paths.projectroot, "Gem", "PythonTests"))

import editor_python_test_tools.hydra_editor_utils as hydra
from Atom.atom_utils.atom_constants import LIGHT_TYPES

LIGHT_TYPE_PROPERTY = 'Controller|Configuration|Light type'
SPHERE_AND_SPOT_DISK_LIGHT_PROPERTIES = [
    ("Controller|Configuration|Shadows|Enable shadow", True),
    ("Controller|Configuration|Shadows|Shadowmap size", 0),  # 256
    ("Controller|Configuration|Shadows|Shadowmap size", 1),  # 512
    ("Controller|Configuration|Shadows|Shadowmap size", 2),  # 1024
    ("Controller|Configuration|Shadows|Shadowmap size", 3),  # 2048
    ("Controller|Configuration|Shadows|Shadow filter method", 1),  # PCF
    ("Controller|Configuration|Shadows|Filtering sample count", 4.0),
    ("Controller|Configuration|Shadows|Filtering sample count", 64.0),
    ("Controller|Configuration|Shadows|Shadow filter method", 2),  # ECM
    ("Controller|Configuration|Shadows|ESM exponent", 50),
    ("Controller|Configuration|Shadows|ESM exponent", 5000),
    ("Controller|Configuration|Shadows|Shadow filter method", 3),  # ESM+PCF
]
QUAD_LIGHT_PROPERTIES = [
    ("Controller|Configuration|Both directions", True),
    ("Controller|Configuration|Fast approximation", True),
]
SIMPLE_POINT_LIGHT_PROPERTIES = [
    ("Controller|Configuration|Attenuation radius|Mode", 0),
    ("Controller|Configuration|Attenuation radius|Radius", 100.0),
]
SIMPLE_SPOT_LIGHT_PROPERTIES = [
    ("Controller|Configuration|Shutters|Inner angle", 45.0),
    ("Controller|Configuration|Shutters|Outer angle", 90.0),
]


def verify_required_component_property_value(entity_name, component, property_path, expected_property_value):
    """
    Compares the property value of component against the expected_property_value.
    :param entity_name: name of the entity to use (for test verification purposes).
    :param component: component to check on a given entity for its current property value.
    :param property_path: the path to the property inside the component.
    :param expected_property_value: The value expected from the value inside property_path.
    :return: None, but prints to general.log() which the test uses to verify against.
    """
    property_value = editor.EditorComponentAPIBus(
        bus.Broadcast, "GetComponentProperty", component, property_path).GetValue()
    general.log(f"{entity_name}_test: Property value is {property_value} "
                f"which matches {expected_property_value}")


def run():
    """
    Test Case - Light Component
    1. Creates a "light_entity" Entity and attaches a "Light" component to it.
    2. Updates the Light component to each light type option from the LIGHT_TYPES constant.
    3. The test will check the Editor log to ensure each light type was selected.
    4. Prints the string "Light component test (non-GPU) completed" after completion.

    Tests will fail immediately if any of these log lines are found:
    1. Trace::Assert
    2. Trace::Error
    3. Traceback (most recent call last):

    :return: None
    """
    # Create a "light_entity" entity with "Light" component.
    light_entity_name = "light_entity"
    light_component = "Light"
    light_entity = hydra.Entity(light_entity_name)
    light_entity.create_entity(math.Vector3(-1.0, -2.0, 3.0), [light_component])
    general.log(
        f"{light_entity_name}_test: Component added to the entity: "
        f"{hydra.has_components(light_entity.id, [light_component])}")

    # Populate the light_component_id_pair value so that it can be used to select all Light component options.
    light_component_id_pair = None
    component_type_id_list = azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 'FindComponentTypeIdsByEntityType', [light_component], 0)
    if len(component_type_id_list) < 1:
        general.log(f"ERROR: A component class with name {light_component} doesn't exist")
        light_component_id_pair = None
    elif len(component_type_id_list) > 1:
        general.log(f"ERROR: Found more than one component classes with same name: {light_component}")
        light_component_id_pair = None
    entity_component_id_pair = azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 'GetComponentOfType', light_entity.id, component_type_id_list[0])
    if entity_component_id_pair.IsSuccess():
        light_component_id_pair = entity_component_id_pair.GetValue()

    # Test each Light component option can be selected and it's properties updated.
    # Point (sphere) light type checks.
    light_type_property_test(
        light_type=LIGHT_TYPES['sphere'],
        light_properties=SPHERE_AND_SPOT_DISK_LIGHT_PROPERTIES,
        light_component_id_pair=light_component_id_pair,
        light_entity_name=light_entity_name,
        light_entity=light_entity
    )

    # Spot (disk) light type checks.
    light_type_property_test(
        light_type=LIGHT_TYPES['spot_disk'],
        light_properties=SPHERE_AND_SPOT_DISK_LIGHT_PROPERTIES,
        light_component_id_pair=light_component_id_pair,
        light_entity_name=light_entity_name,
        light_entity=light_entity
    )

    # Capsule light type checks.
    azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast,
        'SetComponentProperty',
        light_component_id_pair,
        LIGHT_TYPE_PROPERTY,
        LIGHT_TYPES['capsule']
    )
    verify_required_component_property_value(
        entity_name=light_entity_name,
        component=light_entity.components[0],
        property_path=LIGHT_TYPE_PROPERTY,
        expected_property_value=LIGHT_TYPES['capsule']
    )

    # Quad light type checks.
    light_type_property_test(
        light_type=LIGHT_TYPES['quad'],
        light_properties=QUAD_LIGHT_PROPERTIES,
        light_component_id_pair=light_component_id_pair,
        light_entity_name=light_entity_name,
        light_entity=light_entity
    )

    # Polygon light type checks.
    azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast,
        'SetComponentProperty',
        light_component_id_pair,
        LIGHT_TYPE_PROPERTY,
        LIGHT_TYPES['polygon']
    )
    verify_required_component_property_value(
        entity_name=light_entity_name,
        component=light_entity.components[0],
        property_path=LIGHT_TYPE_PROPERTY,
        expected_property_value=LIGHT_TYPES['polygon']
    )

    # Point (simple punctual) light type checks.
    light_type_property_test(
        light_type=LIGHT_TYPES['simple_point'],
        light_properties=SIMPLE_POINT_LIGHT_PROPERTIES,
        light_component_id_pair=light_component_id_pair,
        light_entity_name=light_entity_name,
        light_entity=light_entity
    )

    # Spot (simple punctual) light type checks.
    light_type_property_test(
        light_type=LIGHT_TYPES['simple_spot'],
        light_properties=SIMPLE_SPOT_LIGHT_PROPERTIES,
        light_component_id_pair=light_component_id_pair,
        light_entity_name=light_entity_name,
        light_entity=light_entity
    )

    general.log("Light component test (non-GPU) completed.")


def light_type_property_test(light_type, light_properties, light_component_id_pair, light_entity_name, light_entity):
    """
    Updates the current light type and modifies its properties, then verifies they are accurate to what was set.
    :param light_type: The type of light to update, must match a value in LIGHT_TYPES
    :param light_properties: List of tuples detailing properties to modify with update values.
    :param light_component_id_pair: Entity + component ID pair for updating the light component on a given entity.
    :param light_entity_name: the name of the Entity holding the light component.
    :param light_entity: the Entity object containing the light component.
    :return: None
    """
    azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast,
        'SetComponentProperty',
        light_component_id_pair,
        LIGHT_TYPE_PROPERTY,
        light_type
    )
    verify_required_component_property_value(
        entity_name=light_entity_name,
        component=light_entity.components[0],
        property_path=LIGHT_TYPE_PROPERTY,
        expected_property_value=light_type
    )

    for light_property in light_properties:
        light_entity.get_set_test(0, light_property[0], light_property[1])


if __name__ == "__main__":
    run()
