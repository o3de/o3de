"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    camera_creation = (
        "Camera Entity successfully created",
        "Camera Entity failed to be created")
    camera_component_added = (
        "Camera component was added to entity",
        "Camera component failed to be added to entity")
    camera_component_check = (
        "Entity has a Camera component",
        "Entity failed to find Camera component")
    creation_undo = (
        "UNDO Entity creation success",
        "UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "REDO Entity creation failed")
    directional_light_creation = (
        "Directional Light Entity successfully created",
        "Directional Light Entity failed to be created")
    directional_light_component = (
        "Entity has a Directional Light component",
        "Entity failed to find Directional Light component")
    color_set_to_purple = (
        "Color has been set to 213,0,255,255",
        "Color could not be set 213,0,255,255")
    color_set_back_to_default = (
        "Color has been set to 255,255,255,255",
        "Color could not be set 255,255,255,255")
    intensity_mode_set_to_lux = (
        "Intensity mode has been set to Lux",
        "Intensity could not be set to Lux")
    intensity_mode_set_to_ev100 = (
        "Intensity mode has been set to Ev100",
        "Intensity could not be set to Ev100")
    intensity_set_to_10 = (
        "Intensity value has been set to 10.0",
        "Intensity value could not be set to 10.0")
    intensity_set_to_default = (
        "Intensity value has been set to 4.0",
        "Intensity value could not be set to 4.0")
    angular_diameter_set_to_1 = (
        "Intensity value has been set to 1.0",
        "Intensity value could not be set to 1.0")
    angular_diameter_set_to_default = (
        "Intensity value has been set to 0.5",
        "Intensity value could not be set to 0.5")
    shadow_camera_check = (
        "Directional Light component Shadow camera set",
        "Directional Light component Shadow camera was not set")
    shadow_far_clip_set_to_2000 = (
        "Shadow far clip value has been set to 2000.0",
        "Shadow far clip value could not be set to 2000.0")
    shadow_far_clip_set_to_default = (
        "Shadow far clip value has been set to 100.0",
        "Shadow far clip value could not be set to 0100.0")
    shadowmap_size_set_to_256 = (
        "Shadowmap size has been set to 256",
        "Shadowmap size could not be set to 256")
    shadowmap_size_set_to_default = (
        "Shadowmap size has been set to 1024",
        "Shadowmap size has been set to 1024")
    cascade_count_set_to_2 = (
        "Cascade count value has been set to 2",
        "Cascade count could not be set to 2")
    cascade_count_set_to_default = (
        "Cascade count has been set to 4",
        "Cascade count has not been set to 4")
    automatic_splitting_turned_off = (
        "Automatic splitting can be turned off",
        "Automatic splitting can not be turned off")
    automatic_splitting_turned_back_on = (
        "Automatic splitting can be turned back on",
        "Automatic splitting can not be turned back on")
    split_ratio_can_be_set_to_three_tenths = (
        "Automatic splitting value can be set 0.3",
        "Automatic splitting value can not be set 0.3")
    split_ratio_can_be_set_to_default = (
        "Automatic splitting value can be set to 0.9",
        "Automatic splitting value can not be set to 0.9")
    far_depth_cascade_set_to_test_value = (
        "Automatic splitting value can be set to X:15.0,Y:40.0,Z:65.0,W:90.0",
        "Automatic splitting value can not be set to X:15.0,Y:40.0,Z:65.0,W:90.0")
    far_depth_cascade_set_to_default = (
        "Automatic splitting value can be set to X:25.0,Y:50.0,Z:75.0,W:100.0",
        "Automatic splitting value can not be set to X:25.0,Y:50.0,Z:75.0,W:100.0")
    ground_height_set_to_ten = (
        "Ground height value can be set 10.0",
        "Ground height value can not be set 10.0")
    ground_height_set_to_default = (
        "Ground height value can be set 0.0",
        "Ground height value can not be set 0.0")
    cascade_correction_turned_on = (
        "Cascade correction can be turned on",
        "Cascade correction can not be turned on")
    cascade_correction_turned_back_off = (
        "Cascade correction can be turned back off",
        "Cascade correction can not be turned back off")
    debug_coloring_turned_on = (
        "Debug coloring can be turned on",
        "Debug coloring can not be turned on")
    debug_coloring_turned_back_off = (
        "Debug coloring can be turned back off",
        "Debug coloring can not be turned back off")
    shadow_filter_method_set_to_PCF = (
        "Shadow filter can be set to PCF",
        "Shadow filter can not be set to PCF")
    shadow_filter_method_set_to_ECM = (
        "Shadow filter can be set to ECM",
        "Shadow filter can not be set to ECM")
    shadow_filter_method_set_to_ECM_and_PCF = (
        "Shadow filter can be set to ECM+PCF",
        "Shadow filter can not be set to ECM+PCF")
    shadow_filter_method_set_to_None = (
        "Shadow filter can be set to None",
        "Shadow filter can not be set to None")
    filtering_sample_count_set_to_fifty = (
        "Filtering sample count value can be set to 50",
        "Filtering sample count value can not be set to 50")
    filtering_sample_count_set_to_default = (
        "Filtering sample count value can be set to 32",
        "Filtering sample count value can not be set to 32")
    shadow_receiver_plane_bias_off = (
        "Shadow receiver plane can be turned off",
        "Shadow receiver plane can not be turned off")
    shadow_receiver_plane_bias_back_on = (
        "Shadow receiver plane can be turned back on",
        "Shadow receiver plane can not be turned back on")
    shadow_bias_set_to_one_tenth = (
        "Shadow bias value can be set to 0.1",
        "Shadow bias value can not be set to 0.1")
    shadow_bias_count_set_to_default = (
        "Shadow bias value can be set to 0.002",
        "Shadow bias value can not be set to 0.002")
    normal_shadow_bias_set_to_seven = (
        "Normal Shadow bias value can be set to 7.0",
        "Normal Shadow bias value can not be set to 7.0")
    normal_shadow_bias_count_set_to_default = (
        "Normal Shadow bias value can be set to 2.5",
        "Normal Shadow bias value can not be set to 2.5")
    blend_between_cascades_off = (
        "Blend between cascades can be turned off",
        "Blend between cascades can not be turned off")
    blend_between_cascades_back_on = (
        "Blend between cascades can be turned back on",
        "Blend between cascades can not be turned back on")
    fullscreen_blur_off = (
        "Fullscreen blur can be turned off",
        "Fullscreen blur can not be turned off")
    fullscreen_blur_back_on = (
        "Fullscreen blur can be turned back on",
        "Fullscreen blur can not be turned back on")
    fullscreen_blur_strength_set_to_zero = (
        "Fullscreen blur strength can be set to 0.0",
        "Fullscreen blur strength can not be set to 0.0")
    fullscreen_blur_strength_set_to_default = (
        "Fullscreen blur strength value can be set to 0.667",
        "Fullscreen blur strength value can not be set to 0.667")
    fullscreen_blur_sharpness_set_to_240= (
        "Fullscreen blur sharpness can be set to 240.0",
        "Fullscreen blur sharpness value can not be set to 240.0")
    fullscreen_blur_sharpness_set_to_default = (
        "Fullscreen blur sharpness value can be set to 50.0",
        "Fullscreen blur sharpness value can not be set to 50.0")
    affects_gi_off = (
        "Affects GI can be turned off",
        "Affects GI can not be turned off")
    affects_gi_back_on = (
        "Affects GI can be turned back on",
        "Affects GI can not be turned back on")
    factor_set_to_2 = (
        "Factor value can be set to 2.0",
        "Factor value can not be set to 2.0")
    factor_set_to_default = (
        "Factor value can be set to 1.0",
        "Factorvalue can not be set to 1.0")
    enter_game_mode = (
        "Entered game mode",
        "Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "REDO deletion failed")


def AtomEditorComponents_DirectionalLight_AddedToEntity():
    """
    Summary:
    Tests the Directional Light component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Directional Light entity with no components.
    2) Add Directional Light component to Directional Light entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Test IsHidden.
    6) Test IsVisible.
    7) Set Color to purple, then back to default.
    8) Intensity mode set to Lux then back to Ev100.
    9) Set Intensity value to 10 then back to default of 4.
    10) Set Angular Diameter to 1.0 then back to default of 0.5.
    11) Add Camera entity.
    12) Add Camera component to Camera entity.
    13) Set the Directional Light component property Shadow|Camera to the Camera entity.
    14) Shadow Far Clip set to 2000 then back to the default of 100.
    15) Shadow Map size set to 256 then back to the default of 1024. (valid vales are 256, 512, 1024, 2048)
    16) Cascade Count set to 2 then back to the default of 4.
    17) Split ratio can be set to 0.3 then back to the default of 0.9.
    18) Turn Automatic Splitting off for subsequent tests.
    19) Far depth cascade can be set to test value then back to the default value.
    20) Turn Automatic Splitting back on.
    21) Turn Cascade correction on for subsequent test.
    22) Ground Height can be set to 10 then back to the default of 0.0.
    23) Turn Cascade correction back off.
    24) Turn Debug Coloring on then back off.
    25) Set Shadow filter method to PCF for subsequent tests.
    26) Filtering sample count size set to 50 then back to the default of 32.
    27) Turn Shadow Receiver Plane Bias Enable off then back on.
    28) Shadow Bias can be set to 0.1 then back to the default of 0.002.
    29) Normal Shadow Bias can be set to 7.0 then back to the default of 2.5.
    30) Turn Blend Between Cascades on then back off.
    31) Set Shadow filter method to ESM, PCF+ESM, then back to default of None.
    32) Turn Fullscreen Blur off then back on.
    33) Fullscreen Blur Strength can be set to 0.0 then back to the default of 0.667.
    34) Fullscreen Blur Sharpness can be set to 240.0 then back to the default of 50.
    35) Turn Affects GI Blur off then back on.
    36) Factor can be set to 2.0 then back to the default of 1.0.
    37) Enter/Exit game mode.
    38) Delete Directional Light entity.
    39) UNDO deletion.
    40) REDO deletion.
    41) Look for errors and asserts.

    :return: None
    """

    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import (
        AtomComponentProperties, DIRECTIONAL_LIGHT_INTENSITY_MODE, DIRECTIONAL_LIGHT_SHADOW_FILTER_METHOD)
    from azlmbr.math import Math_IsClose, Color, Vector4

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a Directional Light entity with no components.
        directional_light_entity = EditorEntity.create_editor_entity(AtomComponentProperties.directional_light())
        Report.critical_result(Tests.directional_light_creation, directional_light_entity.exists())

        # 2. Add Directional Light component to Directional Light entity.
        directional_light_component = directional_light_entity.add_component(AtomComponentProperties.directional_light())
        Report.critical_result(
            Tests.directional_light_component,
            directional_light_entity.has_component(AtomComponentProperties.directional_light()))

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
        Report.result(Tests.creation_undo, not directional_light_entity.exists())

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
        Report.result(Tests.creation_redo, directional_light_entity.exists())

        # 5. Test IsHidden.
        directional_light_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, directional_light_entity.is_hidden() is True)

        # 6. Test IsVisible.
        directional_light_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, directional_light_entity.is_visible() is True)

        # 7. Set Color to purple, then back to default.
        color_purple = Color(213.0, 0.0, 255.0, 255.0)
        color_default = Color(255.0, 255.0, 255.0, 255.0)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Color'), color_purple)
        general.idle_wait_frames(1)
        Report.result(
            Tests.color_set_to_purple,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Color')) == color_purple)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Color'),
            color_default)
        general.idle_wait_frames(1)
        Report.result(
            Tests.color_set_back_to_default,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Color')) == color_default)

        # 8. Intensity mode set to Lux then back to Ev100.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Intensity mode'), DIRECTIONAL_LIGHT_INTENSITY_MODE["Lux"])
        Report.result(
            Tests.intensity_mode_set_to_lux,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light(
                    'Intensity mode')) == DIRECTIONAL_LIGHT_INTENSITY_MODE["Lux"])

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Intensity mode'), DIRECTIONAL_LIGHT_INTENSITY_MODE["Ev100"])
        general.idle_wait_frames(1)
        Report.result(
            Tests.intensity_mode_set_to_ev100,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light(
                    'Intensity mode')) == DIRECTIONAL_LIGHT_INTENSITY_MODE["Ev100"])

        # 9. Set Intensity value to 10 then back to default of 4.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Intensity'), 10.0)
        Report.result(
            Tests.intensity_set_to_10,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Intensity')) == 10.0)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Intensity'), 4.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.intensity_set_to_default,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Intensity')) == 4.0)

        # 10. Set Angular Diameter to 1.0 then back to default of 0.5.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Angular diameter'), 1.0)
        Report.result(
            Tests.angular_diameter_set_to_1, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Angular diameter')), 1.0))

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Angular diameter'), 0.5)
        general.idle_wait_frames(1)
        Report.result(
            Tests.angular_diameter_set_to_default, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Angular diameter')), 0.5))

        # 11. Add Camera entity.
        camera_entity = EditorEntity.create_editor_entity(AtomComponentProperties.camera())
        Report.result(Tests.camera_creation, camera_entity.exists())

        # 12. Add Camera component to Camera entity.
        camera_entity.add_component(AtomComponentProperties.camera())
        Report.result(Tests.camera_component_added, camera_entity.has_component(AtomComponentProperties.camera()))

        # 13. Set the Directional Light component property Shadow|Camera to the Camera entity.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Camera'), camera_entity.id)
        Report.result(
            Tests.shadow_camera_check,
            camera_entity.id == directional_light_component.get_component_property_value(
                                    AtomComponentProperties.directional_light('Camera')))

        # 14. Shadow Far Clip set to 2000 then back to the default of 100.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Shadow far clip'), 2000.0)
        Report.result(
            Tests.intensity_set_to_10,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Shadow far clip')) == 2000.0)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Shadow far clip'), 100.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.intensity_set_to_default,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Shadow far clip')) == 100.0)

        # 15. Shadow Map size set to 256 then back to the default of 1024. (valid vales are 256, 512, 1024, 2048)
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Shadowmap size'), 256)
        Report.result(
            Tests.shadowmap_size_set_to_256,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Shadowmap size')) == 256)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Shadowmap size'), 1024)
        general.idle_wait_frames(1)
        Report.result(
            Tests.shadowmap_size_set_to_default,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Shadowmap size')) == 1024)

        # 16. Cascade Count set to 2 then back to the default of 4.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Cascade count'), 2)
        Report.result(
            Tests.cascade_count_set_to_2,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Cascade count')) == 2)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Cascade count'), 4)
        general.idle_wait_frames(1)
        Report.result(
            Tests.cascade_count_set_to_default,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Cascade count')) == 4)

        # 17. Split ratio can be set to 0.3 then back to the default of 0.9.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Split ratio'), 0.3)
        Report.result(
            Tests.split_ratio_can_be_set_to_three_tenths, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Split ratio')), 0.3))

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Split ratio'), 0.9)
        general.idle_wait_frames(1)
        Report.result(
            Tests.split_ratio_can_be_set_to_default, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Split ratio')), 0.9))

        # 18. Turn Automatic Splitting off for subsequent tests.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Automatic splitting'), False)
        general.idle_wait_frames(1)
        Report.result(
            Tests.automatic_splitting_turned_off,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Automatic splitting')) is False)

        # 19. Far depth cascade can be set to test value then back to the default value.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Far depth cascade'), Vector4(15.0, 40.0, 65.0, 90.0))
        Report.result(
            Tests.far_depth_cascade_set_to_test_value,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Far depth cascade')) == Vector4(15.0, 40.0, 65.0, 90.0))

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Far depth cascade'), Vector4(25.0, 50.0, 75.0, 100.0))
        general.idle_wait_frames(1)
        Report.result(
            Tests.far_depth_cascade_set_to_default,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Far depth cascade')) == Vector4(25.0, 50.0, 75.0, 100.0))

        # 20. Turn Automatic Splitting back on.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Automatic splitting'), True)
        Report.result(
            Tests.automatic_splitting_turned_back_on,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Automatic splitting')) is True)

        # 21. Turn Cascade correction on for subsequent test.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Cascade correction'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.cascade_correction_turned_on,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Cascade correction')) is True)

        # 22. Ground Height can be set to 10 then back to the default of 0.0.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Ground height'), 10.0)
        Report.result(
            Tests.ground_height_set_to_ten,
            directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Ground height')) == 10.0)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Ground height'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.ground_height_set_to_default,
            directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Ground height')) == 0.0)

        # 23. Turn Cascade correction back off.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Cascade correction'), False)
        Report.result(
            Tests.cascade_correction_turned_back_off,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Cascade correction')) is False)

        # 24. Turn Debug Coloring on then back off.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Debug coloring'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.debug_coloring_turned_on,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Debug coloring')) is True)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Debug coloring'), False)
        Report.result(
            Tests.debug_coloring_turned_back_off,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Debug coloring')) is False)

        # 25. Set Shadow filter method to PCF for subsequent tests.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light(
                'Shadow filter method'), DIRECTIONAL_LIGHT_SHADOW_FILTER_METHOD["PCF"])
        Report.result(
            Tests.shadow_filter_method_set_to_PCF,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light(
                    'Shadow filter method')) == DIRECTIONAL_LIGHT_SHADOW_FILTER_METHOD["PCF"])

        # 26. Filtering sample count size set to 50 then back to the default of 32.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Filtering sample count'), 50.0)
        Report.result(
            Tests.filtering_sample_count_set_to_fifty,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Filtering sample count')) == 50)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Filtering sample count'), 32.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.filtering_sample_count_set_to_default,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Filtering sample count')) == 32.0)

        # 27. Turn Shadow Receiver Plane Bias Enable off then back on.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Shadow Receiver Plane Bias Enable'), False)
        general.idle_wait_frames(1)
        Report.result(
            Tests.shadow_receiver_plane_bias_off,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Shadow Receiver Plane Bias Enable')) is False)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Shadow Receiver Plane Bias Enable'), True)
        Report.result(
            Tests.shadow_receiver_plane_bias_back_on,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Shadow Receiver Plane Bias Enable')) is True)

        # 28. Shadow Bias can be set to 0.1 then back to the default of 0.002.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Shadow Bias'), 0.1)
        Report.result(
            Tests.shadow_bias_set_to_one_tenth, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Shadow Bias')), 0.1))

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Shadow Bias'), 0.002)
        general.idle_wait_frames(1)
        Report.result(
            Tests.shadow_bias_count_set_to_default, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Shadow Bias')), 0.002))

        # 29. Normal Shadow Bias can be set to 7.0 then back to the default of 2.5.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Normal Shadow Bias'), 7.0)
        Report.result(
            Tests.normal_shadow_bias_set_to_seven, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Normal Shadow Bias')), 7.0))

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Normal Shadow Bias'), 2.5)
        general.idle_wait_frames(1)
        Report.result(
            Tests.normal_shadow_bias_count_set_to_default, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Normal Shadow Bias')), 2.5))

        # 30. Turn Blend Between Cascades on then back off.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Blend between cascades'), True)
        general.idle_wait_frames(1)
        Report.result(
            Tests.blend_between_cascades_off,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Blend between cascades')) is True)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Blend between cascades'), False)
        Report.result(
            Tests.blend_between_cascades_back_on,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Blend between cascades')) is False)

        # 31. Set Shadow filter method to ESM, PCF+ESM, then back to default of None.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light(
                'Shadow filter method'), DIRECTIONAL_LIGHT_SHADOW_FILTER_METHOD["ESM"])
        Report.result(
            Tests.shadow_filter_method_set_to_ECM,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light(
                    'Shadow filter method')) == DIRECTIONAL_LIGHT_SHADOW_FILTER_METHOD["ESM"])

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light(
                'Shadow filter method'), DIRECTIONAL_LIGHT_SHADOW_FILTER_METHOD["PCF+ESM"])
        Report.result(
            Tests.shadow_filter_method_set_to_ECM_and_PCF,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light(
                    'Shadow filter method')) == DIRECTIONAL_LIGHT_SHADOW_FILTER_METHOD["PCF+ESM"])

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light(
                'Shadow filter method'), DIRECTIONAL_LIGHT_SHADOW_FILTER_METHOD["None"])
        Report.result(
            Tests.shadow_filter_method_set_to_None,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light(
                    'Shadow filter method')) == DIRECTIONAL_LIGHT_SHADOW_FILTER_METHOD["None"])

        # 32. Turn Fullscreen Blur off then back on.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Fullscreen Blur'), False)
        general.idle_wait_frames(1)
        Report.result(
            Tests.fullscreen_blur_off,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Fullscreen Blur')) is False)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Fullscreen Blur'), True)
        Report.result(
            Tests.fullscreen_blur_back_on,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Fullscreen Blur')) is True)

        # 33. Fullscreen Blur Strength can be set to 0.0 then back to the default of 0.667.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Fullscreen Blur Strength'), 0.0)
        Report.result(
            Tests.fullscreen_blur_strength_set_to_zero, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Fullscreen Blur Strength')), 0.0))

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Fullscreen Blur Strength'), 0.667)
        general.idle_wait_frames(1)
        Report.result(
            Tests.fullscreen_blur_strength_set_to_default, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Fullscreen Blur Strength')), 0.667))

        # 34. Fullscreen Blur Sharpness can be set to 240.0 then back to the default of 50.0.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Fullscreen Blur Sharpness'), 240.0)
        Report.result(
            Tests.fullscreen_blur_sharpness_set_to_240, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Fullscreen Blur Sharpness')), 240.0))

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Fullscreen Blur Sharpness'), 50.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.fullscreen_blur_sharpness_set_to_default, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Fullscreen Blur Sharpness')), 50.0))

        # 35. Turn Affects GI Blur off then back on.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Affects GI'), False)
        general.idle_wait_frames(1)
        Report.result(
            Tests.affects_gi_off,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Affects GI')) is False)

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Affects GI'), True)
        Report.result(
            Tests.affects_gi_back_on,
            directional_light_component.get_component_property_value(
                AtomComponentProperties.directional_light('Affects GI')) is True)

        # 36. Factor can be set to 2.0 then back to the default of 1.0.
        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Factor'), 2.0)
        Report.result(
            Tests.factor_set_to_2, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Factor')), 2.0))

        directional_light_component.set_component_property_value(
            AtomComponentProperties.directional_light('Factor'), 1.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.factor_set_to_default, Math_IsClose(
                directional_light_component.get_component_property_value(
                    AtomComponentProperties.directional_light('Factor')), 1.0))

        # 37. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 38. Delete Directional Light entity.
        directional_light_entity.delete()
        Report.result(Tests.entity_deleted, not directional_light_entity.exists())

        # 39. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, directional_light_entity.exists())

        # 40. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not directional_light_entity.exists())

        # 41. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_DirectionalLight_AddedToEntity)
