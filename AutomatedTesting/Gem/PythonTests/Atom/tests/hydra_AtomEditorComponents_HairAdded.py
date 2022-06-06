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
    9) Enter/Exit game mode.
    10) Test IsHidden.
    11) Test IsVisible.
    12) Delete Hair entity.
    13) UNDO deletion.
    14) REDO deletion.
    15) Look for errors or asserts.

    :return: None
    """

    import azlmbr.legacy.general as general

    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

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
        head_path = os.path.join('testdata', 'headbonechainhairstyle', 'test_hair_bone_chain_head_style.azmodel')
        head_asset = Asset.find_asset_by_path(head_path)
        hair_path = os.path.join('testdata', 'headbonechainhairstyle', 'test_hair_bone_chain_head_style.tfxhair')
        hair_asset = Asset.find_asset_by_path(hair_path)
        actor_component.set_component_property_value(AtomComponentProperties.actor('Actor asset'), head_asset.id)
        hair_component.set_component_property_value(AtomComponentProperties.hair('Hair Asset'), hair_asset.id)
        Report.result(
            Tests.hair_asset,
            hair_component.get_component_property_value(AtomComponentProperties.hair('Hair Asset')) == hair_asset.id)

        # 9. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 10. Test IsHidden.
        hair_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, hair_entity.is_hidden() is True)

        # 11. Test IsVisible.
        hair_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, hair_entity.is_visible() is True)

        # 12. Delete Hair entity.
        hair_entity.delete()
        Report.result(Tests.entity_deleted, not hair_entity.exists())

        # 13. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, hair_entity.exists())

        # 14. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not hair_entity.exists())

        # 15. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Hair_AddedToEntity)
