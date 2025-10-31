"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    exposure_control_creation = (
        "Exposure Control Entity successfully created",
        "P0: Exposure Control Entity failed to be created")
    exposure_control_component = (
        "Entity has a Exposure Control component",
        "P0: Entity failed to find Exposure Control component")
    exposure_control_component_removal = (
        "Exposure Control component successfully removed",
        "P0: Exposure Control component failed to be removed")
    removal_undo = (
        "UNDO Exposure Control component removal success",
        "P0: UNDO Exposure Control component removal failed")
    exposure_control_disabled = (
        "Exposure Control component disabled",
        "P0: Exposure Control component was not disabled.")
    post_fx_component = (
        "Entity has a Post FX Layer component",
        "P0: Entity did not have a Post FX Layer component")
    exposure_control_enabled = (
        "Exposure Control component enabled",
        "P0: Exposure Control component was not enabled.")
    toggle_enable_parameter_on = (
        "Enable parameter enabled.",
        "Enable parameter failed to be enabled.")
    toggle_enable_parameter_off = (
        "Enable parameter disabled.",
        "Enable parameter failed to be disabled.")
    toggle_enabled_override_on = (
        "Enabled Override parameter enabled",
        "P1: Enabled Override parameter was not enabled")
    toggle_enabled_override_off = (
        "Enabled Override parameter disabled",
        "P1: Enabled Override parameter was not disabled")
    toggle_control_type_override_on = (
        "ExposureControlType Override enabled",
        "P1: ExposureControlType Override was not enabled")
    toggle_control_type_override_off = (
        "ExposureControlType Override disabled",
        "P1: ExposureControlType Override was not disabled")
    manual_compensation_override_min_value = (
        "ManualCompensation Override set to minimum value",
        "P1: ManualCompensation Override failed to be set to minimum value")
    manual_compensation_override_max_value = (
        "ManualCompensation Override set to maximum value",
        "P1: ManualCompensation Override failed to be set to maximum value")
    minimum_exposure_override_min_value = (
        "EyeAdaptationExposureMin Override set to minimum value",
        "P1: EyeAdaptationExposureMin Override failed to be set to minimum value")
    minimum_exposure_override_max_value = (
        "EyeAdaptationExposureMin Override set to maximum value",
        "P1: EyeAdaptationExposureMin Override failed to be set to maximum value")
    maximum_exposure_override_min_value = (
        "EyeAdaptationExposureMax Override set to minimum value",
        "P1: EyeAdaptationExposureMax Override failed to be set to minimum value")
    maximum_exposure_override_max_value = (
        "EyeAdaptationExposureMax Override set to maximum value",
        "P1: EyeAdaptationExposureMax Override failed to be set to maximum value")
    speed_up_override_min_value = (
        "EyeAdaptationSpeedUp Override set to minimum value",
        "P1: EyeAdaptationSpeedUp Override failed to be set to minimum value")
    speed_up_override_max_value = (
        "EyeAdaptationSpeedUp Override set to maximum value",
        "P1: EyeAdaptationSpeedUp Override failed to be set to maximum value")
    speed_down_override_min_value = (
        "EyeAdaptationSpeedDown Override set to minimum value",
        "P1: EyeAdaptationSpeedDown Override failed to be set to minimum value")
    speed_down_override_max_value = (
        "EyeAdaptationSpeedDown Override set to maximum value",
        "P1: EyeAdaptationSpeedDown Override failed to be set to maximum value")
    toggle_heatmap_override_on = (
        "HeatmapEnabled Override enabled",
        "HeatmapEnabled Override failed to be enabled")
    toggle_heatmap_override_off = (
        "HeatmapEnabled Override disabled",
        "HeatmapEnabled Override failed to be disabled")
    set_control_type = (
        "Control Type set to Eye Adaptation",
        "Control Type failed to be set to Eye Adaptation")
    manual_compensation_min_value = (
        "Manual Compensation set to minimum value",
        "Manual Compensation failed to be set to minimum value")
    manual_compensation_max_value = (
        "Manual Compensation set to maximum value",
        "Manual Compensation failed to be set to maximum value")
    minimum_exposure_min_value = (
        "Minimum Exposure set to minimum value",
        "Minimum Exposure failed to be set to minimum value")
    minimum_exposure_max_value = (
        "Minimum Exposure set to maximum value",
        "Minimum Exposure failed to be set to maximum value")
    maximum_exposure_min_value = (
        "Maximum Exposure set to minimum value",
        "Maximum Exposure failed to be set to minimum value")
    maximum_exposure_max_value = (
        "Maximum Exposure set to maximum value",
        "Maximum Exposure failed to be set to maximum value")
    speed_up_min_value = (
        "Speed Up set to minimum value",
        "Speed Up failed to be set to minimum value")
    speed_up_max_value = (
        "Speed Up set to maximum value",
        "Speed Up failed to be set to maximum value")
    speed_down_min_value = (
        "Speed Down set to minimum value",
        "Speed Down failed to be set to minimum value")
    speed_down_max_value = (
        "Speed Down set to maximum value",
        "Speed Down failed to be set to maximum value")
    toggle_enable_heatmap_on = (
        "Heatmap enabled",
        "Heatmap failed to be enabled")
    toggle_enable_heatmap_off = (
        "Heatmap disabled",
        "Heatmap failed to be disabled")
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
        "P0:Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "P0: Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "P0: UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "P0: REDO deletion failed")


def AtomEditorComponents_ExposureControl_AddedToEntity():
    """
    Summary:
    Tests the Exposure Control component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an Exposure Control entity with no components.
    2) Add Exposure Control component to Exposure Control entity.
    3) Remove the Exposure Control component.
    4) Undo Exposure Control component removal.
    5) Verify Exposure Control component not enabled.
    6) Add Post FX Layer component since it is required by the Exposure Control component.
    7) Verify Exposure Control component is enabled.
    8) Toggle the Enable parameter.
    9) Toggle the Enable Override parameter.
    10) Toggle the ExposureControlType Override parameter.
    11) Set ManualCompensation Override to min/max values.
    12) Set EyeAdaptationExposureMin Override to min/max values.
    13) Set EyeAdaptationExposureMax Override to min/max values.
    14) Set EyeAdaptationSpeedUp Override to min/max values.
    15) Set EyeAdaptationSpeedDown Override to min/max values.
    16) Toggle HeatmapEnabled Override.
    17) Set Control Type to Eye Adaptation.
    18) Set Manual Compensation to min/max values.
    19) Set Minimum Exposure to min/max values.
    20) Set Maximum Exposure to min/max values.
    21) Set Speed Up parameter to min/max values.
    22) Set Speed Down parameter to min/max values.
    23) Toggle Enable Heatmap.
    24) Enter/Exit game mode.
    25) Test IsHidden.
    26) Test IsVisible.
    27) Delete Exposure Control entity.
    28) UNDO deletion.
    29) REDO deletion.
    30) Look for errors and asserts.

    :return: None
    """

    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties, EXPOSURE_CONTROL_TYPE
    from azlmbr.math import Math_IsClose

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Creation of Exposure Control entity with no components.
        exposure_control_entity = EditorEntity.create_editor_entity(AtomComponentProperties.exposure_control())
        Report.critical_result(Tests.exposure_control_creation, exposure_control_entity.exists())

        # 2. Add Exposure Control component to Exposure Control entity.
        exposure_control_component = exposure_control_entity.add_component(AtomComponentProperties.exposure_control())
        Report.critical_result(
            Tests.exposure_control_component,
            exposure_control_entity.has_component(AtomComponentProperties.exposure_control()))

        # 3. Remove the Exposure Control component.
        exposure_control_component.remove()
        general.idle_wait_frames(1)
        Report.result(Tests.exposure_control_component_removal,
                               not exposure_control_entity.has_component(AtomComponentProperties.exposure_control()))

        # 4. Undo Exposure Control component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.removal_undo,
                      exposure_control_entity.has_component(AtomComponentProperties.exposure_control()))

        # 5. Verify Exposure Control component not enabled.
        Report.result(Tests.exposure_control_disabled, not exposure_control_component.is_enabled())

        # 6. Add Post FX Layer component since it is required by the Exposure Control component.
        exposure_control_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(Tests.post_fx_component,
                      exposure_control_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 7. Verify Exposure Control component is enabled.
        Report.result(Tests.exposure_control_enabled, exposure_control_component.is_enabled())

        # 8. Toggle the Enable parameter.
        # Set the Enable parameter to False.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Enable'), value=False)
        Report.result(
            Tests.toggle_enable_parameter_off,
            exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Enable')) is False)

        # Set the Enable parameter to True.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Enable'), value=True)
        Report.result(
            Tests.toggle_enable_parameter_on,
            exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Enable')) is True)

        # 9. Toggle the Enable Override parameter.
        # Set the Enable Override parameter to False.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Enabled Override'), value=False)
        Report.result(
            Tests.toggle_enabled_override_off,
            exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Enabled Override')) is False)

        # Set the Enable Override parameter to True.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Enabled Override'), value=True)
        Report.result(
            Tests.toggle_enabled_override_on,
            exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Enabled Override')) is True)

        # 10. Toggle the ExposureControlType Override parameter.
        # Set the ExposureControlType Override parameter to False.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('ExposureControlType Override'), value=False)
        Report.result(
            Tests.toggle_control_type_override_off,
            exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('ExposureControlType Override')) is False)

        # Set the ExposureControlType Override parameter to True.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('ExposureControlType Override'), value=True)
        Report.result(
            Tests.toggle_control_type_override_on,
            exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('ExposureControlType Override')) is True)

        # 11. Set ManualCompensation Override to min/max values.
        # Set ManualCompensationOverride to minimum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('ManualCompensation Override'), value=0.0)
        Report.result(
            Tests.manual_compensation_override_min_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('ManualCompensation Override')), 0.0))

        # Set ManualCompensationOverride to maximum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('ManualCompensation Override'), value=1.0)
        Report.result(
            Tests.manual_compensation_override_max_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('ManualCompensation Override')), 1.0))

        # 12. Set EyeAdaptationExposureMin Override to min/max values.
        # Set EyeAdaptationExposureMin to minimum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('EyeAdaptationExposureMin Override'), value=0.0)
        Report.result(
            Tests.minimum_exposure_override_min_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('EyeAdaptationExposureMin Override')), 0.0))

        # Set EyeAdaptationExposureMin to maximum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('EyeAdaptationExposureMin Override'), value=1.0)
        Report.result(
            Tests.minimum_exposure_override_max_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('EyeAdaptationExposureMin Override')), 1.0))

        # 13. Set EyeAdaptationExposureMax Override to min/max values.
        # Set EyeAdaptationExposureMax Override to minimum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('EyeAdaptationExposureMax Override'), value=0.0)
        Report.result(
            Tests.maximum_exposure_override_min_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('EyeAdaptationExposureMax Override')), 0.0))

        # Set EyeAdaptationExposureMax Override to maximum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('EyeAdaptationExposureMax Override'), value=1.0)
        Report.result(
            Tests.maximum_exposure_override_max_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('EyeAdaptationExposureMax Override')), 1.0))

        # 14. Set EyeAdaptationSpeedUp Override to min/max values.
        # Set EyeAdaptationSpeedUp Override to minimum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('EyeAdaptationSpeedUp Override'), value=0.0)
        Report.result(
            Tests.speed_up_override_min_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('EyeAdaptationSpeedUp Override')), 0.0))

        # Set EyeAdaptationSpeedUp Override to maximum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('EyeAdaptationSpeedUp Override'), value=1.0)
        Report.result(
            Tests.speed_up_override_max_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('EyeAdaptationSpeedUp Override')), 1.0))

        # 15. Set EyeAdaptationSpeedDown Override to min/max values.
        # Set EyeAdaptationSpeedDown Override to minimum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('EyeAdaptationSpeedDown Override'), value=0.0)
        Report.result(
            Tests.speed_down_override_min_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('EyeAdaptationSpeedDown Override')), 0.0))

        # Set EyeAdaptationSpeedDown Override to maximum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('EyeAdaptationSpeedDown Override'), value=1.0)
        Report.result(
            Tests.speed_down_override_max_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('EyeAdaptationSpeedDown Override')), 1.0))

        # 16. Toggle HeatmapEnabled Override.
        # Set the HeatmapEnabled Override parameter to False.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('HeatmapEnabled Override'), value=False)
        Report.result(
            Tests.toggle_heatmap_override_off,
            exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('HeatmapEnabled Override')) is False)

        # Set the HeatmapEnabled Override parameter to True.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('HeatmapEnabled Override'), value=True)
        Report.result(
            Tests.toggle_heatmap_override_on,
            exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('HeatmapEnabled Override')) is True)

        general.idle_wait_frames(1)

        # 17. Set Control Type to Eye Adaptation.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Control Type'), EXPOSURE_CONTROL_TYPE['eye_adaptation'])
        Report.result(
            Tests.set_control_type,
            exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Control Type')) == EXPOSURE_CONTROL_TYPE['eye_adaptation'])

        # 18. Set Manual Compensation to min/max values.
        # Set Manual Compensation to minimum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Manual Compensation'), value=-16.0)
        Report.result(
            Tests.manual_compensation_min_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Manual Compensation')), -16.0))

        # Set Manual Compensation to maximum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Manual Compensation'), value=16.0)
        Report.result(
            Tests.manual_compensation_max_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Manual Compensation')), 16.0))

        # 19. Set Minimum Exposure to min/max values.
        # Set Minimum Exposure to minimum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Minimum Exposure'), value=-16.0)
        Report.result(
            Tests.minimum_exposure_min_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Minimum Exposure')), -16.0))

        # Set Minimum Exposure to maximum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Minimum Exposure'), value=16.0)
        Report.result(
            Tests.minimum_exposure_max_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Minimum Exposure')), 16.0))

        # 20. Set Maximum Exposure to min/max values.
        # Set Maximum Exposure to minimum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Maximum Exposure'), value=-16.0)
        Report.result(
            Tests.maximum_exposure_min_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Maximum Exposure')), -16.0))

        # Set Maximum Exposure to maximum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Maximum Exposure'), value=16.0)
        Report.result(
            Tests.maximum_exposure_max_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Maximum Exposure')), 16.0))

        # 21. Set Speed Up parameter to min/max values.
        # Set Speed Up to minimum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Speed Up'), value=0.01)
        Report.result(
            Tests.speed_up_min_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Speed Up')), 0.01))

        # Set Speed Up to maximum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Speed Up'), value=10.0)
        Report.result(
            Tests.speed_up_max_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Speed Up')), 10.0))

        # 22. Set Speed Down parameter to min/max values.
        # Set Speed Down to minimum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Speed Down'), value=0.01)
        Report.result(
            Tests.speed_down_min_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Speed Down')), 0.01))

        # Set Speed Down to maximum value.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Speed Down'), value=10.0)
        Report.result(
            Tests.speed_down_max_value,
            Math_IsClose(exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Speed Down')), 10.0))

        # 23. Toggle Enable Heatmap.
        # Set the Enable Heatmap parameter to True.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Enable Heatmap'), value=True)
        Report.result(
            Tests.toggle_enable_heatmap_on,
            exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Enable Heatmap')) is True)

        # Set the Enable Heatmap parameter to False.
        exposure_control_component.set_component_property_value(
            AtomComponentProperties.exposure_control('Enable Heatmap'), value=False)
        Report.result(
            Tests.toggle_enable_heatmap_off,
            exposure_control_component.get_component_property_value(
                AtomComponentProperties.exposure_control('Enable Heatmap')) is False)

        # 24. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 25. Test IsHidden.
        exposure_control_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, exposure_control_entity.is_hidden() is True)

        # 26. Test IsVisible.
        exposure_control_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, exposure_control_entity.is_visible() is True)

        # 27. Delete ExposureControl entity.
        exposure_control_entity.delete()
        Report.result(Tests.entity_deleted, not exposure_control_entity.exists())

        # 28. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, exposure_control_entity.exists())

        # 29. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not exposure_control_entity.exists())

        # 30. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_ExposureControl_AddedToEntity)
