"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.
SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class MacroMaterialTests:
    setup_test = (
        "Setup successful", 
        "Setup failed"
    )
    material_changed_not_called_when_inactive = (
        "OnTerrainMacroMaterialRegionChanged not called successfully", 
        "OnTerrainMacroMaterialRegionChanged called when component inactive."
    )
    material_created = (
        "MaterialCreated called successfully", 
        "MaterialCreated failed"
    )
    material_destroyed = (
        "MaterialDestroyed called successfully", 
        "MaterialDestroyed failed"
    )
    material_recreated = (
        "MaterialCreated called successfully on second test", 
        "MaterialCreated failed on second test"
    )
    material_changed_call_on_aabb_change = (
        "OnTerrainMacroMaterialRegionChanged called successfully", 
        "Timed out waiting for OnTerrainMacroMaterialRegionChanged"
    )

def TerrainMacroMaterialComponent_MacroMaterialActivates():
    """
    Summary:
    Load an empty level, create a MacroMaterialComponent and check assigning textures results in the correct callbacks.
    :return: None
    """

    import os
    import math as sys_math

    import azlmbr.legacy.general as general
    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.math as math
    import azlmbr.terrain as terrain
    import azlmbr.editor as editor
    import azlmbr.vegetation as vegetation
    import azlmbr.entity as EntityId

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    import editor_python_test_tools.pyside_utils as pyside_utils
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.asset_utils import Asset

    material_created_called = False
    material_changed_called = False
    material_region_changed_called = False
    material_destroyed_called = False

    def create_entity_at(entity_name, components_to_add, x, y, z):
        entity = EditorEntity.create_editor_entity_at([x, y, z], entity_name)
        for component in components_to_add:
            entity.add_component(component)

        return entity
        
    def on_macro_material_created(args):
        nonlocal material_created_called
        material_created_called = True

    def on_macro_material_changed(args):
        nonlocal material_changed_called
        material_changed_called = True

    def on_macro_material_region_changed(args):
        nonlocal material_region_changed_called
        material_region_changed_called = True

    def on_macro_material_destroyed(args):
        nonlocal material_destroyed_called
        material_destroyed_called = True

    helper.init_idle()

    # Open a level.
    helper.open_level("Physics", "Base")
    helper.wait_for_condition(lambda: general.get_current_level_name() == "Base", 2.0)
    
    general.idle_wait_frames(1)

    # Set up a handler to wait for notifications from the TerrainSystem.
    handler = terrain.TerrainMacroMaterialAutomationBusHandler()
    handler.connect()
    handler.add_callback("OnTerrainMacroMaterialCreated", on_macro_material_created)
    handler.add_callback("OnTerrainMacroMaterialChanged", on_macro_material_changed)
    handler.add_callback("OnTerrainMacroMaterialRegionChanged", on_macro_material_region_changed)
    handler.add_callback("OnTerrainMacroMaterialDestroyed", on_macro_material_destroyed)

    macro_material_entity = create_entity_at("macro", ["Terrain Macro Material", "Axis Aligned Box Shape"], 0.0, 0.0, 0.0)

    # Check that no macro material callbacks happened. It should be "inactive" as it has no assets assigned.
    setup_success =  not material_created_called and not material_changed_called and not material_region_changed_called and not material_destroyed_called
    Report.result(MacroMaterialTests.setup_test, setup_success)

    # Find the aabb component.
    aabb_component_type_id_type = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Axis Aligned Box Shape"], 0)[0]
    aabb_component_id = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'GetComponentOfType', macro_material_entity.id, aabb_component_type_id_type).GetValue()
    
    # Change the aabb dimensions
    material_region_changed_called = False
    box_dimensions_path = "Axis Aligned Box Shape|Box Configuration|Dimensions"
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", aabb_component_id, box_dimensions_path, math.Vector3(1.0, 1.0, 1.0))

    # Check we don't receive a callback. The macro material component should be inactive as it has no images assigned.
    general.idle_wait_frames(1)
    Report.result(MacroMaterialTests.material_changed_not_called_when_inactive, material_region_changed_called == False)

     # Find the macro material component.
    macro_material_id_type = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Terrain Macro Material"], 0)[0]
    macro_material_component_id = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'GetComponentOfType', macro_material_entity.id, macro_material_id_type).GetValue()

    # Find a color image asset.
    color_image_path = os.path.join("assets", "textures", "image.png.streamingimage")
    color_image_asset = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", color_image_path, math.Uuid(), False)
    
    # Assign the image to the MacroMaterial component, which should result in a created message.
    material_created_called = False
    color_texture_path = "Configuration|Color Texture"
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", macro_material_component_id, color_texture_path, color_image_asset)

    call_result = helper.wait_for_condition(lambda: material_created_called == True, 2.0)
    Report.result(MacroMaterialTests.material_created, call_result)

    # Find a normal image asset.
    normal_image_path = os.path.join("assets", "textures", "normal.png.streamingimage")
    normal_image_asset = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", normal_image_path, math.Uuid(), False)

    # Assign the normal image to the MacroMaterial component, which should result in a created message.
    material_created_called = False
    material_destroyed_called = False
    normal_texture_path = "Configuration|Normal Texture"
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", macro_material_component_id, normal_texture_path, normal_image_asset)

    # Check the MacroMaterial was destroyed and recreated.
    destroyed_call_result = helper.wait_for_condition(lambda: material_destroyed_called == True, 2.0)
    Report.result(MacroMaterialTests.material_destroyed, destroyed_call_result)

    recreated_call_result = helper.wait_for_condition(lambda: material_created_called == True, 2.0)
    Report.result(MacroMaterialTests.material_recreated, recreated_call_result)

    # Change the aabb dimensions.
    box_dimensions_path = "Axis Aligned Box Shape|Box Configuration|Dimensions"
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", aabb_component_id, box_dimensions_path, math.Vector3(1.0, 1.0, 1.0))

    # Check that a callback is received.
    region_changed_call_result = helper.wait_for_condition(lambda: material_region_changed_called == True, 2.0)
    Report.result(MacroMaterialTests.material_changed_call_on_aabb_change, region_changed_call_result)

if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(TerrainMacroMaterialComponent_MacroMaterialActivates)