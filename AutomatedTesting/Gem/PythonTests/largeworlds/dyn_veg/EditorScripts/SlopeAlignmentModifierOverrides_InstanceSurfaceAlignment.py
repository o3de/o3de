"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    surface_entity_created = (
        "Successfully created Surface entity",
        "Failed to create Surface entity"
    )
    instance_count = (
        "Found the expected number of instances",
        "Unexpected number of instances found"
    )
    instances_aligned_0 = (
        "All instances are pointed straight up",
        "Found instances not aligned to surface pointing straight up"
    )
    instances_aligned_1 = (
        "All instances are planted perpendicularly to the surface",
        "Found instances not aligned to surface perpendicularly"
    )


def SlopeAlignmentModifierOverrides_InstanceSurfaceAlignment():
    """
    Summary:
    Verifies instances properly align to surfaces based on configuration of descriptor overrides of the
    Vegetation Slope Alignment Modifier component.

    :return: None
    """

    import os
    from math import radians

    import azlmbr.areasystem as areasystem
    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.components as components
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def verify_proper_alignment(instance, rot_degrees_vec):
        expected_rotation = math.Quaternion()
        expected_rotation.SetFromEulerDegrees(rot_degrees_vec)
        if instance.alignment.IsClose(expected_rotation):
            return True
        Report.info(f"Expected rotation of {expected_rotation}, Found {instance.alignment}")
        return False

    # Open an existing simple level
    hydra.open_base_level()

    general.set_current_view_position(512.0, 480.0, 38.0)

    # Create a spawner entity setup with all needed components
    center_point = math.Vector3(512.0, 512.0, 32.0)
    pink_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.azmodel")
    pink_flower_prefab = dynveg.create_temp_mesh_prefab(pink_flower_asset_path, "SlopeAlign_PinkFlower2")[0]
    spawner_entity = dynveg.create_temp_prefab_vegetation_area("Instance Spawner", center_point, 16.0, 16.0, 32.0,
                                                               pink_flower_prefab)

    # Create a sloped mesh surface for the instances to plant on
    center_point = math.Vector3(502.0, 512.0, 24.0)
    mesh_asset_path = os.path.join("objects", "_primitives", "_box_1x1.azmodel")
    mesh_asset = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", mesh_asset_path, math.Uuid(),
                                              False)
    rotation = math.Vector3(0.0, radians(45.0), 0.0)
    surface_entity = hydra.Entity("Surface Entity")
    surface_entity.create_entity(
        center_point,
        ["Mesh", "Mesh Surface Tag Emitter"]
    )
    Report.critical_result(Tests.surface_entity_created, surface_entity.id.IsValid())
    hydra.get_set_test(surface_entity, 0, "Controller|Configuration|Model Asset", mesh_asset)
    components.TransformBus(bus.Event, "SetLocalRotation", surface_entity.id, rotation)
    components.TransformBus(bus.Event, "SetLocalUniformScale", surface_entity.id, 30.0)

    # Add a Vegetation Debugger component to allow refreshing instances
    hydra.add_level_component("Vegetation Debugger")

    # Add Vegetation Slope Alignment Modifier to the spawner entity and toggle on Allow Per-Item Overrides
    spawner_entity.add_component("Vegetation Slope Alignment Modifier")
    spawner_entity.get_set_test(3, "Configuration|Allow Per-Item Overrides", True)

    # Toggle on Surface Slope Alignment Override Enabled on the Vegetation Asset List component
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Surface Slope Alignment|Override Enabled",
                                True)

    # Set Surface Slope Alignment Override Min and Max to 0 and validate instance alignment
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Surface Slope Alignment|Max", 0.0)
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Surface Slope Alignment|Max", 0.0)

    # Verify instances are have planted and are aligned to slope as expected
    num_expected = 20 * 20
    instances_planted = helper.wait_for_condition(
        lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)
    Report.critical_result(Tests.instance_count, instances_planted)

    box = azlmbr.shape.ShapeComponentRequestsBus(bus.Event, 'GetEncompassingAabb', spawner_entity.id)
    instances = areasystem.AreaSystemRequestBus(bus.Broadcast, 'GetInstancesInAabb', box)

    success = True
    for instance in instances:
        success = verify_proper_alignment(instance, math.Vector3(0.0, 0.0, 0.0))
    Report.result(Tests.instances_aligned_0, success)

    # Set Surface Slope Alignment Min and Max to 1 and validate instance alignment
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Surface Slope Alignment|Min", 1.0)
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]|Surface Slope Alignment|Max", 1.0)
    general.run_console('veg_debugClearAllAreas')
    instances_planted = helper.wait_for_condition(
        lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)
    Report.critical_result(Tests.instance_count, instances_planted)

    box = azlmbr.shape.ShapeComponentRequestsBus(bus.Event, 'GetEncompassingAabb', spawner_entity.id)
    instances = areasystem.AreaSystemRequestBus(bus.Broadcast, 'GetInstancesInAabb', box)

    success = True
    for instance in instances:
        success = verify_proper_alignment(instance, math.Vector3(0.0, 45.0, 0.0))
    Report.result(Tests.instances_aligned_1, success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(SlopeAlignmentModifierOverrides_InstanceSurfaceAlignment)
