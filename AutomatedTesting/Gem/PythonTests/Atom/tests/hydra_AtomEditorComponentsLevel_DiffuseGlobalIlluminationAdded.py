"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    creation_undo = (
        "UNDO Level component addition success",
        "UNDO Level component addition failed")
    creation_redo = (
        "REDO Level component addition success",
        "REDO Level component addition failed")
    diffuse_global_illumination_component = (
        "Level has a Diffuse Global Illumination component",
        "Level failed to find Diffuse Global Illumination component")
    diffuse_global_illumination_quality = (
        "Quality Level set",
        "Quality Level could not be set")
    enter_game_mode = (
        "Entered game mode",
        "Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "Couldn't exit game mode")


def AtomEditorComponentsLevel_DiffuseGlobalIllumination_AddedToEntity():
    """
    Summary:
    Tests the Diffuse Global Illumination level component can be added to the level entity and is stable.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Add Diffuse Global Illumination level component to the level entity.
    2) UNDO the level component addition.
    3) REDO the level component addition.
    4) Set Quality Level property to Low
    5) Enter/Exit game mode.
    6) Look for errors and asserts.

    :return: None
    """

    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorLevelEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties, GLOBAL_ILLUMINATION_QUALITY

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Test steps begin.
        # 1. Add Diffuse Global Illumination level component to the level entity.
        diffuse_global_illumination_component = EditorLevelEntity.add_component(
            AtomComponentProperties.diffuse_global_illumination())
        Report.critical_result(
            Tests.diffuse_global_illumination_component,
            EditorLevelEntity.has_component(AtomComponentProperties.diffuse_global_illumination()))

        # 2. UNDO the level component addition.
        # -> UNDO component addition.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_undo,
                      not EditorLevelEntity.has_component(AtomComponentProperties.diffuse_global_illumination()))

        # 3. REDO the level component addition.
        # -> REDO component addition.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_redo,
                      EditorLevelEntity.has_component(AtomComponentProperties.diffuse_global_illumination()))

        # 4. Set Quality Level property to Low
        diffuse_global_illumination_component.set_component_property_value(
            AtomComponentProperties.diffuse_global_illumination('Quality Level', GLOBAL_ILLUMINATION_QUALITY['Low']))
        quality = diffuse_global_illumination_component.get_component_property_value(
            AtomComponentProperties.diffuse_global_illumination('Quality Level'))
        Report.result(diffuse_global_illumination_quality, quality == GLOBAL_ILLUMINATION_QUALITY['Low'])

        # 5. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 6. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponentsLevel_DiffuseGlobalIllumination_AddedToEntity)
