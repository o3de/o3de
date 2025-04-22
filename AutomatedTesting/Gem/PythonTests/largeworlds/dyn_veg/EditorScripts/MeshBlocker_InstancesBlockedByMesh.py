"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    blocked_instance_count = (
        "Instance count is as expected wih a Blocker setup",
        "Found unexpected instances with a Blocker setup"
    )


def MeshBlocker_InstancesBlockedByMesh():
    """
    Summary:
    Level is created. An entity with a vegetation spawner and entity with vegetation blocker (Mesh) component are
    added. Finally, the instance counts are checked to verify expected numbers after blocker is applied.

    Expected Behavior:
    The vegetation planted in the Spawner area is blocked by the Mesh of the Vegetation Blocker Mesh component.

    Test Steps:
     --> Open a level
     --> Create Spawner Entity
     --> Create Surface Entity to spawn vegetation instances on
     --> Create Blocker Entity with cube mesh
     --> Verify spawned vegetation instance counts

    Note:
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.components as components
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.wait_utils import PrefabWaiter
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # Open an existing simple level
    hydra.open_base_level()

    general.set_current_view_position(500.49, 498.69, 46.66)
    general.set_current_view_rotation(-42.05, 0.00, -36.33)

    # Create entity with components "Vegetation Layer Spawner", "Vegetation Asset List", "Box Shape"
    entity_position = math.Vector3(512.0, 512.0, 32.0)
    pink_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
    pink_flower_prefab = dynveg.create_temp_mesh_prefab(pink_flower_asset_path, "MeshBlocker_PinkFlower")[0]
    spawner_entity = dynveg.create_temp_prefab_vegetation_area("Instance Spawner", entity_position, 10.0, 10.0, 10.0,
                                                               pink_flower_prefab)

    # Create surface entity to plant on
    dynveg.create_surface_entity("Surface Entity", entity_position, 10.0, 10.0, 1.0)

    # Create blocker entity with cube mesh
    mesh_type_id = azlmbr.globals.property.EditorMeshComponentTypeId
    blocker_entity = hydra.Entity("Blocker Entity")
    blocker_entity.create_entity(entity_position,
                                 ["Vegetation Layer Blocker (Mesh)"])
    blocker_entity.add_component_of_type(mesh_type_id)
    if blocker_entity.id.IsValid():
        print(f"'{blocker_entity.name}' created")
    cubeId = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", os.path.join("objects", "_primitives", "_box_1x1.fbx.azmodel"), math.Uuid(),
        False)

    blocker_entity.get_set_test(1, "Controller|Configuration|Mesh Asset", cubeId)
    PrefabWaiter.wait_for_propagation()
    components.TransformBus(bus.Event, "SetLocalUniformScale", blocker_entity.id, 2.0)

    # Verify spawned instance counts are accurate after addition of Blocker Entity
    num_expected = 160  # Number of "PurpleFlower"s that plant on a 10 x 10 surface minus 2m blocker cube
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                            num_expected), 2.0)
    Report.result(Tests.blocked_instance_count, result)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(MeshBlocker_InstancesBlockedByMesh)
