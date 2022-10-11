"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    creation_undo = (
        "UNDO level component addition success",
        "P0: UNDO level component addition failed")
    creation_redo = (
        "REDO Level component addition success",
        "P0: REDO Level component addition failed")
    display_mapper_component = (
        "Level has a Display Mapper component",
        "P0: Level failed to find Display Mapper component")
    ldr_color_grading_lut = (
        "LDR color Grading LUT asset set",
        "P0: LDR color Grading LUT asset could not be set")
    enable_ldr_color_grading_lut = (
        "Enable LDR color grading LUT set",
        "P0: Enable LDR color grading LUT could not be set")
    enter_game_mode = (
        "Entered game mode",
        "P0: Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "P0: Couldn't exit game mode")
    override_defaults = (
        "Override Defaults property set",
        "P1: Override Defaults property failed to be set correctly")
    alter_surround = (
        "Alter Surround property set",
        "P1: Alter Surround property failed to be set correctly")
    alter_desaturation = (
        "Alter Desaturation property set",
        "P1: Alter Desaturation property failed to be set correctly")
    alter_cat = (
        "Alter CAT D60 to D65 property set",
        "P1: Alter CAT D60 to D65 property failed to be set correctly")
    black_level_min = (
        "Cinema Limit (black) property set to minimum value",
        "P1: Cinema Limit (black) property failed to take minimum value")
    black_level_max = (
        "Cinema Limit (black) property set to maximum value",
        "P1: Cinema Limit (black) property failed to take maximum value")
    white_level_min = (
        "Cinema Limit (white) property set to minimum value",
        "P1: Cinema Limit (white) property failed to take minimum value")
    white_level_max = (
        "Cinema Limit (white) property set to maximum value",
        "P1: Cinema Limit (white) property failed to take maximum value")
    luminance_level_min = (
        "Min Point (luminance) property set",
        "P1: Min Point (luminance) property failed to set expected value")
    luminance_level_mid = (
        "Mid Point (luminance) property set",
        "P1: Mid Point (luminance) property failed to set expected value")
    luminance_level_max = (
        "Max Point (luminance) property set",
        "P1: Max Point (luminance) property failed to set expected value")
    surround_gamma = (
        "Surround Gamma property set",
        "P1: Surround Gamma property failed to set expected value")
    gamma = (
        "Gamma property set",
        "P1: Gamma property failed to set expected value")
    display_mapper_type = (
        "Display Mapper Operation Type is correctly set",
        "P1: Display Mapper Operation type is not as expected")


# IsClose tolerance defaults to , 1e-9, but float values in some tests need a looser tolerance
TOLERANCE = 1e-6


def AtomEditorComponentsLevel_DisplayMapper_AddedToEntity():
    """
    Summary:
    Tests the Display Mapper level component can be added to the level entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Add Display Mapper level component to the level entity.
    2) UNDO the level component addition.
    3) REDO the level component addition.
    4) Set LDR color Grading LUT asset.
    5) Set the Display Mapper Operation Type enumerated in DISPLAY_MAPPER_OPERATION_TYPE for each type
    6) Set Enable LDR color grading LUT property True
    7) Toggle Override Defaults
    8) Toggle Alter Surround
    9) Toggle Alter Desaturation
    10) Toggle 'Alter CAT D60 to D65'
    11) Set 'Cinema Limit (black)' max value which is dynamic based on Cinema Limit (white) then min value
    12) Set 'Cinema Limit (white)' max value then min, and finally back to default
    13) Set 'Min Point (luminance)' to high and low value
    14) Set 'Mid Point (luminance)' to high and low value then set default
    15) Set 'Max Point (luminance)' to high and low value then set default
    16) Set 'Surround Gamma' to high and low value
    17) Set 'Gamma' to high and low value
    18) Enter/Exit game mode.
    19) Select and load each preset
    20) Look for errors and asserts.

    :return: None
    """
    import os

    import azlmbr.bus as bus
    import azlmbr.legacy.general as general
    import azlmbr.render as render

    from azlmbr.math import Math_IsClose
    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorLevelEntity
    from editor_python_test_tools.wait_utils import PrefabWaiter
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties, DISPLAY_MAPPER_PRESET, DISPLAY_MAPPER_OPERATION_TYPE

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Add Display Mapper level component to the level entity.
        display_mapper_component = EditorLevelEntity.add_component(AtomComponentProperties.display_mapper())
        PrefabWaiter.wait_for_propagation()
        Report.critical_result(
            Tests.display_mapper_component,
            EditorLevelEntity.has_component(AtomComponentProperties.display_mapper()))

        # 2. UNDO the level component addition.
        # -> UNDO component addition.
        general.undo()
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.creation_undo, not EditorLevelEntity.has_component(AtomComponentProperties.display_mapper()))

        # 3. REDO the level component addition.
        # -> REDO component addition.
        general.redo()
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.creation_redo, EditorLevelEntity.has_component(AtomComponentProperties.display_mapper()))

        # 4. Set LDR color Grading LUT asset.
        display_mapper_asset_path = os.path.join("lookuptables", "lut_sepia.azasset")
        display_mapper_asset = Asset.find_asset_by_path(display_mapper_asset_path, False)
        display_mapper_component.set_component_property_value(
            AtomComponentProperties.display_mapper('LDR color Grading LUT'), display_mapper_asset.id)
        Report.result(
            Tests.ldr_color_grading_lut,
            display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('LDR color Grading LUT')) == display_mapper_asset.id)

        for operation_type in DISPLAY_MAPPER_OPERATION_TYPE.keys():
            # 5. Set the Display Mapper Operation Type enumerated in DISPLAY_MAPPER_OPERATION_TYPE for each type
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Type'), DISPLAY_MAPPER_OPERATION_TYPE[operation_type])
            general.idle_wait_frames(3)
            set_type = render.DisplayMapperComponentRequestBus(bus.Broadcast, "GetDisplayMapperOperationType")
            Report.info(f"DiplayMapperOperationType: {set_type}")
            Report.result(Tests.display_mapper_type, set_type == DISPLAY_MAPPER_OPERATION_TYPE[operation_type])

            # 6. Set Enable LDR color grading LUT property True
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Enable LDR color grading LUT'), True)
            Report.result(
                Tests.enable_ldr_color_grading_lut,
                display_mapper_component.get_component_property_value(
                    AtomComponentProperties.display_mapper('Enable LDR color grading LUT')) is True)

            # 7. Toggle Override Defaults
            # Set Override Defaults to False
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Override Defaults'), False)
            Report.result(
                Tests.override_defaults,
                display_mapper_component.get_component_property_value(
                    AtomComponentProperties.display_mapper('Override Defaults')) is False)

            # Set Override Defaults to True
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Override Defaults'), True)
            Report.result(
                Tests.override_defaults,
                display_mapper_component.get_component_property_value(
                    AtomComponentProperties.display_mapper('Override Defaults')) is True)

            # 8. Toggle Alter Surround
            # Set Alter Surround to True
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Alter Surround'), True)
            Report.result(
                Tests.alter_surround,
                display_mapper_component.get_component_property_value(
                    AtomComponentProperties.display_mapper('Alter Surround')) is True)

            # Set Alter Surround to False
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Alter Surround'), False)
            Report.result(
                Tests.alter_surround,
                display_mapper_component.get_component_property_value(
                    AtomComponentProperties.display_mapper('Alter Surround')) is False)

            # 9. Toggle Alter Desaturation
            # Set Alter Desaturation to True
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Alter Desaturation'), True)
            Report.result(
                Tests.alter_desaturation,
                display_mapper_component.get_component_property_value(
                    AtomComponentProperties.display_mapper('Alter Desaturation')) is True)

            # Set Alter Desaturation to False
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Alter Desaturation'), False)
            Report.result(
                Tests.alter_desaturation,
                display_mapper_component.get_component_property_value(
                    AtomComponentProperties.display_mapper('Alter Desaturation')) is False)

            # 10. Toggle 'Alter CAT D60 to D65'
            # Set 'Alter CAT D60 to D65' to True
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Alter CAT D60 to D65'), True)
            Report.result(
                Tests.alter_cat,
                display_mapper_component.get_component_property_value(
                    AtomComponentProperties.display_mapper('Alter CAT D60 to D65')) is True)

            # Set 'Alter CAT D60 to D65' to False
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Alter CAT D60 to D65'), False)
            Report.result(
                Tests.alter_cat,
                display_mapper_component.get_component_property_value(
                    AtomComponentProperties.display_mapper('Alter CAT D60 to D65')) is False)

            general.idle_wait_frames(1)

            # 11. Set 'Cinema Limit (black)' max value which is dynamic based on Cinema Limit (white) then min value
            # set max value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Cinema Limit (black)'), value=48.0)
            Report.result(Tests.black_level_max, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Cinema Limit (black)')), 48.0, TOLERANCE))

            # set min value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Cinema Limit (black)'), value=0.02)
            Report.result(Tests.black_level_min, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Cinema Limit (black)')), 0.02, TOLERANCE))

            # 12. Set 'Cinema Limit (white)' maximum value then min, and finally back to default
            # set max value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Cinema Limit (white)'), value=4000.0)
            Report.result(Tests.white_level_max, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Cinema Limit (white)')), 4000.0, TOLERANCE))

            # Set min value which is dynamic based on Cinema Limit (black)
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Cinema Limit (white)'), value=0.02)
            Report.result(Tests.white_level_min, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Cinema Limit (white)')), 0.02, TOLERANCE))

            # Reset this to the default 48 so following cycles don't impact Cinema Limit (Black)
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Cinema Limit (white)'), value=48.0)

            # 13. Set 'Min Point (luminance)' to high and low value
            # set high value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Min Point (luminance)'), value=4.8)
            Report.info(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Min Point (luminance)')))
            Report.result(Tests.luminance_level_min, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Min Point (luminance)')), 4.8, TOLERANCE))

            # set low value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Min Point (luminance)'), value=0.002)
            Report.result(Tests.luminance_level_min, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Min Point (luminance)')), 0.002, TOLERANCE))

            # 14. Set 'Mid Point (luminance)' to high and low value then set default
            # set high value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Mid Point (luminance)'), value=1005.0)
            Report.result(Tests.luminance_level_mid, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Mid Point (luminance)')), 1005.0, TOLERANCE))

            # set low value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Mid Point (luminance)'), value=0.002)
            Report.result(Tests.luminance_level_mid, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Mid Point (luminance)')), 0.002, TOLERANCE))

            # restore the default since as this impacts the range of 'Min Point (luminance)'
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Mid Point (luminance)'), value=4.8)

            # 15. Set 'Max Point (luminance)' to high and low value then set default
            # set a high value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Max Point (luminance)'), value=4000.0)
            Report.result(Tests.luminance_level_max, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Max Point (luminance)')), 4000.0, TOLERANCE))

            # set a low value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Max Point (luminance)'), value=0.002)
            Report.result(Tests.luminance_level_max, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Max Point (luminance)')), 0.002, TOLERANCE))

            # restore the default since this impacts the range of 'Mid Point (luminance)'
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Max Point (luminance)'), value=1005.7191162)

            # 16. Set 'Surround Gamma' to high and low value
            # set a high value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Surround Gamma'), value=1.2)
            Report.result(Tests.surround_gamma, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Surround Gamma')), 1.2, TOLERANCE))

            # set a low value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Surround Gamma'), value=0.6)
            Report.result(Tests.surround_gamma, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Surround Gamma')), 0.6, TOLERANCE))

            # 17. Set 'Gamma' to high and low value
            # set a high value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Gamma'), value=4.0)
            Report.result(Tests.gamma, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Gamma')), 4.0, TOLERANCE))

            # set a low value
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Gamma'), value=0.2)
            Report.result(Tests.gamma, Math_IsClose(display_mapper_component.get_component_property_value(
                AtomComponentProperties.display_mapper('Gamma')), 0.2, TOLERANCE))

            # 18. Enter/Exit game mode.
            TestHelper.enter_game_mode(Tests.enter_game_mode)
            general.idle_wait_frames(1)
            TestHelper.exit_game_mode(Tests.exit_game_mode)

            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Enable LDR color grading LUT'), False)

        display_mapper_component.set_component_property_value(
            AtomComponentProperties.display_mapper('Enable LDR color grading LUT'), True)

        cinema_limit_white_presets = [48.0, 184.3200073, 368.6400146, 737.2800293]
        for preset in DISPLAY_MAPPER_PRESET.keys():
            # 19. Select and load each preset
            display_mapper_component.set_component_property_value(
                AtomComponentProperties.display_mapper('Preset Selection'), DISPLAY_MAPPER_PRESET[preset])
            general.idle_wait_frames(1)
            render.DisplayMapperComponentRequestBus(
                bus.Broadcast,
                "LoadPreset",
                display_mapper_component.get_component_property_value(
                    AtomComponentProperties.display_mapper('Preset Selection')))
            general.idle_wait_frames(1)
            # check some value to confirm preset loaded
            test_preset = (f"Preset {preset} loaded expected value",
                      f"P1: Preset {preset} failed to load values as expected")
            Report.result(test_preset, Math_IsClose(
                display_mapper_component.get_component_property_value(
                    AtomComponentProperties.display_mapper('Cinema Limit (white)')),
                cinema_limit_white_presets[DISPLAY_MAPPER_PRESET[preset]], TOLERANCE))

        # 20. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponentsLevel_DisplayMapper_AddedToEntity)
