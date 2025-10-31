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
    material_creation = (
        "Material Entity successfully created",
        "P0: Material Entity failed to be created")
    material_component = (
        "Entity has a Material component",
        "P0: Entity failed to find Material component")
    material_disabled = (
        "Material component disabled",
        "P0: Material component was not disabled.")
    actor_component = (
        "Entity has an Actor component",
        "P0: Entity did not have an Actor component")
    actor_undo = (
        "Entity Actor component gone",
        "P0: Entity Actor component add failed to undo")
    mesh_component = (
        "Entity has a Mesh component",
        "P0: Entity did not have a Mesh component")
    material_enabled = (
        "Material component enabled",
        "P0: Material component was not enabled.")
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
    model_asset = (
        "Model Asset set",
        "P1: Model Asset not set")
    default_material = (
        "Default Material set to metal_gold.azmaterial",
        "P1: Default Material was not set as expected")
    model_material_count = (
        "Model Material count is 5 as expected",
        "P1: Model Material count did not reach 5 within timeout")
    model_material_label = (
        "Model Material zero index is labeled lambert0",
        "P1: Model Material index zero does not have the expected label")
    model_material_asset_path = (
        "Asset path of the Material Model index zero is as expected",
        "P1: Material Model index zero Asset path is not as expected")
    enable_lod_materials = (
        "Enable LOD Materials property set to True",
        "P1: Enable LOD Materials property was not set correctly ")
    lod_material_count = (
        "LOD Material count is 5 as expected",
        "P1: LOD Material count did not reach 5 within timeout")
    lod_material_label = (
        "LOD Material zero index is labeled lambert0",
        "P1: LOD Material index zero does not have the expected label")
    lod_material_asset_path = (
        "Asset path of LOD Material index zero first item is as expected",
        "P1: Asset path of LOD Material index zero first item is not as expected")
    lod_material_set = (
        "LOD material zero is set to metal_gold.azmaterial id",
        "P1: LOD material zero was not property set as expected")
    controler_materials_is_empty_container = (
        "Controller|Materials is a container property and is empty",
        "P1: Controller|Materials is not a container or is not empty as expected")
    clear_default_material = (
        "Default Material cleared with MaterialComponentRequestBus",
        "P1: Default Material FAILED to clear with MaterialComponentRequestBus. does not contain empty AssetId.")


def AtomEditorComponents_Material_AddedToEntity():
    """
    Summary:
    Tests the Material component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Material entity with no components.
    2) Add a Material component to Material entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Verify Material component not enabled.
    6) Add Actor component since it is required by the Material component.
    7) Verify Material component is enabled.
    8) Remove Actor component
    9) Verify Material component not enabled.
    10) Add Mesh component since it is required by the Material component.
    11) Verify Material component is enabled.
    12) Set a model asset to Mesh component
    13) Wait for Model Materials to indicate count 5 terminate early if container fails to reflect correct count
    14) Get the slot zero material from the container property Model Materials (materials defined in the fbx)
    15) Enable the use of LOD materials
    16) Wait for LOD Materials to indicate count 5 terminate early if container fails to reflect correct count
    17) Get the slot zero list from LOD Material
    18) Override the slot zero LOD Material asset using EditorMaterialComponentSlot method SetAssetId
    19) Clear the LOD Material override and confirm the active asset is the default value
    20) Set the Default Material asset to an override using set component property by path
    21) Clear the assignment of a default material using MaterialComponentRequestBus and confirm null asset
    22) Set the Default Material asset using MaterialComponentRequestBus
    23) Enter/Exit game mode.
    24) Test IsHidden.
    25) Test IsVisible.
    26) Delete Material entity.
    27) UNDO deletion.
    28) REDO deletion.
    29) Look for errors or asserts.

    :return: None
    """
    import os

    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.render as render

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
        # 1. Create a Material entity with no components.
        material_entity = EditorEntity.create_editor_entity(AtomComponentProperties.material())
        Report.critical_result(Tests.material_creation, material_entity.exists())

        # 2. Add a Material component to Material entity.
        material_component = material_entity.add_component(AtomComponentProperties.material())
        Report.critical_result(
            Tests.material_component,
            material_entity.has_component(AtomComponentProperties.material()))

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
        Report.result(Tests.creation_undo, not material_entity.exists())

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
        Report.result(Tests.creation_redo, material_entity.exists())

        # 5. Verify Material component not enabled.
        Report.result(Tests.material_disabled, not material_component.is_enabled())

        # 6. Add Actor component since it is required by the Material component.
        actor_component = material_entity.add_component(AtomComponentProperties.actor())
        Report.result(Tests.actor_component, material_entity.has_component(AtomComponentProperties.actor()))

        # 7. Verify Material component is enabled.
        Report.result(Tests.material_enabled, material_component.is_enabled())

        # 8. Remove actor component.
        actor_component.remove()
        general.idle_wait_frames(1)
        Report.result(Tests.actor_undo, not material_entity.has_component(AtomComponentProperties.actor()))

        # 9. Verify Material component not enabled.
        Report.result(Tests.material_disabled, not material_component.is_enabled())

        # 10. Add Mesh component since it is required by the Material component.
        mesh_component = material_entity.add_component(AtomComponentProperties.mesh())
        Report.result(Tests.mesh_component, material_entity.has_component(AtomComponentProperties.mesh()))

        # 11. Verify Material component is enabled.
        Report.result(Tests.material_enabled, material_component.is_enabled())

        # 12. Set a model asset to Mesh component
        # Set a simple model to ensure that the more complex model will load cleanly
        model_path = os.path.join('objects', 'cube.fbx.azmodel')
        model = Asset.find_asset_by_path(model_path)
        mesh_component.set_component_property_value(AtomComponentProperties.mesh('Model Asset'), model.id)
        general.idle_wait_frames(1)
        # Update model asset to a model with 5 LOD materials
        model_path = os.path.join('testdata', 'objects', 'modelhotreload', 'sphere_5lods.fbx.azmodel')
        model = Asset.find_asset_by_path(model_path)
        mesh_component.set_component_property_value(AtomComponentProperties.mesh('Model Asset'), model.id)
        general.idle_wait_frames(1)
        Report.result(
            Tests.model_asset,
            mesh_component.get_component_property_value(AtomComponentProperties.mesh('Model Asset')) == model.id)

        # 13. Wait for Model Materials to indicate count 5 terminate early if container fails to reflect correct count
        Report.critical_result(Tests.model_material_count, TestHelper.wait_for_condition(
            lambda: (material_component.get_container_count(
                AtomComponentProperties.material('Model Materials')) == 5), timeout_in_seconds=5))

        # 14. Get the slot zero material from the container property Model Materials (materials defined in the fbx)
        item = material_component.get_container_item(AtomComponentProperties.material('Model Materials'), key=0)
        # item is an EditorMaterialComponentSlot which defines a number of method interfaces found in
        # .\o3de\Gems\AtomLyIntegration\CommonFeatures\Code\Source\Material\EditorMaterialComponentSlot.cpp
        label = item.GetLabel()
        Report.result(Tests.model_material_label, label == 'lambert0')
        # Asset path for lambert0 is 'objects/sphere_5lods_lambert0_11781189446760285338.fbx.azmaterial'; numbers may vary
        default_asset = Asset(item.GetDefaultAssetId())
        default_asset_path = default_asset.get_path()
        Report.result(
            Tests.model_material_asset_path,
            default_asset_path.startswith('testdata/objects/modelhotreload/sphere_5lods_lambert0_'))

        # 15. Enable the use of LOD materials
        material_component.set_component_property_value(AtomComponentProperties.material('Enable LOD Materials'), True)
        Report.result(
            Tests.enable_lod_materials,
            material_component.get_component_property_value(
                AtomComponentProperties.material('Enable LOD Materials')) is True)

        # 16. Wait for LOD Materials to indicate count 5 terminate early if container fails to reflect correct count
        Report.critical_result(Tests.lod_material_count, TestHelper.wait_for_condition(
            lambda: (material_component.get_container_count(
                AtomComponentProperties.material('LOD Materials')) == 5), timeout_in_seconds=5))

        # 17. Get the slot zero list from LOD Material
        item = material_component.get_container_item(
            AtomComponentProperties.material('LOD Materials'), key=0)  # list of EditorMaterialComponentSlot
        active_asset = Asset(item[0].GetActiveAssetId())
        active_asset_path = active_asset.get_path()
        Report.result(
            Tests.lod_material_asset_path,
            active_asset_path.startswith('testdata/objects/modelhotreload/sphere_5lods_lambert0_'))

        # Setup a material for overrides in further testing
        material_path = os.path.join('materials', 'presets', 'pbr', 'metal_gold.azmaterial')
        metal_gold = Asset.find_asset_by_path(material_path)

        # 18. Override the slot zero LOD Material asset using EditorMaterialComponentSlot method SetAssetId
        item[0].SetAssetId(metal_gold.id)
        # check override with EditorMaterialComponentSlot method GetActiveAssetId
        Report.result(Tests.lod_material_set, item[0].GetActiveAssetId() == metal_gold.id)
        # FindMaterialAsignementId expects slot index and slot label (index for default material is -1, slot 0 is 0)
        assignment_id = render.MaterialComponentRequestBus(
            bus.Event, "FindMaterialAssignmentId", material_entity.id, 0, 'lambert0')
        # check override with ebus call
        Report.result(Tests.lod_material_set,
                      render.MaterialComponentRequestBus(
                          bus.Event, "GetMaterialAssetId", material_entity.id, assignment_id) == metal_gold.id)

        # 19. Clear the LOD Material override and confirm the active asset is the default value
        item[0].Clear()
        general.idle_wait_frames(1)
        active_asset = Asset(item[0].GetActiveAssetId())
        active_asset_path = active_asset.get_path()
        Report.result(
            Tests.lod_material_asset_path,
            active_asset_path.startswith('testdata/objects/modelhotreload/sphere_5lods_lambert0_'))

        # 20. Set the Default Material asset to an override using set component property by path
        material_component.set_component_property_value(
            AtomComponentProperties.material('Material Asset'), metal_gold.id)
        Report.result(
            Tests.default_material,
            material_component.get_component_property_value(
                AtomComponentProperties.material('Material Asset')) == metal_gold.id)

        # 21. Clear the assignment of a default material using MaterialComponentRequestBus and confirm null asset
        render.MaterialComponentRequestBus(bus.Event, "ClearMaterialAssetIdOnDefaultSlot", material_entity.id)
        default_material_asset_id = render.MaterialComponentRequestBus(
            bus.Event, "GetMaterialAssetIdOnDefaultSlot", material_entity.id)
        Report.result(Tests.clear_default_material, default_material_asset_id == azlmbr.asset.AssetId())

        # 22. Set the Default Material asset using MaterialComponentRequestBus
        render.MaterialComponentRequestBus(
            bus.Event, "SetMaterialAssetIdOnDefaultSlot", material_entity.id, metal_gold.id)
        default_material_asset_id = render.MaterialComponentRequestBus(
            bus.Event, "GetMaterialAssetIdOnDefaultSlot", material_entity.id)
        Report.result(Tests.default_material, default_material_asset_id == metal_gold.id)

        # 23. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 24. Test IsHidden.
        material_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, material_entity.is_hidden() is True)

        # 25. Test IsVisible.
        material_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, material_entity.is_visible() is True)

        # 26. Delete Material entity.
        material_entity.delete()
        Report.result(Tests.entity_deleted, not material_entity.exists())

        # 27. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, material_entity.exists())

        # 28. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not material_entity.exists())

        # 29. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Material_AddedToEntity)
