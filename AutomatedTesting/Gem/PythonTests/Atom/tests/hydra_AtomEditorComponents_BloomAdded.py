"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    bloom_creation = (
        "Bloom Entity successfully created",
        "P0: Bloom Entity failed to be created")
    bloom_component = (
        "Entity has a Bloom component",
        "P0: Entity failed to find Bloom component")
    bloom_component_removal = (
        "Bloom component successfully removed",
        "P0: Bloom component failed to be removed")
    removal_undo = (
        "UNDO Bloom component removal success",
        "P0: UNDO Bloom component removal failed")
    bloom_disabled = (
        "Bloom component disabled",
        "P0: Bloom component was not disabled")
    postfx_layer_component = (
        "Entity has a PostFX Layer component",
        "P0: Entity did not have an PostFX Layer component")
    bloom_enabled = (
        "Bloom component enabled",
        "P0: Bloom component was not enabled")
    enable_bloom_parameter_enabled = (
        "Enable Bloom parameter enabled",
        "P0: Enable Bloom parameter was not enabled")
    enable_bloom_parameter_disabled = (
        "Enable Bloom parameter disabled",
        "P1: Enable Bloom parameter was not disabled")
    enabled_override_parameter_enabled = (
        "Enabled Override parameter enabled",
        "P1: Enabled Override parameter was not enabled")
    enabled_override_parameter_disabled = (
        "Enabled Override parameter disabled",
        "P1: Enabled Override parameter was not disabled")
    threshold_override_min = (
        "Threshold Override parameter set to minimum value",
        "P1: Threshold Override parameter failed to be set to minimum value")
    threshold_override_max = (
        "Threshold Override parameter set to maximum value",
        "P1: Threshold Override parameter failed to be set to maximum value")
    knee_override_min = (
        "Knee Override parameter set to minimum value",
        "P1: Knee Override parameter failed to be set to minimum value")
    knee_override_max = (
        "Knee Override parameter set to maximum value",
        "P1: Knee Override parameter failed to be set to maximum value")
    intensity_override_min = (
        "Intensity Override parameter set to minimum value",
        "P1: Intensity Override parameter failed to be set to minimum value")
    intensity_override_max = (
        "Intensity Override parameter set to maximum value",
        "P1: Intensity Override parameter failed to be set to maximum value")
    bicubicenabled_parameter_disabled = (
        "BicubicEnabled Override parameter disabled",
        "P1: BicubicEnabled Override parameter was not disabled")
    bicubicenabled_parameter_enabled = (
        "BicubicEnabled Override parameter enabled",
        "P1: BicubicEnabled Override parameter was not enabled")
    kernelsizescale_override_min = (
        "KernelSizeScale Override parameter set to minimum value",
        "P1: KernelSizeScale Override parameter failed to be set to minimum value")
    kernelsizescale_override_max = (
        "KernelSizeScale Override parameter set to maximum value",
        "P1: KernelSizeScale Override parameter failed to be set to maximum value")
    kernelsizestage0_override_min = (
        "KernelSizeStage0 Override parameter set to minimum value",
        "P1: KernelSizeStage0 Override parameter failed to be set to minimum value")
    kernelsizestage0_override_max = (
        "KernelSizeStage0 Override parameter set to maximum value",
        "P1: KernelSizeStage0 Override parameter failed to be set to maximum value")
    kernelsizestage1_override_min = (
        "KernelSizeStage1 Override parameter set to minimum value",
        "P1: KernelSizeStage1 Override parameter failed to be set to minimum value")
    kernelsizestage1_override_max = (
        "KernelSizeStage1 Override parameter set to maximum value",
        "P1: KernelSizeStage1 Override parameter failed to be set to maximum value")
    kernelsizestage2_override_min = (
        "KernelSizeStage2 Override parameter set to minimum value",
        "P1: KernelSizeStage2 Override parameter failed to be set to minimum value")
    kernelsizestage2_override_max = (
        "KernelSizeStage2 Override parameter set to maximum value",
        "P1: KernelSizeStage2 Override parameter failed to be set to maximum value")
    kernelsizestage3_override_min = (
        "KernelSizeStage3 Override parameter set to minimum value",
        "P1: KernelSizeStage3 Override parameter failed to be set to minimum value")
    kernelsizestage3_override_max = (
        "KernelSizeStage3 Override parameter set to maximum value",
        "P1: KernelSizeStage3 Override parameter failed to be set to maximum value")
    kernelsizestage4_override_min = (
        "KernelSizeStage4 Override parameter set to minimum value",
        "P1: KernelSizeStage0 Override parameter failed to be set to minimum value")
    kernelsizestage4_override_max = (
        "KernelSizeStage4 Override parameter set to maximum value",
        "P1: KernelSizeStage4 Override parameter failed to be set to maximum value")
    tintstage0_override_min = (
        "TintStage0 Override parameter set to minimum value",
        "P1: TintStage0 Override parameter failed to be set to minimum value")
    tintstage0_override_max = (
        "TintStage0 Override parameter set to maximum value",
        "P1: TintStage0 Override parameter failed to be set to maximum value")
    tintstage1_override_min = (
        "TintStage1 Override parameter set to minimum value",
        "P1: TintStage1 Override parameter failed to be set to minimum value")
    tintstage1_override_max = (
        "TintStage1 Override parameter set to maximum value",
        "P1: TintStage1 Override parameter failed to be set to maximum value")
    tintstage2_override_min = (
        "TintStage2 Override parameter set to minimum value",
        "P1: TintStage2 Override parameter failed to be set to minimum value")
    tintstage2_override_max = (
        "TintStage2 Override parameter set to maximum value",
        "P1: TintStage2 Override parameter failed to be set to maximum value")
    tintstage3_override_min = (
        "TintStage3 Override parameter set to minimum value",
        "P1: TintStage3 Override parameter failed to be set to minimum value")
    tintstage3_override_max = (
        "TintStage3 Override parameter set to maximum value",
        "P1: TintStage3 Override parameter failed to be set to maximum value")
    tintstage4_override_min = (
        "TintStage4 Override parameter set to minimum value",
        "P1: TintStage4 Override parameter failed to be set to minimum value")
    tintstage4_override_max = (
        "TintStage4 Override parameter set to maximum value",
        "P1: TintStage4 Override parameter failed to be set to maximum value")
    threshold_min = (
        "Threshold parameter set to minimum value",
        "P1: Threshold parameter failed to be set to minimum value")
    threshold_high = (
        "Threshold parameter set to a large value",
        "P1: Threshold parameter failed to be set to a large value")
    knee_min = (
        "Knee parameter set to minimum value",
        "P1: Knee parameter failed to be set to minimum value")
    knee_max = (
        "Knee parameter set to maximum value",
        "P1: Knee parameter failed to be set to maximum value")
    intensity_min = (
        "Intensity parameter set to minimum value",
        "P1: Intensity parameter failed to be set to minimum value")
    intensity_max = (
        "Intensity parameter set to maximum value",
        "P1: Intensity parameter failed to be set to maximum value")
    enable_bicubic_parameter_enabled = (
        "Enable Bicubic parameter enabled",
        "P1: Enable Bicubic parameter was not enabled")
    enable_bicubic_parameter_disabled = (
        "Enable Bicubic parameter disabled",
        "P1: Enable Bicubic parameter was not disabled")
    kernel_size_scale_min = (
        "KernelSize Scale parameter set to minimum value",
        "P1: Kernel Size Scale parameter failed to be set to minimum value")
    kernel_size_scale_max = (
        "KernelSize Scale parameter set to maximum value",
        "P1: Kernel Size Scale parameter failed to be set to maximum value")
    kernel_size_0_min = (
        "Kernel Size 0 parameter set to minimum value",
        "P1: Kernel Size 0 parameter failed to be set to minimum value")
    kernel_size_0_max = (
        "Kernel Size 0 parameter set to maximum value",
        "P1: Kernel Size 0 Override parameter failed to be set to maximum value")
    kernel_size_1_min = (
        "Kernel Size 1 parameter set to minimum value",
        "P1: Kernel Size 1 parameter failed to be set to minimum value")
    kernel_size_1_max = (
        "Kernel Size 1 parameter set to maximum value",
        "P1: Kernel Size 1 parameter failed to be set to maximum value")
    kernel_size_2_min = (
        "Kernel Size 2 parameter set to minimum value",
        "P1: Kernel Size 2 parameter failed to be set to minimum value")
    kernel_size_2_max = (
        "Kernel Size 2 parameter set to maximum value",
        "P1: Kernel Size 2 parameter failed to be set to maximum value")
    kernel_size_3_min = (
        "Kernel Size 3 parameter set to minimum value",
        "P1: Kernel Size 3 parameter failed to be set to minimum value")
    kernel_size_3_max = (
        "Kernel Size 3 parameter set to maximum value",
        "P1: Kernel Size 3 parameter failed to be set to maximum value")
    kernel_size_4_min = (
        "Kernel Size 4 parameter set to minimum value",
        "P1: Kernel Size 4 parameter failed to be set to minimum value")
    kernel_size_4_max = (
        "Kernel Size 4 parameter set to maximum value",
        "P1: Kernel Size 4 parameter failed to be set to maximum value")
    edit_tint_0 = (
        "Tint 0 parameter set to red",
        "P1: Tint 0 parameter failed to be set to red")
    edit_tint_1 = (
        "Tint 1 parameter set to green",
        "P1: Tint 1 parameter failed to be set to green")
    edit_tint_2 = (
        "Tint 2 parameter set to blue",
        "P1: Tint 2 parameter failed to be set to blue")
    edit_tint_3 = (
        "Tint 3 parameter set to yellow",
        "P1: Tint 3 parameter failed to be set to yellow")
    edit_tint_4 = (
        "Tint 4 parameter set to violet",
        "P1: Tint 4 parameter failed to be set to violet")
    enter_game_mode = (
        "Entered game mode",
        "P0: Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "P0: Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "P0: Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "P0: Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "P0: Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "P0: UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "P0: REDO deletion failed")


def AtomEditorComponents_Bloom_AddedToEntity():
    """
    Summary:
    Tests the Bloom component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an Bloom entity with no components.
    2) Add Bloom component to Bloom entity.
    3) Remove the Bloom component.
    4) Undo Bloom component removal.
    5) Verify Bloom component not enabled.
    6) Add PostFX Layer component since it is required by the Bloom component.
    7) Verify Bloom component is enabled.
    8) Enable the "Enable Bloom" parameter.
    9) Disable the "Enable Bloom" parameter (re-enable after test for game mode verification).
    10) Disable the Enabled Override parameter.
    11) Enable the Enabled Override parameter.
    12) Update the Threshold Override parameter to minimum value.
    13) Update the Threshold Override parameter to maximum value.
    14) Update the Knee Override parameter to minimum value.
    15) Update the Knee Override parameter to maximum value.
    16) Update the Intensity Override parameter to minimum value.
    17) Update the Intensity Override parameter to maximum value.
    18) Disable the BicubicEnabled Override parameter.
    19) Enable the BicubicEnabled Override parameter.
    20) Update the KernelSizeScale Override parameter to minimum value.
    21) Update the KernelSizeScale Override parameter to maximum value.
    22) Update the KernelSizeStage0 Override parameter to minimum value.
    23) Update the KernelSizeStage0 Override parameter to maximum value.
    24) Update the KernelSizeStage1 Override parameter to minimum value.
    25) Update the KernelSizeStage1 Override parameter to maximum value.
    26) Update the KernelSizeStage2 Override parameter to minimum value.
    27) Update the KernelSizeStage2 Override parameter to maximum value.
    28) Update the KernelSizeStage3 Override parameter to minimum value.
    29) Update the KernelSizeStage3 Override parameter to maximum value.
    30) Update the KernelSizeStage4 Override parameter to minimum value.
    31) Update the KernelSizeStage4 Override parameter to maximum value.
    32) Update the TintStage0 Override parameter to minimum value.
    33) Update the TintStage0 Override parameter to maximum value.
    34) Update the TintStage1 Override parameter to minimum value.
    35) Update the TintStage1 Override parameter to maximum value.
    36) Update the TintStage2 Override parameter to minimum value.
    37) Update the TintStage2 Override parameter to maximum value.
    38) Update the TintStage3 Override parameter to minimum value.
    39) Update the TintStage3 Override parameter to maximum value.
    40) Update the TintStage4 Override parameter to minimum value.
    41) Update the TintStage4 Override parameter to maximum value.
    42) Update the Threshold parameter to minimum value.
    43) Update the Threshold parameter to a large value.
    44) Update the Knee parameter to minimum value.
    45) Update the Knee parameter to maximum value.
    46) Update the Intensity parameter to minimum value.
    47) Update the Intensity parameter to maximum value.
    48) Enable the "Enable Bicubic" parameter.
    49) Disable the "Enable Bicubic" parameter (re-enable after test for game mode verification).
    50) Update the Kernel Size Scale parameter to minimum value.
    51) Update the Kernel Size Scale parameter to maximum value.
    52) Update the Kernel Size 0 parameter to minimum value.
    53) Update the Kernel Size 0 parameter to maximum value.
    54) Update the Kernel Size 1 parameter to minimum value.
    55) Update the Kernel Size 1 parameter to maximum value.
    56) Update the Kernel Size 2 parameter to minimum value.
    57) Update the Kernel Size 2 parameter to maximum value.
    58) Update the Kernel Size 3 parameter to minimum value.
    59) Update the Kernel Size 3 parameter to maximum value.
    60) Update the Kernel Size 4 parameter to minimum value.
    61) Update the Kernel Size 4 parameter to maximum value.
    62) Edit the Tint 0 parameter.
    63) Edit the Tint 1 parameter.
    64) Edit the Tint 2 parameter.
    65) Edit the Tint 3 parameter.
    66) Edit the Tint 4 parameter.
    67) Enter/Exit game mode.
    68) Test IsHidden.
    69) Test IsVisible.
    70) Delete Bloom entity.
    71) UNDO deletion.
    72) REDO deletion.
    73) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create an Bloom entity with no components.
        bloom_entity = EditorEntity.create_editor_entity(AtomComponentProperties.bloom())
        Report.critical_result(Tests.bloom_creation, bloom_entity.exists())

        # 2. Add Bloom component to Bloom entity.
        bloom_component = bloom_entity.add_component(AtomComponentProperties.bloom())
        Report.critical_result(Tests.bloom_component, bloom_entity.has_component(AtomComponentProperties.bloom()))

        # 3. Remove the Bloom component.
        bloom_component.remove()
        general.idle_wait_frames(1)
        Report.critical_result(Tests.bloom_component_removal,
                               not bloom_entity.has_component(AtomComponentProperties.bloom()))

        # 4. Undo Bloom component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.removal_undo, bloom_entity.has_component(AtomComponentProperties.bloom()))

        # 5. Verify Bloom component not enabled.
        Report.result(Tests.bloom_disabled, not bloom_component.is_enabled())

        # 6. Add PostFX Layer component since it is required by the Bloom component.
        bloom_entity.add_component(AtomComponentProperties.postfx_layer())
        general.idle_wait_frames(1)
        Report.result(
            Tests.postfx_layer_component,
            bloom_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify Bloom component is enabled.
        Report.result(Tests.bloom_enabled, bloom_component.is_enabled())

        # 8. Enable the "Enable Bloom" parameter.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Enable Bloom'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enable_bloom_parameter_enabled,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Enable Bloom')) is True)

        # 9. Disable the "Enable Bloom" parameter (re-enable after test for game mode verification).
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Enable Bloom'), False)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enable_bloom_parameter_disabled,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Enable Bloom')) is False)
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Enable Bloom'), True)

        # 10. Disable the Enabled Override parameter.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Enabled Override'), False)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enabled_override_parameter_disabled,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Enabled Override')) is False)

        # 11. Enable the Enabled Override parameter.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Enabled Override'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enabled_override_parameter_enabled,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Enabled Override')) is True)

        # 12. Update the Threshold Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Threshold Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.threshold_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Threshold Override')) == 0.0)

        # 13. Update the Threshold Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Threshold Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.threshold_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Threshold Override')) == 1.0)

        # 14. Update the Knee Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Knee Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.knee_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Knee Override')) == 0.0)

        # 15. Update the Knee Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Knee Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.knee_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Knee Override')) == 1.0)

        # 16. Update the Intensity Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Intensity Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.intensity_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Intensity Override')) == 0.0)

        # 17. Update the Intensity Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Intensity Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.intensity_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Intensity Override')) == 1.0)

        # 18. Disable the BicubicEnabled Override parameter.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'BicubicEnabled Override'), False)
        general.idle_wait_frames(1)
        Report.result(
            Tests.bicubicenabled_parameter_disabled,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'BicubicEnabled Override')) is False)

        # 19. Enable the BicubicEnabled Override parameter.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'BicubicEnabled Override'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.bicubicenabled_parameter_enabled,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'BicubicEnabled Override')) is True)

        # 20. Update the KernelSizeScale Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'KernelSizeScale Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernelsizescale_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'KernelSizeScale Override')) == 0.0)

        # 21. Update the KernelSizeScale Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'KernelSizeScale Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernelsizescale_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'KernelSizeScale Override')) == 1.0)

        # 22. Update the KernelSizeStage0 Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'KernelSizeStage0 Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernelsizestage0_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'KernelSizeStage0 Override')) == 0.0)

        # 23. Update the KernelSizeStage0 Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'KernelSizeStage0 Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernelsizestage0_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'KernelSizeStage0 Override')) == 1.0)

        # 24. Update the KernelSizeStage1 Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'KernelSizeStage1 Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernelsizestage1_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'KernelSizeStage1 Override')) == 0.0)

        # 25. Update the KernelSizeStage1 Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'KernelSizeStage1 Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernelsizestage1_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'KernelSizeStage1 Override')) == 1.0)

        # 26. Update the KernelSizeStage2 Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'KernelSizeStage2 Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernelsizestage2_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'KernelSizeStage2 Override')) == 0.0)

        # 27. Update the KernelSizeStage2 Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'KernelSizeStage2 Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernelsizestage2_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'KernelSizeStage2 Override')) == 1.0)

        # 28. Update the KernelSizeStage3 Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'KernelSizeStage3 Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernelsizestage3_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'KernelSizeStage3 Override')) == 0.0)

        # 29. Update the KernelSizeStage3 Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'KernelSizeStage3 Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernelsizestage3_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'KernelSizeStage3 Override')) == 1.0)

        # 30. Update the KernelSizeStage4 Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'KernelSizeStage4 Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernelsizestage4_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'KernelSizeStage4 Override')) == 0.0)

        # 31. Update the KernelSizeStage4 Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'KernelSizeStage4 Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernelsizestage4_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'KernelSizeStage4 Override')) == 1.0)

        # 32. Update the TintStage0 Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'TintStage0 Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.tintstage0_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'TintStage0 Override')) == 0.0)

        # 33. Update the TintStage0 Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'TintStage0 Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.tintstage0_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'TintStage0 Override')) == 1.0)

        # 34. Update the TintStage1 Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'TintStage1 Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.tintstage1_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'TintStage1 Override')) == 0.0)

        # 35. Update the TintStage1 Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'TintStage1 Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.tintstage1_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'TintStage1 Override')) == 1.0)

        # 36. Update the TintStage2 Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'TintStage2 Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.tintstage2_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'TintStage2 Override')) == 0.0)

        # 37. Update the TintStage2 Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'TintStage2 Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.tintstage2_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'TintStage2 Override')) == 1.0)

        # 38. Update the TintStage3 Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'TintStage3 Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.tintstage3_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'TintStage3 Override')) == 0.0)

        # 39. Update the TintStage3 Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'TintStage3 Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.tintstage3_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'TintStage3 Override')) == 1.0)

        # 40. Update the TintStage4 Override parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'TintStage4 Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.tintstage4_override_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'TintStage4 Override')) == 0.0)

        # 41. Update the TintStage4 Override parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'TintStage4 Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.tintstage4_override_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'TintStage4 Override')) == 1.0)

        # 42. Update the Threshold parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Threshold'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.threshold_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Threshold')) == 0.0)

        # 43. Update the Threshold parameter to a large value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Threshold'), 1000.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.threshold_high,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Threshold')) == 1000.0)

        # 44. Update the Knee parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Knee'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.knee_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Knee')) == 0.0)

        # 45. Update the Knee parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Knee'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.knee_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Knee')) == 1.0)

        # 46. Update the Intensity parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Intensity'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.intensity_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Intensity')) == 0.0)

        # 47. Update the Intensity parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Intensity'), 10000.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.intensity_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Intensity')) == 10000.0)

        # 48. Enable the "Enable Bicubic" parameter.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Enable Bicubic'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enable_bicubic_parameter_enabled,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Enable Bicubic')) is True)

        # 49. Disable the "Enable Bicubic" parameter (re-enable after test for game mode verification).
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Enable Bicubic'), False)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enable_bicubic_parameter_disabled,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Enable Bicubic')) is False)
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Enable Bicubic'), True)

        # 50. Update the Kernel Size Scale parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Kernel Size Scale'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernel_size_scale_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Kernel Size Scale')) == 0.0)

        # 51. Update the Kernel Size Scale parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Kernel Size Scale'), 2.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernel_size_scale_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Kernel Size Scale')) == 2.0)

        # 52. Update the Kernel Size 0 parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Kernel Size 0'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernel_size_0_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Kernel Size 0')) == 0.0)

        # 53. Update the Kernel Size 0 parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Kernel Size 0'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernel_size_0_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Kernel Size 0')) == 1.0)

        # 54. Update the Kernel Size 1 parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Kernel Size 1'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernel_size_1_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Kernel Size 1')) == 0.0)

        # 55. Update the Kernel Size 1 parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Kernel Size 1'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernel_size_1_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Kernel Size 1')) == 1.0)

        # 56. Update the Kernel Size 2 parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Kernel Size 2'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernel_size_2_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Kernel Size 2')) == 0.0)

        # 57. Update the Kernel Size 2 parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Kernel Size 2'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernel_size_2_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Kernel Size 2')) == 1.0)

        # 58. Update the Kernel Size 3 parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Kernel Size 3'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernel_size_3_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Kernel Size 3')) == 0.0)

        # 59. Update the Kernel Size 3 parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Kernel Size 3'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernel_size_3_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Kernel Size 3')) == 1.0)

        # 60. Update the Kernel Size 4 parameter to minimum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Kernel Size 4'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernel_size_4_min,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Kernel Size 4')) == 0.0)

        # 61. Update the Kernel Size 4 parameter to maximum value.
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Kernel Size 4'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.kernel_size_4_max,
            bloom_component.get_component_property_value(AtomComponentProperties.bloom(
                'Kernel Size 4')) == 1.0)

        # 62. Edit the Tint 0 parameter.
        red_color_value = math.Vector3(1.0, 0.0, 0.0)
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Tint 0'), red_color_value)
        general.idle_wait_frames(1)
        tint_color_0 = bloom_component.get_component_property_value(
            AtomComponentProperties.bloom('Tint 0'))
        Report.result(Tests.edit_tint_0, tint_color_0.IsClose(red_color_value))

        # 63. Edit the Tint 1 parameter.
        green_color_value = math.Vector3(0.0, 1.0, 0.0)
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Tint 1'), green_color_value)
        general.idle_wait_frames(1)
        tint_color_1 = bloom_component.get_component_property_value(
            AtomComponentProperties.bloom('Tint 1'))
        Report.result(Tests.edit_tint_1, tint_color_1.IsClose(green_color_value))

        # 64. Edit the Tint 2 parameter.
        blue_color_value = math.Vector3(0.0, 0.0, 1.0)
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Tint 2'), blue_color_value)
        general.idle_wait_frames(1)
        tint_color_2 = bloom_component.get_component_property_value(
            AtomComponentProperties.bloom('Tint 2'))
        Report.result(Tests.edit_tint_2, tint_color_2.IsClose(blue_color_value))

        # 65. Edit the Tint 3 parameter.
        yellow_color_value = math.Vector3(1.0, 1.0, 0.0)
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Tint 3'), yellow_color_value)
        general.idle_wait_frames(1)
        tint_color_3 = bloom_component.get_component_property_value(
            AtomComponentProperties.bloom('Tint 3'))
        Report.result(Tests.edit_tint_3, tint_color_3.IsClose(yellow_color_value))

        # 66. Edit the Tint 4 parameter.
        violet_color_value = math.Vector3(0.498, 0.0, 1.0)
        bloom_component.set_component_property_value(AtomComponentProperties.bloom(
            'Tint 4'), violet_color_value)
        general.idle_wait_frames(1)
        tint_color_4 = bloom_component.get_component_property_value(
            AtomComponentProperties.bloom('Tint 4'))
        Report.result(Tests.edit_tint_4, tint_color_4.IsClose(violet_color_value))

        # 67. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 68. Test IsHidden.
        bloom_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, bloom_entity.is_hidden() is True)

        # 69. Test IsVisible.
        bloom_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, bloom_entity.is_visible() is True)

        # 70. Delete Bloom entity.
        bloom_entity.delete()
        Report.result(Tests.entity_deleted, not bloom_entity.exists())

        # 71. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, bloom_entity.exists())

        # 72. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not bloom_entity.exists())

        # 73. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Bloom_AddedToEntity)
