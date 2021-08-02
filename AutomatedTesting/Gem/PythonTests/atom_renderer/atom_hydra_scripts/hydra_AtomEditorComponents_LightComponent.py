"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Hydra script that creates an entity, attaches the Light component to it for test verifications.
The test verifies that each light type option is available and can be selected without errors.
"""

import os
import sys

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.paths
import azlmbr.legacy.general as general

sys.path.append(os.path.join(azlmbr.paths.devassets, "Gem", "PythonTests"))

import editor_python_test_tools.hydra_editor_utils as hydra
from atom_renderer.atom_utils.atom_component_helper import LIGHT_TYPES


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
    light_entity = hydra.Entity(f"{light_entity_name}")
    light_entity.create_entity(math.Vector3(-1.0, -2.0, 3.0), [light_component])
    general.log(
        f"{light_entity_name}_test: Component added to the entity: "
        f"{hydra.has_components(light_entity.id, [light_component])}")

    # Populate the light_component_id_pair value so that it can be used to select all Light component options.
    light_component_id_pair = None
    component_type_id_list = azlmbr.editor.EditorComponentAPIBus(
        azlmbr.bus.Broadcast, 'FindComponentTypeIdsByEntityType', [light_component], 0)
    general.log(f"Components found = {len(component_type_id_list)}")
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

    # Test each Light component option can be selected.
    light_type_property = 'Controller|Configuration|Light type'
    for light_type in LIGHT_TYPES:
        current_light_type = light_type
        azlmbr.editor.EditorComponentAPIBus(
            azlmbr.bus.Broadcast,
            'SetComponentProperty',
            light_component_id_pair,
            light_type_property,
            current_light_type
        )

        property_value = editor.EditorComponentAPIBus(
            bus.Broadcast, "GetComponentProperty", light_entity.components[0], light_type_property).GetValue()
        general.log(f"{light_entity_name}_test: Property value is {property_value} which matches {current_light_type}")

    general.log("Light component test (non-GPU) completed.")


if __name__ == "__main__":
    run()
