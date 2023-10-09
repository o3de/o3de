"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    dpe_enabled = (
        "DPE is enabled",
        "DPE is not enabled"
    )


def DPE_AllComponentPropertyTypesEditable():
    """
    With the DPE Enabled, validates that every component property type available in the AutomatedTesting project can be
    successfully edited.
    """

    import os

    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.utils import TestHelper, Report, Tracer
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.wait_utils import PrefabWaiter

    def edit_component_property_value(component, property_name, value_to_set):
        Report.info(f"Starting edits on {component.get_component_name()}'s {property_name} property.")
        original_value = component.get_component_property_value(property_name)
        Report.info(f"Original value for {property_name}: {original_value}")
        # Set new property value
        set_value_results = (
            f"Successfully set new value for {property_name}",
            f"Failed to set new value for {property_name}"
        )
        component.set_component_property_value(property_name, value_to_set)
        new_value = component.get_component_property_value(property_name)
        Report.result(set_value_results, original_value != new_value)
        # Undo the change
        undo_results = (
            f"Undo of {property_name} set successful",
            f"Undo failed on edit of {property_name}"
        )
        general.undo()
        PrefabWaiter.wait_for_propagation()
        undo_value = component.get_component_property_value(property_name)
        Report.result(undo_results, undo_value == original_value)
        # Redo the change
        redo_results = (
            f"Redo of {property_name} set successful",
            f"Redo failed on edit of {property_name}"
        )
        general.redo()
        PrefabWaiter.wait_for_propagation()
        redo_value = component.get_component_property_value(property_name)
        Report.result(redo_results, redo_value == new_value)

    with Tracer() as error_tracer:
        # Open the base level
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Verify the DPE is enabled
        Report.critical_result(Tests.dpe_enabled, general.get_cvar("ed_enableDPEInspector") == "true")

        # Create several new entities to hold various components
        dither_entity = EditorEntity.create_editor_entity("DitherComponentTestEntity")
        light_entity = EditorEntity.create_editor_entity("LightTestEntity")
        mesh_entity = EditorEntity.create_editor_entity("MeshTestEntity")
        debug_text_entity = EditorEntity.create_editor_entity("DebugTextTestEntity")
        pick_me_entity = EditorEntity.create_editor_entity("PickMeTestEntity")
        box_shape_entity = EditorEntity.create_editor_entity("BoxShapeEntity")
        container_entity = EditorEntity.create_editor_entity("ContainerComponentTestEntity")

        # Add the appropriate components to each test entity
        dither_component = dither_entity.add_component("Dither Gradient Modifier")
        light_component = light_entity.add_component("Light")
        mesh_component = mesh_entity.add_component("Mesh")
        debug_text_component = debug_text_entity.add_component("DebugDraw Text")
        gradient_component = pick_me_entity.add_component("Perlin Noise Gradient")
        box_shape_component = box_shape_entity.add_component("Box Shape")
        compound_shape_component = container_entity.add_component("Compound Shape")

        # Edit each component property type and validate

        # Entity Picker Property Type
        edit_component_property_value(dither_component, 'Configuration|Gradient|Gradient Entity Id', pick_me_entity.id)

        # Vector 3 (Float) Property Type
        edit_component_property_value(dither_component, 'Previewer|Preview Settings|Preview Size',
                                      math.Vector3(2.0, 2.0, 2.0))

        # Float Property Type
        edit_component_property_value(dither_component, 'Configuration|Gradient|Opacity', 0.5)

        # Dropdown/Combo Box Property Type
        edit_component_property_value(light_component, 'Controller|Configuration|Light type', 1)

        # Toggle Property Type
        edit_component_property_value(light_component, 'Controller|Configuration|Shadows|Enable shadow', True)

        # Slider (Float) Property Type
        edit_component_property_value(light_component, 'Controller|Configuration|Intensity', 250.0)

        # CSV (Int) Property Type
        edit_component_property_value(light_component, 'Controller|Configuration|Color', math.Color(0.5, 0.5, 0.5, 0.5))

        # Asset Picker
        asset_to_pick_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
        asset_to_pick = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", asset_to_pick_path,
                                                     math.Uuid(), False)
        edit_component_property_value(mesh_component, 'Controller|Configuration|Model Asset', asset_to_pick)

        # Temporarily skipping str property type due to https://github.com/o3de/o3de/issues/15147
        """
        # String Property Type
        edit_component_property_value(debug_text_component,  'Text element settings|Text', "Hello")
        """
        
        # Look for errors and asserts
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(DPE_AllComponentPropertyTypesEditable)
