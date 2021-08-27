"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def GradientModifiers_Incompatibilities():
    """
    Summary:
    This test verifies that components are disabled when conflicting components are present on the same entity.

    Expected Behavior:
    Gradient Modifier components are incompatible with Vegetation area components.

    Test Steps:
     1) Open a simple level
     2) Create a new entity in the level
     3) Add each Gradient Modifier component to an entity, and add a Vegetation Area component to the same entity
     4) Verify that components are only enabled when entity is free of a conflicting component

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.entity as entity

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    gradient_generators = [
        'Altitude Gradient',
        'Constant Gradient',
        'FastNoise Gradient',
        'Image Gradient',
        'Perlin Noise Gradient',
        'Random Noise Gradient',
        'Shape Falloff Gradient',
        'Slope Gradient',
        'Surface Mask Gradient'
    ]
    require_transform_modifiers = [
        'FastNoise Gradient',
        'Image Gradient',
        'Perlin Noise Gradient',
        'Random Noise Gradient'
    ]
    gradient_modifiers = [
        'Dither Gradient Modifier',
        'Gradient Mixer',
        'Invert Gradient Modifier',
        'Levels Gradient Modifier',
        'Posterize Gradient Modifier',
        'Smooth-Step Gradient Modifier',
        'Threshold Gradient Modifier'
    ]
    vegetation_areas = [
        'Vegetation Layer Spawner',
        'Vegetation Layer Blender',
        'Vegetation Layer Blocker',
        'Vegetation Layer Blocker (Mesh)'
    ]
    area_dependencies = {
        'Vegetation Layer Spawner': 'Vegetation Asset List',
        'Vegetation Layer Blocker (Mesh)': 'Mesh'
    }

    # Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # For every gradient modifier component, verify that they are incompatible
    # which each vegetation area and gradient generator/modifier component
    all_gradients = gradient_modifiers + gradient_generators
    for component_name in gradient_modifiers:
        for vegetation_area_name in vegetation_areas:
            # Create a new Entity in the level
            entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

            # Most of these need a shape, so use a Box Shape
            hydra.add_component('Box Shape', entity_id)

            # Add the specific vegetation area dependencies (if necessary)
            if vegetation_area_name in area_dependencies:
                hydra.add_component(area_dependencies[vegetation_area_name], entity_id)

            # Add the vegetation area component we are validating against, then add the
            # gradient modifier afterwards, so that the gradient modifier will actually
            # be disabled (if it was present before, it would only get deactivated instead of disabled
            # by the vegetation area)
            area_component = hydra.add_component(vegetation_area_name, entity_id)
            gradient_component = hydra.add_component(component_name, entity_id)

            # Verify the gradient modifier component is disabled since the vegetation area is incompatible
            active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', gradient_component)
            component_is_disabled = (
                f"{component_name} is disabled before removing {vegetation_area_name} component",
                f"{component_name} is unexpectedly enabled before removing {vegetation_area_name} component"
            )
            Report.result(component_is_disabled, not active)

            # Remove the vegetation area component
            editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', [area_component])

            # Verify the gradient modifier component is enabled now that the vegetation area is gone
            active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', gradient_component)
            component_is_enabled = (
                f"{component_name} is enabled after removing {vegetation_area_name} component",
                f"{component_name} is unexpectedly disabled after removing {vegetation_area_name} component"
            )
            Report.result(component_is_enabled, active)

        for gradient_name in all_gradients:
            # Create a new Entity in the level
            entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

            # Most of these need a shape, so use a Box Shape
            hydra.add_component('Box Shape', entity_id)

            # Add the specific gradient generator dependencies (if necessary)
            conflicting_components = []
            if gradient_name in require_transform_modifiers:
                component = hydra.add_component('Gradient Transform Modifier', entity_id)
                conflicting_components.append(component)

            # Add the gradient component we are validating against, then add the
            # gradient modifier afterwards, so that the gradient modifier will actually
            # be disabled (if it was present before, it would only get deactivated instead of disabled
            # by the other gradient)
            component = hydra.add_component(gradient_name, entity_id)
            conflicting_components.append(component)
            gradient_component = hydra.add_component(component_name, entity_id)

            # Verify the gradient modifier component is disabled since the other gradient is incompatible
            active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', gradient_component)
            component_is_disabled = (
                f"{component_name} is disabled before removing {gradient_name} component",
                f"{component_name} is unexpectedly enabled before removing {gradient_name} component"
            )
            Report.result(component_is_disabled, not active)

            # Remove the conflicting gradient component (and transform modifier if it was added)
            editor.EditorComponentAPIBus(bus.Broadcast, 'RemoveComponents', conflicting_components)

            # Verify the gradient modifier component is enabled now that the other gradient is gone
            active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', gradient_component)
            component_is_enabled = (
                f"{component_name} is enabled after removing {gradient_name} component",
                f"{component_name} is unexpectedly disabled after removing {gradient_name} component"
            )
            Report.result(component_is_enabled, active)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GradientModifiers_Incompatibilities)
