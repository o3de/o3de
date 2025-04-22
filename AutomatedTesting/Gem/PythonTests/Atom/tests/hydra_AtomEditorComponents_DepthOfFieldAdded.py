"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    camera_creation = (
        "P0: Camera Entity successfully created",
        "P0: Camera Entity failed to be created")
    camera_component_added = (
        "P0: Camera component was added to Camera entity",
        "P0: Camera component failed to be added to entity")
    camera_component_check = (
        "P0: Entity has a Camera component",
        "P0: Entity failed to find Camera component")
    camera_property_set = (
        "P0: DepthOfField Entity set Camera Entity",
        "P0: DepthOfField Entity could not set Camera Entity")
    creation_undo = (
        "P0: UNDO Entity creation success",
        "P0: UNDO Entity creation failed")
    creation_redo = (
        "P0: REDO Entity creation success",
        "P0: REDO Entity creation failed")
    depth_of_field_creation = (
        "P0: DepthOfField Entity successfully created",
        "P0: DepthOfField Entity failed to be created")
    depth_of_field_component = (
        "P0: Entity has a DepthOfField component",
        "P0: Entity failed to find DepthOfField component")
    depth_of_field_component_disabled = (
        "P0: DepthOfField component disabled",
        "P0: DepthOfField component was not disabled")
    depth_of_field_component_enabled = (
        "P0: DepthOfField component enabled",
        "P0: DepthOfField component was not enabled")
    post_fx_component = (
        "P0: Entity has a Post FX Layer component",
        "P0: Entity did not have a Post FX Layer component")
    cameraentityid_override_on = (
        "P1: CameraEntityId Override turned on",
        "P1: CameraEntityId Override could not be turned on")
    cameraentityid_override_off = (
        "P1: CameraEntityId Override turned off",
        "P1: CameraEntityId Override could not be turned off")
    enabled_override_on = (
        "P1: Enabled Override turned on",
        "P1: Enabled Override could not be turned on")
    enabled_override_off = (
        "P1: Enabled Override turned off",
        "P1: Enabled Override could not be turned off")
    qualitylevel_override_on = (
        "P1: QualityLevel Override turned on",
        "P1: QualityLevel Override could not be turned on")
    qualitylevel_override_off = (
        "P1: QualityLevel Override turned off",
        "P1: QualityLevel Override could not be turned off")
    aperturef_override_set_to_1 = (
        "P1: ApertureF Override set to 1",
        "P1: ApertureF Override could not be set to 1")
    aperturef_override_set_to_0 = (
        "P1: ApertureF Override set to 0",
        "P1: ApertureF Override could not be set to 0")
    focusdistance_override_set_to_0 = (
        "P1: FocusDistance Override set to 1",
        "P1: FocusDistance Override could not be set to 1")
    focusdistance_override_set_to_1 = (
        "P1: FocusDistance Override set to 0",
        "P1: FocusDistance Override could not be set to 0")
    enableautofocus_override_on = (
        "P1: EnableAutoFocus Override turned on",
        "P1: EnableAutoFocus Override could not be turned on")
    enableautofocus_override_off = (
        "P1: EnableAutoFocus Override turned off",
        "P1: EnableAutoFocus Override could not be turned off")
    autofocusscreeposition_override_set_to_1 = (
        "P1: AutoFocusScreePosition Override set to 1",
        "P1: AutoFocusScreePosition Override could not be set to 1")
    autofocusscreeposition_override_set_to_0 = (
        "P1: AutoFocusScreePosition Override set to 0",
        "P1: AutoFocusScreePosition Override could not be set to 0")
    autofocussensitivity_override_set_to_1 = (
        "P1: AutoFocusSensitivity Override set to 1",
        "P1: AutoFocusSensitivity Override could not be set to 1")
    autofocussensitivity_override_set_to_0 = (
        "P1: AutoFocusSensitivity Override set to 0",
        "P1: AutoFocusSensitivity Override could not be set to 0")
    autofocusspeed_override_set_to_1 = (
        "P1: AutoFocusSpeed Override set to 1",
        "P1: AutoFocusSpeed Override could not be set to 1")
    autofocusspeed_override_set_to_0 = (
        "P1: AutoFocusSpeed Override set to 0",
        "P1: AutoFocusSpeed Override could not be set to 0")
    autofocusdelay_override_set_to_1 = (
        "P1: AutoFocusDelay Override set to 1",
        "P1: AutoFocusDelay Override could not be set to 1")
    autofocusdelay_override_set_to_0 = (
        "P1: AutoFocusDelay Override set to 0",
        "P1: AutoFocusDelay Override could not be set to 0")
    enabledebugcoloring_override_on = (
        "P1: EnableDebugColoring Override turned on",
        "P1: EnableDebugColoring Override could not be turned on")
    enabledebugcoloring_override_off = (
        "P1: EnableDebugColoring Override turned off",
        "P1: EnableDebugColoring Override could not be turned off")
    enable_depth_of_field_on = (
        "P0: Enable Depth of Field turned on",
        "P0: Enable Depth of Field could not be turned on")
    enable_depth_of_field_off = (
        "P0: Enable Depth of Field turned off",
        "P0: Enable Depth of Field could not be turned off")
    quality_level_set_to_0 = (
        "P1: Quality Level set to 0",
        "P1: Quality Level could not be set to 0")
    quality_level_set_to_1 = (
        "P1: Quality Level set to 1",
        "P1: Quality Level could not be set to 1")
    aperture_f_set_to_0 = (
        "P1: Aperture F set to 0",
        "P1: Aperture F could not be set to 0")
    aperture_f_set_to_five_tenths = (
        "P1: Aperture F set to five tenths",
        "P1: Aperture F could not be set to five tenths")
    f_number = (
        "P1: F number is the expected value",
        "P1: F number is not the expected value")
    focus_distance_set_to_0 = (
        "P1: Focus Distance set to 0",
        "P1: Focus Distance could not be set to 0")
    focus_distance_set_to_100 = (
        "P1: Enable Auto Focus set to 100",
        "P1: Enable Auto Focus could not be set to 100")
    enable_auto_focus_off = (
        "P1: Enable Auto Focus turned off",
        "P1: Enable Auto Focus could not be turned off")
    enable_auto_focus_on = (
        "P1: Enable Auto Focus turned on",
        "P1: Enable Auto Focus could not be turned on")
    focus_screen_position_set_0 = (
        "P1: Focus Screen Position set to 0,0",
        "P1: Focus Screen Position could not be set to 0,0")
    focus_screen_position_set_to_default = (
        "P1: Focus Screen Position set to 0.5,0.5",
        "P1: Focus Screen Position could not be set 0.5,0.5")
    auto_focus_sensitivity_set_to_0 = (
        "P1: Auto Focus Sensitivity set to 0",
        "P1: Auto Focus Sensitivity could not be turned on")
    auto_focus_sensitivity_set_to_1 = (
        "P1: Auto Focus Sensitivity set to 1",
        "P1: Auto Focus Sensitivity could not be set to 1")
    auto_focus_speed_set_to_0 = (
        "P1: Auto Focus Speed set to 0",
        "P1: Auto Focus Speed could not be set to 0")
    auto_focus_speed_set_to_2 = (
        "P1: Auto Focus Speed set to 2",
        "P1: Auto Focus Speed could not be set to 2")
    auto_focus_delay_set_to_1 = (
        "P1: Auto Focus Delay set to 1",
        "P1: Auto Focus Delay could not be set to 1")
    auto_focus_delay_set_to_0 = (
        "P1: Auto Focus Delay set to 0",
        "P1: Auto Focus Delay could not be set to 0")
    enable_debug_color_on = (
        "P1: Enable Debug Color turned on",
        "P1: Enable Debug Color could not be turned on")
    enable_debug_color_off = (
        "P1: Enable Debug Color turned off",
        "P1: Enable Debug Color could not be turned off")
    enter_game_mode = (
        "P0: Entered game mode",
        "P0: Failed to enter game mode")
    exit_game_mode = (
        "P0: Exited game mode",
        "P0: Couldn't exit game mode")
    is_visible = (
        "P0: Entity is visible",
        "P0: Entity was not visible")
    is_hidden = (
        "P0: Entity is hidden",
        "P0: Entity was not hidden")
    entity_deleted = (
        "P0: Entity deleted",
        "P0: Entity was not deleted")
    deletion_undo = (
        "P0: UNDO deletion success",
        "P0: UNDO deletion failed")
    deletion_redo = (
        "P0: REDO deletion success",
        "P0: REDO deletion failed")


def AtomEditorComponents_DepthOfField_AddedToEntity():
    """
    Summary:
    Tests the DepthOfField component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a DepthOfField entity with no components.
    2) Add a DepthOfField component to DepthOfField entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Verify DepthOfField component not enabled.
    6) Add Post FX Layer component since it is required by the DepthOfField component.
    7) Verify DepthOfField component is enabled.
    8) Enter/Exit game mode.
    9) Test IsHidden.
    10) Test IsVisible.
    11) Add Camera entity.
    12) Add Camera component to Camera entity.
    13) Set the DepthOfField components's Camera Entity to the newly created Camera entity.
    14) CameraEntityId Override can be turned on and off.
    15) Enabled Override can be turned on and off.
    16) QualityLevel Override can be turned on and off.
    17) ApertureF Override can be set to 0 and back to default value of 1.
    18) FocusDistance Override can be set to 0 and back to 1.
    19) EnableAutoFocus Override can be turned on and off.
    20) AutoFocusScreenPosition Override can be set to 0 and back to 1.
    21) AutoFocusSensitivity Override can be set to 0 and back to 1.
    22) AutoFocusSpeed Override can be set to 0 and back to 1.
    23) AutoFocusDelay Override can be set to 0 and back to 1.
    24) EnabledDebugColoring Override can be turned on and off.
    25) Enable Depth of Field can be turned off and back on again.
    26) Quality Level can be set to 0 and back to 1.
    27) Aperture F can be set to 0 and back to 0.5.
    28a) Enable Auto Focus can be turned off.
    28b) Enable Auto Focus can be turned back on.
    29) Focus Distance can be set to 0 then back to 100.
    30) Focus Screen Position.
    31) Auto Focus Sensitivity.
    32) Auto Focus Speed.
    33) Auto Focus Delay.
    34) Enable Debug Color.
    35) Delete DepthOfField entity.
    36) UNDO deletion.
    37) REDO deletion.
    38) Look for errors and asserts.

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
        # 1. Create a DepthOfField entity with no components.
        depth_of_field_entity = EditorEntity.create_editor_entity(AtomComponentProperties.depth_of_field())
        Report.critical_result(Tests.depth_of_field_creation, depth_of_field_entity.exists())

        # 2. Add a DepthOfField component to DepthOfField entity.
        depth_of_field_component = depth_of_field_entity.add_component(AtomComponentProperties.depth_of_field())
        Report.critical_result(Tests.depth_of_field_component,
                               depth_of_field_entity.has_component(AtomComponentProperties.depth_of_field()))

        # 3. UNDO the entity creation and component addition.
        # -> UNDO component addition.
        general.undo()
        # -> UNDO naming entity.
        general.undo()
        # -> UNDO selecting entity.
        general.undo()
        # -> UNDO entity creation.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_undo, not depth_of_field_entity.exists())

        # 4. REDO the entity creation and component addition.
        # -> REDO entity creation.
        general.redo()
        # -> REDO selecting entity.
        general.redo()
        # -> REDO naming entity.
        general.redo()
        # -> REDO component addition.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_redo, depth_of_field_entity.exists())

        # 5. Verify DepthOfField component not enabled.
        Report.result(Tests.depth_of_field_component_disabled, not depth_of_field_component.is_enabled())

        # 6. Add Post FX Layer component since it is required by the DepthOfField component.
        depth_of_field_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.result(Tests.post_fx_component, depth_of_field_entity.has_component(
            AtomComponentProperties.postfx_layer()))

        # 7. Verify DepthOfField component is enabled.
        Report.result(Tests.depth_of_field_component_enabled, depth_of_field_component.is_enabled())

        # 8. Test IsHidden.
        depth_of_field_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, depth_of_field_entity.is_hidden() is True)

        # 9. Test IsVisible.
        depth_of_field_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, depth_of_field_entity.is_visible() is True)

        # 10. Add Camera entity.
        camera_entity = EditorEntity.create_editor_entity(AtomComponentProperties.camera())
        Report.result(Tests.camera_creation, camera_entity.exists())

        # 11. Add Camera component to Camera entity.
        camera_entity.add_component(AtomComponentProperties.camera())
        Report.result(Tests.camera_component_added, camera_entity.has_component(AtomComponentProperties.camera()))

        # 12. Set the DepthOfField component's Camera Entity to the newly created Camera entity.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Camera Entity'), camera_entity.id)
        Report.result(
            Tests.camera_property_set,
            camera_entity.id == depth_of_field_component.get_component_property_value(
                                    AtomComponentProperties.depth_of_field('Camera Entity')))

        # Enable Depth of Field for subsequent tests.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Enable Depth of Field'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enable_depth_of_field_on,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Enable Depth of Field')) is True)

        # 13. CameraEntityId Override can be turned on and off.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('CameraEntityId Override'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.cameraentityid_override_on,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('CameraEntityId Override')) is True)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('CameraEntityId Override'), False)
        Report.result(
            Tests.cameraentityid_override_off,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('CameraEntityId Override')) is False)

        # 14. Enabled Override can be turned on and off.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Enabled Override'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enabled_override_on,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Enabled Override')) is True)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Enabled Override'), False)
        Report.result(
            Tests.enabled_override_off,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Enabled Override')) is False)

        # 15. QualityLevel Override can be turned on and off.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('QualityLevel Override'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.qualitylevel_override_on,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('QualityLevel Override')) is True)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('QualityLevel Override'), False)
        Report.result(
            Tests.qualitylevel_override_off,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('QualityLevel Override')) is False)

        # 16. ApertureF Override can be set to 0 and back to default value of 1.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('ApertureF Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.aperturef_override_set_to_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('ApertureF Override')) == 0.0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('ApertureF Override'), 1.0)
        Report.result(
            Tests.aperturef_override_set_to_1,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('ApertureF Override')) == 1.0)

        # 17. FocusDistance Override can be set to 0 and back to 1.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('FocusDistance Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.focusdistance_override_set_to_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('FocusDistance Override')) == 0.0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('FocusDistance Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.focusdistance_override_set_to_1,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('FocusDistance Override')) == 1.0)

        # 18. EnableAutoFocus Override can be turned on and off.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('EnableAutoFocus Override'), True)
        Report.result(
            Tests.enable_auto_focus_on,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('EnableAutoFocus Override')) is True)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('EnableAutoFocus Override'), False)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enable_auto_focus_off,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('EnableAutoFocus Override')) is False)

        # 19. AutoFocusScreenPosition Override can be set to 0 and back to 1.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('AutoFocusScreenPosition Override'), 0.0)
        Report.result(
            Tests.autofocusscreeposition_override_set_to_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('AutoFocusScreenPosition Override')) == 0.0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('AutoFocusScreenPosition Override'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.autofocusscreeposition_override_set_to_1,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('AutoFocusScreenPosition Override')) == 1.0)

        # 20. AutoFocusSensitivity Override can be set to 0 and back to 1.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('AutoFocusSensitivity Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.autofocussensitivity_override_set_to_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('AutoFocusSensitivity Override')) == 0.0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('AutoFocusSensitivity Override'), 1.0)
        Report.result(
            Tests.autofocussensitivity_override_set_to_1,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('AutoFocusSensitivity Override')) == 1.0)

        # 21. AutoFocusSpeed Override can be set to 0 and back to 1.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('AutoFocusSpeed Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.autofocusspeed_override_set_to_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('AutoFocusSpeed Override')) == 0.0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('AutoFocusSpeed Override'), 1.0)
        Report.result(
            Tests.autofocusspeed_override_set_to_1,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('AutoFocusSpeed Override')) == 1.0)

        # 22. AutoFocusDelay Override can be set to 0 and back to 1.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('AutoFocusDelay Override'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.auto_focus_delay_set_to_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('AutoFocusDelay Override')) == 0.0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('AutoFocusDelay Override'), 1.0)
        Report.result(
            Tests.auto_focus_delay_set_to_1,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('AutoFocusDelay Override')) == 1.0)

        # 23. EnabledDebugColoring Override can be turned on and off.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('EnableDebugColoring Override'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enabledebugcoloring_override_on,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('EnableDebugColoring Override')) is True)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('EnableDebugColoring Override'), False)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enabledebugcoloring_override_off,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('EnableDebugColoring Override')) is False)

        # 24. Enable Depth of Field can be turned off and back on again.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Enable Depth of Field'), False)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enable_depth_of_field_on,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Enable Depth of Field')) is False)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Enable Depth of Field'), True)
        Report.result(
            Tests.enable_depth_of_field_off,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Enable Depth of Field')) is True)

        # 25. Quality Level can be set to 0 and back to 1.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Quality Level'), 0)
        Report.result(
            Tests.quality_level_set_to_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Quality Level')) == 0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Quality Level'), 1)
        general.idle_wait_frames(1)
        Report.result(
            Tests.quality_level_set_to_1,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Quality Level')) == 1)

        # 26. Aperture F can be set to 0 and back to 0.5.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Aperture F'), 0.0)
        Report.result(
            Tests.aperture_f_set_to_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Aperture F')) == 0.0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Aperture F'), 0.5)
        general.idle_wait_frames(1)
        Report.result(
            Tests.aperture_f_set_to_five_tenths,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Aperture F')) == 0.5)

        # 27. F Number Note - Make sure F number is accurate
        Report.result(
            Tests.f_number,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('F Number')) == 0.239887535572052)

        # 28a. Enable Auto Focus can be turned off.
        # Turns off Auto Focus to unlock Focus distance setting for testing
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Enable Auto Focus'), False)
        Report.result(
            Tests.enable_auto_focus_off,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Enable Auto Focus')) is False)

        # 29. Focus Distance can be set to 0 then back to 100.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Focus Distance'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.focus_distance_set_to_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Focus Distance')) == 0.0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Focus Distance'), 100.0)
        Report.result(
            Tests.focus_distance_set_to_100,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Focus Distance')) == 100.0)

        # 28b. Enable Auto Focus can be turned back on.
        # Turns on auto focus, which needs to be enabled for remaining tests
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Enable Auto Focus'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enable_auto_focus_on,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Enable Auto Focus')) is True)

        # 30. Focus Screen Position.
        position_vector_0 = math.Vector2(0.0, 0.0)
        position_vector_default = math.Vector2(0.5, 0.5)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Focus Screen Position'), position_vector_0)
        Report.result(
            Tests.focus_screen_position_set_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Focus Screen Position')) == position_vector_0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Focus Screen Position'),
            position_vector_default)
        general.idle_wait_frames(1)
        Report.result(
            Tests.focus_screen_position_set_to_default,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Focus Screen Position'))
            == position_vector_default)

        # 31. Auto Focus Sensitivity.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Auto Focus Sensitivity'), 0.0)
        Report.result(
            Tests.auto_focus_sensitivity_set_to_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Auto Focus Sensitivity')) == 0.0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Auto Focus Sensitivity'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.auto_focus_sensitivity_set_to_1,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Auto Focus Sensitivity')) == 1.0)

        # 32. Auto Focus Speed.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Auto Focus Speed'), 0.0)
        Report.result(
            Tests.auto_focus_speed_set_to_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Auto Focus Speed')) == 0.0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Auto Focus Speed'), 2.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.auto_focus_speed_set_to_2,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Auto Focus Speed')) == 2.0)

        # 33. Auto Focus Delay.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Auto Focus Delay'), 1.0)
        Report.result(
            Tests.auto_focus_delay_set_to_1,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Auto Focus Delay')) == 1.0)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Auto Focus Delay'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.auto_focus_delay_set_to_0,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Auto Focus Delay')) == 0.0)

        # 34. Enable Debug Color.
        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Enable Debug Color'), True)
        Report.result(
            Tests.enable_debug_color_on,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Enable Debug Color')) is True)

        depth_of_field_component.set_component_property_value(
            AtomComponentProperties.depth_of_field('Enable Debug Color'), False)
        general.idle_wait_frames(1)
        Report.result(
            Tests.enable_debug_color_off,
            depth_of_field_component.get_component_property_value(
                AtomComponentProperties.depth_of_field('Enable Debug Color')) is False)

        # 35. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 36. Delete DepthOfField entity.
        depth_of_field_entity.delete()
        Report.result(Tests.entity_deleted, not depth_of_field_entity.exists())

        # 37. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, depth_of_field_entity.exists())

        # 38. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not depth_of_field_entity.exists())

        # 39. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_DepthOfField_AddedToEntity)
