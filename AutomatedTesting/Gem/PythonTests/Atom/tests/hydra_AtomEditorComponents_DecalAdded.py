"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    creation_undo = (
        "UNDO Entity creation success",
        "P0: UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "P0: REDO Entity creation failed")
    decal_creation = (
        "Decal Entity successfully created",
        "P0: Decal Entity failed to be created")
    decal_component = (
        "Entity has a Decal component",
        "P0: Entity failed to find Decal component")
    decal_component_removed = (
        "Decal component removed",
        "P0: Decal component failed to be removed")
    material_property_set = (
        "Material property set on Decal component",
        "P0: Couldn't set Material property on Decal component")
    attenuation_property_set = (
        "Attenuation Angle property set on Decal component",
        "P1: Couldn't set Attenuation Angle property on Decal component")
    opacity_property_set = (
        "Opacity property set on Decal component",
        "P1: Couldn't set Opacity property on Decal component")
    normal_map_opacity_property_set = (
        "Normal Map Opacity property set on Decal component",
        "P1: Couldn't set Normal Map Opacity property on Decal component")
    sort_key_property_set = (
        "Sort Key property set on Decal component",
        "P1: Couldn't set Sort Key property on Decal component")
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


def AtomEditorComponents_Decal_AddedToEntity():
    """
    Summary:
    Tests the Decal component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Decal entity with no components.
    2) Add Decal component to Decal entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Set Material property on Decal component.
    6) Set Attenuation Angle property on Decal component.
    7) Set Opacity property on Decal component.
    8) Set Normal Map Opacity property on Decal component.
    9) Set Sort Key property on Decal Component.
    10) Remove Decal component then UNDO the remove.
    11) Enter/Exit game mode.
    12) Test IsHidden.
    13) Test IsVisible.
    14) Delete Decal entity.
    15) UNDO deletion.
    16) REDO deletion.
    17) Look for errors and asserts.

    :return: None
    """
    import os

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
        # 1. Create a Decal entity with no components.
        decal_entity = EditorEntity.create_editor_entity(AtomComponentProperties.decal())
        Report.critical_result(Tests.decal_creation, decal_entity.exists())

        # 2. Add Decal component to Decal entity.
        decal_component = decal_entity.add_component(AtomComponentProperties.decal())
        Report.critical_result(Tests.decal_component, decal_entity.has_component(AtomComponentProperties.decal()))

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
        Report.result(Tests.creation_undo, not decal_entity.exists())

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
        Report.result(Tests.creation_redo, decal_entity.exists())

        # 5. Set Material property on Decal component.
        decal_material_asset_path = os.path.join("materials", "decal", "airship_symbol_decal.azmaterial")
        decal_material_asset = Asset.find_asset_by_path(decal_material_asset_path, False)
        decal_component.set_component_property_value(AtomComponentProperties.decal('Material'), decal_material_asset.id)
        get_material_property = decal_component.get_component_property_value(AtomComponentProperties.decal('Material'))
        Report.result(Tests.material_property_set, get_material_property == decal_material_asset.id)

        # 6. Set Attenuation Angle property on Decal component
        decal_component.set_component_property_value(AtomComponentProperties.decal('Attenuation Angle'), value=0.75)
        get_attenuation_property = decal_component.get_component_property_value(
            AtomComponentProperties.decal('Attenuation Angle'))
        Report.result(Tests.attenuation_property_set, get_attenuation_property == 0.75)

        # 7. Set Opacity property on Decal component
        decal_component.set_component_property_value(AtomComponentProperties.decal('Opacity'), value=0.5)
        get_opacity_property = decal_component.get_component_property_value(AtomComponentProperties.decal('Opacity'))
        Report.result(Tests.opacity_property_set, get_opacity_property == 0.5)

        # 8. Set Normal Map Opacity property on Decal component
        decal_component.set_component_property_value(AtomComponentProperties.decal('Normal Map Opacity'), value=0.5)
        get_normal_map_opacity_property = decal_component.get_component_property_value(AtomComponentProperties.decal('Normal Map Opacity'))
        Report.result(Tests.normal_map_opacity_property_set, get_normal_map_opacity_property == 0.5)

        # 9. Set Sort Key property on Decal component
        decal_component.set_component_property_value(AtomComponentProperties.decal('Sort Key'), value=255.0)
        get_sort_key_property = decal_component.get_component_property_value(AtomComponentProperties.decal('Sort Key'))
        Report.result(Tests.sort_key_property_set, get_sort_key_property == 255.0)
        decal_component.set_component_property_value(AtomComponentProperties.decal('Sort Key'), value=0)
        get_sort_key_property = decal_component.get_component_property_value(AtomComponentProperties.decal('Sort Key'))
        Report.result(Tests.sort_key_property_set, get_sort_key_property == 0)

        # 10. Remove Decal component then UNDO the remove
        decal_component.remove()
        general.idle_wait_frames(1)
        Report.result(Tests.decal_component_removed, not decal_entity.has_component(AtomComponentProperties.decal()))
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.decal_component, decal_entity.has_component(AtomComponentProperties.decal()))

        # 11. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 12. Test IsHidden.
        decal_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, decal_entity.is_hidden() is True)

        # 13. Test IsVisible.
        decal_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, decal_entity.is_visible() is True)

        # 14. Delete Decal entity.
        decal_entity.delete()
        Report.result(Tests.entity_deleted, not decal_entity.exists())

        # 15. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, decal_entity.exists())

        # 16. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not decal_entity.exists())

        # 17. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Decal_AddedToEntity)
