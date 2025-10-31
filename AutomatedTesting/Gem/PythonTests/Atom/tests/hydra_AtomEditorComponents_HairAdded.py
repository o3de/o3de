"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    hair_creation = (
        "Hair Entity successfully created",
        "P0: Hair Entity failed to be created")
    hair_component = (
        "Entity has a Hair component",
        "P0: Entity failed to find Hair component")
    hair_component_removal = (
        "Hair component removed",
        "P0: Hair component removal failed")
    hair_disabled = (
        "Hair component disabled",
        "P0: Hair component was not disabled.")
    actor_component = (
        "Entity has an Actor component",
        "P0: Entity did not have an Actor component")
    hair_enabled = (
        "Hair component enabled",
        "P0: Hair component was not enabled.")
    hair_asset = (
        "Hair Asset property set",
        "P0: Hair Asset property was not set as expected")
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
    enable_area_lights = (
        "Enable Area Lights property set",
        "P1: 'Enable Area Lights' property failed to set")
    enable_azimuth = (
        "Enable Azimuth property set",
        "P1: 'Enable Azimuth' property failed to set")
    enable_directional_lights = (
        "Enable Directional Lights property set",
        "P1: 'Enable Directional Lights' property failed to set")
    enable_ibl = (
        "Enable IBL property set",
        "P1: 'Enable IBL' property failed to set")
    enable_longitude = (
        "Enable Longitude property set",
        "P1: 'Enable Longitude' property failed to set")
    enable_marschner_r = (
        "Enable Marschner R property set",
        "P1: 'Enable Marschner R' property failed to set")
    enable_marschner_trt = (
        "Enable Marschner TRT property set",
        "P1: 'Enable Marschner TRT' property failed to set")
    enable_marschner_tt = (
        "Enable Marschner TT property set",
        "P1: 'Enable Marschner TT' property failed to set")
    enable_punctual_lights = (
        "Enable Punctual Lights property set",
        "P1: 'Enable Punctual Lights' property failed to set")
    enable_shadows = (
        "Enable Shadows property set",
        "P1: 'Enable Shadows' property failed to set")
    base_albedo_asset = (
        "Base Albedo Asset property set",
        "P1: 'Base Albedo Asset' property failed to set")
    base_color = (
        "Base Color property set",
        "P1: 'Base Color' property failed to set")
    enable_hair_lod = (
        "Enable Hair LOD property set",
        "P1: 'Enable Hair LOD' property failed to set")
    enable_hair_lod_shadow = (
        "Enable Hair LOD(Shadow) property set",
        "P1: 'Enable Hair LOD(Shadow)' property failed to set")
    enable_strand_tangent = (
        "Enable Strand Tangent property set",
        "P1: 'Enable Strand Tangent' property failed to set")
    enable_strand_uv = (
        "Enable Strand UV property set",
        "P1: 'Enable Strand UV' property failed to set")
    enable_thin_tip = (
        "Enable Thin Tip property set",
        "P1: 'Enable Thin Tip' property failed to set")
    fiber_radius = (
        "Fiber Radius property set",
        "P1: 'Fiber Radius' property failed to set")
    fiber_spacing = (
        "Fiber Spacing property set",
        "P1: 'Fiber Spacing' property failed to set")
    fiber_ratio = (
        "Fiber ratio property set",
        "P1: 'Fiber ratio' property failed to set")
    hair_cuticle_angle = (
        "Hair Cuticle Angle property set",
        "P1: 'Hair Cuticle Angle' property failed to set")
    hair_ex1 = (
        "Hair Ex1 property set",
        "P1: 'Hair Ex1' property failed to set")
    hair_ex2 = (
        "Hair Ex2 property set",
        "P1: 'Hair Ex2' property failed to set")
    hair_kdiffuse = (
        "Hair Kdiffuse property set",
        "P1: 'Hair Kdiffuse' property failed to set")
    hair_ks1 = (
        "Hair Ks1 property set",
        "P1: 'Hair Ks1' property failed to set")
    hair_ks2 = (
        "Hair Ks2 property set",
        "P1: 'Hair Ks2' property failed to set")
    hair_roughness = (
        "Hair Roughness property set",
        "P1: 'Hair Roughness' property failed to set")
    hair_shadow_alpha = (
        "Hair Shadow Alpha property set",
        "P1: 'Hair Shadow Alpha' property failed to set")
    lod_end_distance = (
        "LOD End Distance property set",
        "P1: 'LOD End Distance' property failed to set")
    lod_start_distance = (
        "LOD Start Distance property set",
        "P1: 'LOD Start Distance' property failed to set")
    mat_tip_color = (
        "Mat Tip Color property set",
        "P1: 'Mat Tip Color' property failed to set")
    max_lod_reduction = (
        "Max LOD Reduction property set",
        "P1: 'Max LOD Reduction' property failed to set")
    max_lod_strand_width_multiplier = (
        "Max LOD Strand Width Multiplier property set",
        "P1: 'Max LOD Strand Width Multiplier' property failed to set")
    max_shadow_fibers = (
        "Max Shadow Fibers property set",
        "P1: 'Max Shadow Fibers' property failed to set")
    shadow_lod_end_distance = (
        "Shadow LOD End Distance property set",
        "P1: 'Shadow LOD End Distance' property failed to set")
    shadow_lod_start_distance = (
        "Shadow LOD Start Distance property set",
        "P1: 'Shadow LOD Start Distance' property failed to set")
    shadow_max_lod_reduction = (
        "Shadow Max LOD Reduction property set",
        "P1: 'Shadow Max LOD Reduction' property failed to set")
    shadow_max_lod_strand_width_multiplier = (
        "Shadow Max LOD Strand Width Multiplier property set",
        "P1: 'Shadow Max LOD Strand Width Multiplier' property failed to set")
    strand_albedo_asset = (
        "Strand Albedo Asset property set",
        "P1: 'Strand Albedo Asset' property failed to set")
    strand_uvtiling_factor = (
        "Strand UVTiling Factor property set",
        "P1: 'Strand UVTiling Factor' property failed to set")
    tip_percentage = (
        "Tip Percentage property set",
        "P1: 'Tip Percentage' property failed to set")
    clamp_velocity = (
        "Clamp Velocity property set",
        "P1: 'Clamp Velocity' property failed to set")
    damping = (
        "Damping property set",
        "P1: 'Damping' property failed to set")
    global_constraint_range = (
        "Global Constraint Range property set",
        "P1: 'Global Constraint Range' property failed to set")
    global_constraint_stiffness = (
        "Global Constraint Stiffness property set",
        "P1: 'Global Constraint Stiffness' property failed to set")
    gravity_magnitude = (
        "Gravity Magnitude property set",
        "P1: 'Gravity Magnitude' property failed to set")
    length_constraint_iterations = (
        "Length Constraint Iterations property set",
        "P1: 'Length Constraint Iterations' property failed to set")
    local_constraint_iterations = (
        "Local Constraint Iterations property set",
        "P1: 'Local Constraint Iterations' property failed to set")
    local_constraint_stiffness = (
        "Local Constraint Stiffness property set",
        "P1: 'Local Constraint Stiffness' property failed to set")
    tip_separation = (
        "Tip Separation property set",
        "P1: 'Tip Separation' property failed to set")
    vsp_accel_threshold = (
        "Vsp Accel Threshold property set",
        "P1: 'Vsp Accel Threshold' property failed to set")
    vsp_coeffs = (
        "Vsp Coeffs property set",
        "P1: 'Vsp Coeffs' property failed to set")
    wind_angle_radians = (
        "Wind Angle Radians property set",
        "P1: 'Wind Angle Radians' property failed to set")
    wind_direction = (
        "Wind Direction property set",
        "P1: 'Wind Direction' property failed to set")
    wind_magnitude = (
        "Wind Magnitude property set",
        "P1: 'Wind Magnitude' property failed to set")


def AtomEditorComponents_Hair_AddedToEntity():
    """
    Summary:
    Tests the Hair component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Hair entity with no components.
    2) Add a Hair component to Hair entity.
    3) Remove Hair component.
    4) UNDO the component removal.
    5) Verify Hair component not enabled.
    6) Add Actor component since it is required by the Hair component.
    7) Verify Hair component is enabled.
    8) Set assets for Actor and Hair
    9) Set 'Base Albedo Asset' "testdata", "test_hair_skin_basecolor.png.streamingimage"
    10) Set 'Strand Albedo Asset' "testdata", "strandalbedo.png.streamingimage"
    11) Toggle 'Enable Area Lights'
    12) Toggle 'Enable Azimuth'
    13) Toggle 'Enable Directional Lights'
    14) Toggle 'Enable IBL'
    15) Toggle 'Enable Longitude'
    16) Toggle 'Enable Marschner R'
    17) Toggle 'Enable Marschner TRT'
    18) Toggle 'Enable Marschner TT'
    19) Toggle 'Enable Punctual Lights'
    20) Toggle 'Enable Shadows'
    21) Toggle 'Enable Hair LOD'
    22) Toggle 'Enable Hair LOD(Shadow)'
    23) Toggle 'Enable Strand Tangent'
    24) Toggle 'Enable Strand UV'
    25) Toggle 'Enable Thin Tip'
    26) Set each 'Hair Lighting Model' from atom_constants.py HAIR_LIGHTING_MODEL default Marschner
    27) Set 'Base Color'
    28) Set 'Fiber Radius'
    29) Set 'Fiber Spacing'
    30) Set 'Fiber ratio'
    31) Set 'Hair Cuticle Angle'
    32) Set 'Hair Ex1'
    33) Set 'Hair Ex2'
    34) Set 'Hair Kdiffuse'
    35) Set 'Hair Ks1'
    36) Set 'Hair Ks2'
    37) Set 'Hair Roughness'
    38) Set 'Hair Shadow Alpha'
    39) Set 'LOD End Distance'
    40) Set 'LOD Start Distance'
    41) Set 'Mat Tip Color'
    42) Set 'Max LOD Reduction'
    43) Set 'Max LOD Strand Width Multiplier'
    44) Set 'Max Shadow Fibers'
    45) Set 'Shadow LOD End Distance'
    46) Set 'Shadow LOD Start Distance'
    47) Set 'Shadow Max LOD Reduction'
    48) Set 'Shadow Max LOD Strand Width Multiplier'
    49) Set 'Strand UVTiling Factor'
    50) Set 'Tip Percentage'
    51) Set 'Clamp Velocity'
    52) Set 'Damping'
    53) Set 'Global Constraint Range'
    54) Set 'Global Constraint Stiffness'
    55) Set 'Gravity Magnitude'
    56) Set 'Length Constraint Iterations'
    57) Set 'Local Constraint Iterations'
    58) Set 'Local Constraint Stiffness'
    59) Set 'Tip Separation'
    60) Set 'Vsp Accel Threshold'
    61) Set 'Vsp Coeffs'
    62) Set 'Wind Angle Radians'
    63) Set 'Wind Direction'
    64) Set 'Wind Magnitude'
    65) Enter/Exit game mode.
    66) Test IsHidden.
    67) Test IsVisible.
    68) Delete Hair entity.
    69) UNDO deletion.
    70) REDO deletion.
    71) Look for errors or asserts.

    :return: None
    """

    import azlmbr.legacy.general as general

    from azlmbr.math import Math_IsClose, Color, Vector3
    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties, HAIR_LIGHTING_MODEL

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a Hair entity with no components.
        hair_entity = EditorEntity.create_editor_entity(AtomComponentProperties.hair())
        Report.critical_result(Tests.hair_creation, hair_entity.exists())

        # 2. Add a Hair component to Hair entity.
        hair_component = hair_entity.add_component(AtomComponentProperties.hair())
        Report.critical_result(
            Tests.hair_component,
            hair_entity.has_component(AtomComponentProperties.hair()))

        # 3. Remove Hair component.
        hair_component.remove()
        general.idle_wait_frames(1)
        Report.result(
            Tests.hair_component_removal,
            not hair_entity.has_component(AtomComponentProperties.hair()))

        # 4. UNDO the component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.critical_result(
            Tests.hair_component,
            hair_entity.has_component(AtomComponentProperties.hair()))

        # 5. Verify Hair component not enabled.
        Report.result(Tests.hair_disabled, not hair_component.is_enabled())

        # 6. Add Actor component since it is required by the Hair component.
        actor_component = hair_entity.add_component(AtomComponentProperties.actor())
        Report.result(Tests.actor_component, hair_entity.has_component(AtomComponentProperties.actor()))

        # 7. Verify Hair component is enabled.
        Report.result(Tests.hair_enabled, hair_component.is_enabled())

        # 8. Set assets for Actor and Hair
        head_path = os.path.join('testdata', 'headbonechainhairstyle', 'test_hair_bone_chain_head_style.fbx.azmodel')
        head_asset = Asset.find_asset_by_path(head_path)
        hair_path = os.path.join('testdata', 'headbonechainhairstyle', 'test_hair_bone_chain_head_style.tfxhair')
        hair_asset = Asset.find_asset_by_path(hair_path)
        actor_component.set_component_property_value(AtomComponentProperties.actor('Actor asset'), head_asset.id)
        hair_component.set_component_property_value(AtomComponentProperties.hair('Hair Asset'), hair_asset.id)
        Report.result(
            Tests.hair_asset,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Hair Asset')) == hair_asset.id)

        # 9. Set 'Base Albedo Asset' "testdata", "test_hair_skin_basecolor.png.streamingimage"
        base_albedo_path = os.path.join('testdata', 'test_hair_skin_basecolor.png.streamingimage')
        base_albede_asset = Asset.find_asset_by_path(base_albedo_path)
        hair_component.set_component_property_value(
            AtomComponentProperties.hair('Base Albedo Asset'), base_albede_asset.id)
        Report.result(
            Tests.base_albedo_asset,
            hair_component.get_component_property_value(
                AtomComponentProperties.hair('Base Albedo Asset')) == base_albede_asset.id)

        # 10. Set 'Strand Albedo Asset' "testdata", "strandalbedo.png.streamingimage"
        strand_path = os.path.join('testdata', 'strandalbedo.png.streamingimage')
        strand_asset = Asset.find_asset_by_path(strand_path)
        hair_component.set_component_property_value(
            AtomComponentProperties.hair('Strand Albedo Asset'), strand_asset.id)
        Report.result(
            Tests.strand_albedo_asset,
            hair_component.get_component_property_value(
                AtomComponentProperties.hair('Strand Albedo Asset')) == strand_asset.id)

        # 11. Toggle 'Enable Area Lights'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Area Lights'), False)
        Report.result(
            Tests.enable_area_lights,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Area Lights')) is False)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Area Lights'), True)
        Report.result(
            Tests.enable_area_lights,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Area Lights')) is True)

        # 12. Toggle 'Enable Azimuth'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Azimuth'), False)
        Report.result(
            Tests.enable_azimuth,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Azimuth')) is False)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Azimuth'), True)
        Report.result(
            Tests.enable_azimuth,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Azimuth')) is True)

        # 13. Toggle 'Enable Directional Lights'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Directional Lights'), False)
        Report.result(
            Tests.enable_directional_lights,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Directional Lights')) is False)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Directional Lights'), True)
        Report.result(
            Tests.enable_directional_lights,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Directional Lights')) is True)

        # 14. Toggle 'Enable IBL'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable IBL'), False)
        Report.result(
            Tests.enable_ibl,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable IBL')) is False)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable IBL'), True)
        Report.result(
            Tests.enable_ibl,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable IBL')) is True)

        # 15. Toggle 'Enable Longitude'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Longitude'), False)
        Report.result(
            Tests.enable_longitude,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Longitude')) is False)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Longitude'), True)
        Report.result(
            Tests.enable_longitude,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Longitude')) is True)

        # 16. Toggle 'Enable Marschner R'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Marschner R'), False)
        Report.result(
            Tests.enable_marschner_r,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Marschner R')) is False)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Marschner R'), True)
        Report.result(
            Tests.enable_marschner_r,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Marschner R')) is True)

        # 17. Toggle 'Enable Marschner TRT'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Marschner TRT'), False)
        Report.result(
            Tests.enable_marschner_trt,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Marschner TRT')) is False)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Marschner TRT'), True)
        Report.result(
            Tests.enable_marschner_trt,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Marschner TRT')) is True)

        # 18. Toggle 'Enable Marschner TT'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Marschner TT'), False)
        Report.result(
            Tests.enable_marschner_tt,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Marschner TT')) is False)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Marschner TT'), True)
        Report.result(
            Tests.enable_marschner_tt,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Marschner TT')) is True)

        # 19. Toggle 'Enable Punctual Lights'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Punctual Lights'), False)
        Report.result(
            Tests.enable_punctual_lights,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Punctual Lights')) is False)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Punctual Lights'), True)
        Report.result(
            Tests.enable_punctual_lights,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Punctual Lights')) is True)

        # 20. Toggle 'Enable Shadows'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Shadows'), False)
        Report.result(
            Tests.enable_shadows,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Shadows')) is False)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Shadows'), True)
        Report.result(
            Tests.enable_shadows,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Shadows')) is True)

        # 21. Toggle 'Enable Hair LOD'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Hair LOD'), True)
        Report.result(
            Tests.enable_hair_lod,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Hair LOD')) is True)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Hair LOD'), False)
        Report.result(
            Tests.enable_hair_lod,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Hair LOD')) is False)

        # 22. Toggle 'Enable Hair LOD(Shadow)'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Hair LOD(Shadow)'), True)
        Report.result(
            Tests.enable_hair_lod_shadow,
            hair_component.get_component_property_value(
                AtomComponentProperties.hair('Enable Hair LOD(Shadow)')) is True)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Hair LOD(Shadow)'), False)
        Report.result(
            Tests.enable_hair_lod_shadow,
            hair_component.get_component_property_value(
                AtomComponentProperties.hair('Enable Hair LOD(Shadow)')) is False)

        # 23. Toggle 'Enable Strand Tangent'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Strand Tangent'), True)
        Report.result(
            Tests.enable_strand_tangent,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Strand Tangent')) is True)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Strand Tangent'), False)
        Report.result(
            Tests.enable_strand_tangent,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Strand Tangent')) is False)

        # 24. Toggle 'Enable Strand UV'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Strand UV'), True)
        Report.result(
            Tests.enable_strand_uv,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Strand UV')) is True)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Strand UV'), False)
        Report.result(
            Tests.enable_strand_uv,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Strand UV')) is False)

        # 25. Toggle 'Enable Thin Tip'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Thin Tip'), False)
        Report.result(
            Tests.enable_thin_tip,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Thin Tip')) is False)

        hair_component.set_component_property_value(AtomComponentProperties.hair('Enable Thin Tip'), True)
        Report.result(
            Tests.enable_thin_tip,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Enable Thin Tip')) is True)

        # 26. Set each 'Hair Lighting Model' from atom_constants.py HAIR_LIGHTING_MODEL default Marschner
        for model in HAIR_LIGHTING_MODEL:
            hair_component.set_component_property_value(
                AtomComponentProperties.hair('Hair Lighting Model'), HAIR_LIGHTING_MODEL[model])
            general.idle_wait_frames(5)
            hair_lighting_model = (
                f"Hair Lighting Model property set to {model}",
                f"P1: 'Hair Lighting Model' property failed to set {model}")
            Report.result(
                hair_lighting_model,
                hair_component.get_component_property_value(
                    AtomComponentProperties.hair('Hair Lighting Model')) == HAIR_LIGHTING_MODEL[model])
        hair_component.set_component_property_value(
            AtomComponentProperties.hair('Hair Lighting Model'), HAIR_LIGHTING_MODEL['Marschner'])
        general.idle_wait_frames(5)

        # 27. Set 'Base Color'
        red = Color(255.0, 0.0, 0.0, 161.0)
        hair_component.set_component_property_value(AtomComponentProperties.hair('Base Color'), red)
        Report.result(
            Tests.base_color,
            hair_component.get_component_property_value(
                AtomComponentProperties.hair('Base Color')) == red)

        # 28. Set 'Fiber Radius'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Fiber Radius'), 0.009)
        Report.result(
            Tests.fiber_radius,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Fiber Radius')), 0.009))

        # 29. Set 'Fiber Spacing'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Fiber Spacing'), 0.9)
        Report.result(
            Tests.fiber_spacing,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Fiber Spacing')), 0.9))

        # 30. Set 'Fiber ratio'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Fiber ratio'), 0.09)
        Report.result(
            Tests.fiber_ratio,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Fiber ratio')), 0.09))

        # 31. Set 'Hair Cuticle Angle'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Hair Cuticle Angle'), 0.9)
        Report.result(
            Tests.hair_cuticle_angle,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Hair Cuticle Angle')), 0.9))

        # 32. Set 'Hair Ex1'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Hair Ex1'), 99.0)
        Report.result(
            Tests.hair_ex1,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Hair Ex1')), 99.0))

        general.idle_wait_frames(1)

        # 33. Set 'Hair Ex2'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Hair Ex2'), 100.0)
        Report.result(
            Tests.hair_ex2,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Hair Ex2')), 100.0))

        # 34. Set 'Hair Kdiffuse'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Hair Kdiffuse'), 0.9)
        Report.result(
            Tests.hair_kdiffuse,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Hair Kdiffuse')), 0.9))

        # 35. Set 'Hair Ks1'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Hair Ks1'), 0.009)
        Report.result(
            Tests.hair_ks1,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Hair Ks1')), 0.009))

        # 36. Set 'Hair Ks2'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Hair Ks2'), 0.199)
        Report.result(
            Tests.hair_ks2,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Hair Ks2')), 0.199))

        # 37. Set 'Hair Roughness'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Hair Roughness'), 0.9)
        Report.result(
            Tests.hair_roughness,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Hair Roughness')), 0.9))

        # 38. Set 'Hair Shadow Alpha'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Hair Shadow Alpha'), 0.6)
        Report.result(
            Tests.hair_shadow_alpha,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Hair Shadow Alpha')), 0.6))

        general.idle_wait_frames(1)

        # 39. Set 'LOD End Distance'
        hair_component.set_component_property_value(AtomComponentProperties.hair('LOD End Distance'), 9.0)
        Report.result(
            Tests.lod_end_distance,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('LOD End Distance')), 9.0))

        # 40. Set 'LOD Start Distance'
        hair_component.set_component_property_value(AtomComponentProperties.hair('LOD Start Distance'), 0.9)
        Report.result(
            Tests.lod_start_distance,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('LOD Start Distance')), 0.9))

        # 41. Set 'Mat Tip Color'
        blue = Color(0.0, 0.0, 255.0, 161.0)
        hair_component.set_component_property_value(AtomComponentProperties.hair('Mat Tip Color'), blue)
        Report.result(
            Tests.mat_tip_color,
            hair_component.get_component_property_value(
                AtomComponentProperties.hair('Mat Tip Color')) == blue)

        # 42. Set 'Max LOD Reduction'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Max LOD Reduction'), 0.6)
        Report.result(
            Tests.max_lod_reduction,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Max LOD Reduction')), 0.6))

        # 43. Set 'Max LOD Strand Width Multiplier'
        hair_component.set_component_property_value(
            AtomComponentProperties.hair('Max LOD Strand Width Multiplier'), 6.0)
        Report.result(
            Tests.max_lod_strand_width_multiplier,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Max LOD Strand Width Multiplier')), 6.0))

        # 44. Set 'Max Shadow Fibers'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Max Shadow Fibers'), 100)
        Report.result(
            Tests.max_shadow_fibers,
            hair_component.get_component_property_value(
                AtomComponentProperties.hair('Max Shadow Fibers')) == 100)

        general.idle_wait_frames(1)

        # 45. Set 'Shadow LOD End Distance'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Shadow LOD End Distance'), 6.0)
        Report.result(
            Tests.shadow_lod_end_distance,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Shadow LOD End Distance')), 6.0))

        # 46. Set 'Shadow LOD Start Distance'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Shadow LOD Start Distance'), 0.6)
        Report.result(
            Tests.shadow_lod_start_distance,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Shadow LOD Start Distance')), 0.6))

        # 47. Set 'Shadow Max LOD Reduction'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Shadow Max LOD Reduction'), 0.9)
        Report.result(
            Tests.shadow_max_lod_reduction,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Shadow Max LOD Reduction')), 0.9))

        # 48. Set 'Shadow Max LOD Strand Width Multiplier'
        hair_component.set_component_property_value(
            AtomComponentProperties.hair('Shadow Max LOD Strand Width Multiplier'), 6.0)
        Report.result(
            Tests.shadow_max_lod_strand_width_multiplier,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Shadow Max LOD Strand Width Multiplier')), 6.0))

        # 49. Set 'Strand UVTiling Factor'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Strand UVTiling Factor'), 9.0)
        Report.result(
            Tests.strand_uvtiling_factor,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Strand UVTiling Factor')), 9.0))

        # 50. Set 'Tip Percentage'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Tip Percentage'), 0.6)
        Report.result(
            Tests.tip_percentage,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Tip Percentage')), 0.6))

        general.idle_wait_frames(1)

        # 51. Set 'Clamp Velocity'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Clamp Velocity'), 1.0)
        Report.result(
            Tests.clamp_velocity,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Clamp Velocity')), 1.0))

        # 52. Set 'Damping'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Damping'), 0.6)
        Report.result(
            Tests.damping,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Damping')), 0.6))

        # 53. Set 'Global Constraint Range'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Global Constraint Range'), 0.999)
        Report.result(
            Tests.global_constraint_range,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Global Constraint Range')), 0.999))

        # 54. Set 'Global Constraint Stiffness'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Global Constraint Stiffness'), 0.666)
        Report.result(
            Tests.global_constraint_stiffness,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Global Constraint Stiffness')), 0.666))

        # 55. Set 'Gravity Magnitude'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Gravity Magnitude'), 0.66)
        Report.result(
            Tests.gravity_magnitude,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Gravity Magnitude')), 0.66))

        # 56. Set 'Length Constraint Iterations'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Length Constraint Iterations'), 9)
        Report.result(
            Tests.length_constraint_iterations,
            hair_component.get_component_property_value(
                AtomComponentProperties.hair('Length Constraint Iterations')) == 9)

        general.idle_wait_frames(1)

        # 57. Set 'Local Constraint Iterations'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Local Constraint Iterations'), 1)
        Report.result(
            Tests.local_constraint_iterations,
            hair_component.get_component_property_value(
                AtomComponentProperties.hair('Local Constraint Iterations')) == 1)

        # 58. Set 'Local Constraint Stiffness'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Local Constraint Stiffness'), 0.999)
        Report.result(
            Tests.local_constraint_stiffness,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Local Constraint Stiffness')), 0.999))

        # 59. Set 'Tip Separation'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Tip Separation'), 0.6)
        Report.result(
            Tests.tip_separation,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Tip Separation')), 0.6))

        # 60. Set 'Vsp Accel Threshold'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Vsp Accel Threshold'), 10.0)
        Report.result(
            Tests.vsp_accel_threshold,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Vsp Accel Threshold')), 10.0))

        # 61. Set 'Vsp Coeffs'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Vsp Coeffs'), 0.999)
        Report.result(
            Tests.vsp_coeffs,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Vsp Coeffs')), 0.999))

        # 62. Set 'Wind Angle Radians'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Wind Angle Radians'), 0.999)
        Report.result(
            Tests.wind_angle_radians,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Wind Angle Radians')), 0.999))

        general.idle_wait_frames(1)

        # 63. Set 'Wind Direction'
        x_direction = Vector3(0.9, 0.0, 0.2)
        hair_component.set_component_property_value(AtomComponentProperties.hair('Wind Direction'), x_direction)
        Report.result(
            Tests.wind_direction,
            hair_component.get_component_property_value(
                AtomComponentProperties.hair('Wind Direction')) == x_direction)

        # 64. Set 'Wind Magnitude'
        hair_component.set_component_property_value(AtomComponentProperties.hair('Wind Magnitude'), 1.0)
        Report.result(
            Tests.wind_magnitude,
            Math_IsClose(hair_component.get_component_property_value(
                AtomComponentProperties.hair('Wind Magnitude')), 1.0))

        # 65. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 66. Test IsHidden.
        hair_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, hair_entity.is_hidden() is True)

        # 67. Test IsVisible.
        hair_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, hair_entity.is_visible() is True)

        # 68. Delete Hair entity.
        hair_entity.delete()
        Report.result(Tests.entity_deleted, not hair_entity.exists())

        # 69. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, hair_entity.exists())

        # 70. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not hair_entity.exists())

        # 71. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Hair_AddedToEntity)
