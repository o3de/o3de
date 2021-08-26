"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""



def GradientSurfaceTagEmitter_ComponentDependencies():
    """
    Summary:
    This test verifies that the Gradient Surface Tag Emitter component is dependent on a gradient component.

    Expected Result:
    Gradient Surface Tag Emitter component is disabled until a Gradient Generator, Modifier or Gradient Reference
    component (and any sub-dependencies) is added to the entity.

    Test Steps:
     1) Open level
     2) Create a new entity with a Gradient Surface Tag Emitter component
     3) Verify the component is disabled until a dependent component is also added to the entity

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import azlmbr.math as math
    import azlmbr.bus as bus
    import azlmbr.entity as entity
    import azlmbr.editor as editor

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def is_enabled(EntityComponentIdPair):
        return editor.EditorComponentAPIBus(bus.Broadcast, "IsComponentEnabled", EntityComponentIdPair)

    # Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # Create an entity with Gradient Surface Tag Emitter component
    position = math.Vector3(512.0, 512.0, 32.0)
    gradient = hydra.Entity("gradient")
    gradient.create_entity(position, ["Gradient Surface Tag Emitter"])

    # Make sure Gradient Surface Tag Emitter is disabled
    gradient_surface_tag_disabled = (
        "Gradient Surface Tag Emitter is Disabled",
        "Gradient Surface Tag Emitter is Enabled, but should be Disabled without dependencies met"
    )
    Report.result(gradient_surface_tag_disabled, not is_enabled(gradient.components[0]))

    # Verify Gradient Surface Tag Emitter component is enabled after adding Gradient, Generator, Modifier
    # or Reference component
    new_components_to_add = [
        "Dither Gradient Modifier",
        "Gradient Mixer",
        "Invert Gradient Modifier",
        "Levels Gradient Modifier",
        "Posterize Gradient Modifier",
        "Smooth-Step Gradient Modifier",
        "Threshold Gradient Modifier",
        "Altitude Gradient",
        "Constant Gradient",
        "FastNoise Gradient",
        "Image Gradient",
        "Perlin Noise Gradient",
        "Random Noise Gradient",
        "Reference Gradient",
        "Shape Falloff Gradient",
        "Slope Gradient",
        "Surface Mask Gradient",
    ]
    for component in new_components_to_add:
        component_list = ["FastNoise Gradient", "Image Gradient", "Perlin Noise Gradient", "Random Noise Gradient"]
        if component in component_list:
            for Component in ["Gradient Transform Modifier", "Box Shape"]:
                hydra.add_component(Component, gradient.id)
        typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', [component],
                                                   entity.EntityType().Game)
        ComponentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', gradient.id, [typeIdsList[0]])
        Components = ComponentOutcome.GetValue()
        ComponentIdPair = Components[0]
        gradient_enabled = is_enabled(gradient.components[0])
        new_components_enabled = is_enabled(ComponentIdPair)
        dependencies_met = (
            f"{component} and Gradient Surface Tag Emitter are enabled",
            f"{component} and Gradient Surface Tag Emitter are disabled"
        )
        Report.result(dependencies_met, new_components_enabled and gradient_enabled)

        if component in component_list:
            hydra.remove_component("Gradient Transform Modifier", gradient.id)
            hydra.remove_component("Box Shape", gradient.id)
        hydra.remove_component(component, gradient.id)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GradientSurfaceTagEmitter_ComponentDependencies)
