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


def MeshBlocker_InstancesBlockedByMeshHeightTuning():
    """
    Summary:
    A temporary level is created, then a simple vegetation area is created. A blocker area is created and it is
    verified that the tuning of the height percent blocker setting works as expected.

    Expected Behavior:
    Vegetation is blocked only around the trunk of the tree, while it still plants under the areas covered by branches.

    Test Steps:
     1) Open a level
     2) Create entity with components "Vegetation Layer Spawner", "Vegetation Asset List", "Box Shape"
     3) Create surface entity
     4) Create blocker entity with sphere mesh
     5) Adjust the height Min/Max percentage values of blocker
     6) Verify spawned instance counts are accurate after adjusting height Max percentage of Blocker Entity

    Note:
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    import math as pymath

    import azlmbr
    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.components as components
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    general.set_current_view_position(500.49, 498.69, 46.66)
    general.set_current_view_rotation(-42.05, 0.00, -36.33)

    # 2) Create entity with components "Vegetation Layer Spawner", "Vegetation Asset List", "Box Shape"
    entity_position = math.Vector3(512.0, 512.0, 32.0)
    asset_path = os.path.join("Slices", "PurpleFlower.dynamicslice")
    spawner_entity = dynveg.create_vegetation_area("Instance Spawner",
                                                   entity_position,
                                                   10.0, 10.0, 10.0,
                                                   asset_path)

    # 3) Create surface entity to plant on
    dynveg.create_surface_entity("Surface Entity", entity_position, 10.0, 10.0, 1.0)

    # 4) Create blocker entity with rotated cube mesh
    y_rotation = pymath.radians(45.0)
    mesh_type_id = azlmbr.globals.property.EditorMeshComponentTypeId
    blocker_entity = hydra.Entity("Blocker Entity")
    blocker_entity.create_entity(entity_position,
                                 ["Vegetation Layer Blocker (Mesh)"])
    blocker_entity.add_component_of_type(mesh_type_id)
    if blocker_entity.id.IsValid():
        Report.info(f"'{blocker_entity.name}' created")
    sphere_id = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", os.path.join("objects", "_primitives", "_box_1x1.azmodel"), math.Uuid(),
        False)
    blocker_entity.get_set_test(1, "Controller|Configuration|Mesh Asset", sphere_id)
    components.TransformBus(bus.Event, "SetLocalUniformScale", blocker_entity.id, 5.0)
    components.TransformBus(bus.Event, "SetLocalRotation", blocker_entity.id, math.Vector3(0.0, y_rotation, 0.0))

    # 5) Adjust the height Max percentage values of blocker
    blocker_entity.get_set_test(0, "Configuration|Mesh Height Percent Max", 0.8)

    # 6) Verify spawned instance counts are accurate after adjusting height Max percentage of Blocker Entity
    # The number of "PurpleFlower" instances that plant on a 10 x 10 surface minus those blocked by the rotated at
    # 80% max height factored in.
    num_expected = 127
    result = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                            num_expected), 5.0)
    Report.result(Tests.blocked_instance_count, result)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(MeshBlocker_InstancesBlockedByMeshHeightTuning)
