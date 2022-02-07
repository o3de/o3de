"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def GradientSampling_GradientReferencesAddRemoveSuccessfully():
    """
    Summary:
    An existing gradient generator can be pinned and cleared to/from the Gradient Entity Id field

    Expected Behavior:
    Gradient generator is assigned to the Gradient Entity Id field.
    Gradient generator is removed from the field.

    Test Steps:
     1) Open level
     2) Create a new entity with components "Random Noise Gradient", "Gradient Transform Modifier" and "Box Shape"
     3) Create a new entity with Gradient Modifier's, pin and clear the random noise entity id to the Gradient Id
        field in Gradient Modifier

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import azlmbr.math as math
    import azlmbr.entity as EntityId

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def modifier_pin_clear_to_gradiententityid(modifier):
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        component_to_add = [modifier]
        gradient_modifier = hydra.Entity(modifier)
        gradient_modifier.create_entity(entity_position, component_to_add)
        gradient_modifier.get_set_test(0, "Configuration|Gradient|Gradient Entity Id", random_noise.id)
        entity = hydra.get_component_property_value(
            gradient_modifier.components[0], "Configuration|Gradient|Gradient Entity Id"
        )
        gradient_pinned_to_modifier = (
            f"Gradient Generator is pinned to the {modifier} successfully",
            f"Failed to pin Gradient Generator to the {modifier}"
        )
        Report.result(gradient_pinned_to_modifier, entity.Equal(random_noise.id))
        hydra.get_set_test(gradient_modifier, 0, "Configuration|Gradient|Gradient Entity Id", EntityId.EntityId())
        entity = hydra.get_component_property_value(
            gradient_modifier.components[0], "Configuration|Gradient|Gradient Entity Id"
        )
        gradient_cleared_from_modifier = (
            f"Gradient Generator is cleared from the {modifier} successfully",
            f"Failed to clear Gradient Generator from the {modifier}"
        )
        Report.result(gradient_cleared_from_modifier, entity.Equal(EntityId.EntityId()))

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # 2) Create a new entity with components "Random Noise Gradient", "Gradient Transform Modifier" and "Box Shape"
    entity_position = math.Vector3(125.0, 136.0, 32.0)
    components_to_add = ["Random Noise Gradient", "Gradient Transform Modifier", "Box Shape"]
    random_noise = hydra.Entity("Random_Noise")
    random_noise.create_entity(entity_position, components_to_add)

    # 3) Create a new entity with Gradient Modifier's, pin and clear the random noise entity id to the Gradient Id
    #    field in Gradient Modifier
    modifier_pin_clear_to_gradiententityid("Dither Gradient Modifier")
    modifier_pin_clear_to_gradiententityid("Invert Gradient Modifier")
    modifier_pin_clear_to_gradiententityid("Levels Gradient Modifier")
    modifier_pin_clear_to_gradiententityid("Posterize Gradient Modifier")
    modifier_pin_clear_to_gradiententityid("Smooth-Step Gradient Modifier")
    modifier_pin_clear_to_gradiententityid("Threshold Gradient Modifier")


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GradientSampling_GradientReferencesAddRemoveSuccessfully)
