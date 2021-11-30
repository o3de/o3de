"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    spawner_slice_created = (
        "Spawner slice created successfully",
        "Failed to create Spawner slice"
    )
    instance_count_unhidden = (
        "Initial instance counts are as expected",
        "Found an unexpected number of initial instances"
    )
    instance_count_hidden = (
        "Instance counts upon hiding the Spawner slice are as expected",
        "Unexpectedly found instances with the Spawner slice hidden"
    )
    blender_slice_created = (
        "Blender slice created successfully",
        "Failed to create Blender slice"
    )


def SpawnerSlices_SliceCreationAndVisibilityToggleWorks():
    """
    Summary:
    C2627900 Verifies if a slice containing the component can be created.
    C2627905 A slice containing the Vegetation Layer Blender component can be created.
    C2627904: Hiding a slice containing the component clears any visuals from the Viewport.

    Expected Result:
    C2627900, C2627905: Slice is created, and is properly processed in the Asset Processor.
    C2627904: Vegetation area visuals are hidden from the Viewport.

    :return: None
    """

    import os

    import azlmbr.math as math
    import azlmbr.legacy.general as general
    import azlmbr.slice as slice
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.asset as asset

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def path_is_valid_asset(asset_path):
        asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", asset_path, math.Uuid(), False)
        return asset_id.invoke("IsValid")

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")
    general.set_current_view_position(512.0, 480.0, 38.0)

    # 2) C2627900 Verifies if a slice containing the Vegetation Layer Spawner component can be created.
    # 2.1) Create basic vegetation entity
    position = math.Vector3(512.0, 512.0, 32.0)
    asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
    veg_1 = dynveg.create_vegetation_area("vegetation_1", position, 16.0, 16.0, 16.0, asset_path)

    # 2.2) Create slice from the entity
    slice_path = os.path.join("slices", "TestSlice_1.slice")
    slice.SliceRequestBus(bus.Broadcast, "CreateNewSlice", veg_1.id, slice_path)

    # 2.3) Verify if the slice has been created successfully
    spawner_slice_success = helper.wait_for_condition(lambda: path_is_valid_asset(slice_path), 5.0)
    Report.result(Tests.spawner_slice_created, spawner_slice_success)

    # 3) C2627904: Hiding a slice containing the component clears any visuals from the Viewport
    # 3.1) Create Surface for instances to plant on
    dynveg.create_surface_entity("Surface_Entity", position, 16.0, 16.0, 1.0)

    # 3.2) Initially verify instance count before hiding slice
    initial_count_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(position, 16.0, 400), 5.0)
    Report.result(Tests.instance_count_unhidden, initial_count_success)

    # 3.3) Hide the slice and verify instance count
    editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", veg_1.id, False)
    hidden_instance_count = helper.wait_for_condition(lambda: dynveg.validate_instance_count(position, 16.0, 0), 5.0)
    Report.result(Tests.instance_count_hidden, hidden_instance_count)

    # 3.4) Unhide the slice
    editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", veg_1.id, True)

    # 4) C2627905 A slice containing the Vegetation Layer Blender component can be created.
    # 4.1) Create another vegetation entity to add to blender component
    veg_2 = dynveg.create_vegetation_area("vegetation_2", position, 1.0, 1.0, 1.0, "")

    # 4.2) Create entity with Vegetation Layer Blender
    components_to_add = ["Box Shape", "Vegetation Layer Blender"]
    blender_entity = hydra.Entity("blender_entity")
    blender_entity.create_entity(position, components_to_add)

    # 4.3) Pin both the vegetation areas to the blender entity
    pte = hydra.get_property_tree(blender_entity.components[1])
    path = "Configuration|Vegetation Areas"
    pte.update_container_item(path, 0, veg_1.id)
    pte.add_container_item(path, 1, veg_2.id)

    # 4.4) Drag the simple vegetation areas under the Vegetation Layer Blender entity to create an entity hierarchy.
    veg_1.set_test_parent_entity(blender_entity)
    veg_2.set_test_parent_entity(blender_entity)

    # 4.5) Create slice from blender entity
    slice_path = os.path.join("slices", "TestSlice_2.slice")
    slice.SliceRequestBus(bus.Broadcast, "CreateNewSlice", blender_entity.id, slice_path)

    # 4.6) Verify if the slice has been created successfully
    blender_slice_success = helper.wait_for_condition(lambda: path_is_valid_asset(slice_path), 5.0)
    Report.result(Tests.blender_slice_created, blender_slice_success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(SpawnerSlices_SliceCreationAndVisibilityToggleWorks)
