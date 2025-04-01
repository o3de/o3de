"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    spawner_prefab_created = (
        "Spawner prefab created successfully",
        "Failed to create Spawner prefab"
    )
    instance_count_unhidden = (
        "Initial instance counts are as expected",
        "Found an unexpected number of initial instances"
    )
    instance_count_hidden = (
        "Instance counts upon hiding the Spawner prefab are as expected",
        "Unexpectedly found instances with the Spawner prefab hidden"
    )
    blender_prefab_created = (
        "Blender prefab created successfully",
        "Failed to create Blender prefab"
    )


def SpawnerPrefabs_PrefabCreationAndVisibilityToggleWorks():
    """
    Summary:
    C2627900 Verifies if a prefab containing the component can be created.
    C2627905 A prefab containing the Vegetation Layer Blender component can be created.
    C2627904: Hiding a prefab containing the component clears any visuals from the Viewport.

    Expected Result:
    C2627900, C2627905: Prefab is created, and is properly processed in the Asset Processor.
    C2627904: Vegetation area visuals are hidden from the Viewport.

    :return: None
    """

    import os

    import azlmbr.math as math
    import azlmbr.legacy.general as general
    import azlmbr.bus as bus
    import azlmbr.editor as editor

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.prefab_utils import Prefab

    # 1) Open an existing simple level
    hydra.open_base_level()
    general.set_current_view_position(512.0, 480.0, 38.0)

    # 2) Verifies if a prefab containing the Vegetation Layer Spawner component can be created.
    # 2.1) Create basic vegetation entity
    position = math.Vector3(512.0, 512.0, 32.0)
    pink_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
    pink_flower_prefab = dynveg.create_temp_mesh_prefab(pink_flower_asset_path, "SpawnerPrefab_PinkFlower")[0]
    veg_1 = dynveg.create_temp_prefab_vegetation_area("vegetation_1", position, 16.0, 16.0, 16.0,
                                                      pink_flower_prefab)

    # 2.2) Create prefab from the entity
    spawner_prefab_filename = "TestPrefab_1"
    spawner_prefab, spawner_prefab_instance = Prefab.create_prefab([veg_1], spawner_prefab_filename)

    # Verify if prefab is created
    Report.result(Tests.spawner_prefab_created, spawner_prefab.is_prefab_loaded(spawner_prefab_filename))

    # 3) Hiding a prefab containing the component clears any visuals from the Viewport
    # 3.1) Create Surface for instances to plant on
    dynveg.create_surface_entity("Surface_Entity", position, 16.0, 16.0, 1.0)

    # 3.2) Initially verify instance count before hiding prefab
    initial_count_success = helper.wait_for_condition(lambda: dynveg.validate_instance_count(position, 16.0, 400), 5.0)
    Report.result(Tests.instance_count_unhidden, initial_count_success)

    # 3.3) Hide the prefab and verify instance count
    editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", spawner_prefab_instance.container_entity.id, False)
    hidden_instance_count = helper.wait_for_condition(lambda: dynveg.validate_instance_count(position, 16.0, 0), 5.0)
    Report.result(Tests.instance_count_hidden, hidden_instance_count)

    # 3.4) Unhide the prefab
    editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", spawner_prefab_instance.container_entity.id, True)

    # 4) A prefab containing the Vegetation Layer Blender component can be created.
    # 4.1) Create another vegetation entity to add to blender component
    veg_2 = dynveg.create_empty_vegetation_area("vegetation_2", position, 1.0, 1.0, 1.0)

    # 4.2) Create entity with Vegetation Layer Blender
    components_to_add = ["Box Shape", "Vegetation Layer Blender"]
    blender_entity = EditorEntity.create_editor_entity("blender_entity")
    blender_entity.add_components(components_to_add)

    # 4.3) Pin both the vegetation areas to the blender entity
    pte = blender_entity.components[1].get_property_tree()
    path = "Configuration|Vegetation Areas"
    pte.update_container_item(path, 0, veg_1.id)
    pte.add_container_item(path, 1, veg_2.id)

    # 4.4) Drag the simple vegetation areas under the Vegetation Layer Blender entity to create an entity hierarchy.
    veg_1.set_test_parent_entity(blender_entity)
    veg_2.set_parent_entity(blender_entity.id)

    # 4.5) Create prefab from blender entity
    blender_prefab_filename = "TestPrefab_2"
    blender_prefab, blender_prefab_instance = Prefab.create_prefab([veg_2], blender_prefab_filename)

    # 4.6) Verify if the prefab has been created successfully
    Report.result(Tests.blender_prefab_created, blender_prefab.is_prefab_loaded(blender_prefab_filename))


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(SpawnerPrefabs_PrefabCreationAndVisibilityToggleWorks)
