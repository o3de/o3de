#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import sys

import azlmbr.asset as asset
import azlmbr.bus as bus
import azlmbr.components as components
import azlmbr.math as math
import azlmbr.vegetation as vegetation
import azlmbr.areasystem as areasystem
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.projectroot, 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra


def create_surface_entity(name, center_point, box_size_x, box_size_y, box_size_z):
    # Create a "flat surface" entity to use as a plantable vegetation surface
    surface_entity = hydra.Entity(name)
    surface_entity.create_entity(
        center_point,
        ["Box Shape", "Shape Surface Tag Emitter"]
        )
    if surface_entity.id.IsValid():
        print(f"'{surface_entity.name}' created")
    surface_entity.get_set_test(0, "Box Shape|Box Configuration|Dimensions", math.Vector3(box_size_x, box_size_y,
                                                                                          box_size_z))
    return surface_entity


def create_mesh_surface_entity_with_slopes(name, center_point, uniform_scale):
    # Creates an entity with the assigned mesh_asset as the specified scale and sets up as a planting surface
    mesh_asset_path = os.path.join("models", "sphere.azmodel")
    mesh_asset = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", mesh_asset_path, math.Uuid(),
                                              False)
    surface_entity = hydra.Entity(name)
    surface_entity.create_entity(
        center_point,
        ["Mesh", "Mesh Surface Tag Emitter"]
    )
    if surface_entity.id.IsValid():
        print(f"'{surface_entity.name}' created")
    hydra.get_set_test(surface_entity, 0, "Controller|Configuration|Mesh Asset", mesh_asset)
    components.TransformBus(bus.Event, "SetLocalUniformScale", surface_entity.id, uniform_scale)
    return surface_entity


def create_vegetation_area(name, center_point, box_size_x, box_size_y, box_size_z, dynamic_slice_asset_path):
    # Create a vegetation area entity to use as our test vegetation spawner
    spawner_entity = hydra.Entity(name)
    spawner_entity.create_entity(
        center_point,
        ["Vegetation Layer Spawner", "Box Shape", "Vegetation Asset List"]
        )
    if spawner_entity.id.IsValid():
        print(f"'{spawner_entity.name}' created")
    spawner_entity.get_set_test(1, "Box Shape|Box Configuration|Dimensions", math.Vector3(box_size_x, box_size_y,
                                                                                          box_size_z))

    # Set the vegetation area to a Dynamic Slice spawner with a specific slice asset selected
    dynamic_slice_spawner = vegetation.DynamicSliceInstanceSpawner()
    dynamic_slice_spawner.SetSliceAssetPath(dynamic_slice_asset_path)
    descriptor = hydra.get_component_property_value(spawner_entity.components[2], 'Configuration|Embedded Assets|[0]')
    descriptor.spawner = dynamic_slice_spawner
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets|[0]", descriptor)
    return spawner_entity


def create_blocker_area(name, center_point, box_size_x, box_size_y, box_size_z):
    # Create a Vegetation Layer Blocker area
    blocker_entity = hydra.Entity(name)
    blocker_entity.create_entity(
        center_point,
        ["Vegetation Layer Blocker", "Box Shape"]
    )
    if blocker_entity.id.IsValid():
        print(f"'{blocker_entity.name}' created")
    blocker_entity.get_set_test(1, "Box Shape|Box Configuration|Dimensions", math.Vector3(box_size_x, box_size_y,
                                                                                          box_size_z))
    return blocker_entity


def validate_instance_count(center, radius, num_expected):
    # Verify that we have the correct number of instances in the given area. This creates a bounding box based on a
    # sphere radius
    box = math.Aabb_CreateCenterRadius(center, radius)
    num_found = areasystem.AreaSystemRequestBus(bus.Broadcast, 'GetInstanceCountInAabb', box)
    result = (num_found == num_expected)
    print(f'instance count validation: {result} (found={num_found}, expected={num_expected})')
    return result


def validate_instance_count_in_entity_shape(entity_id, num_expected):
    # Verify that we have the correct number of instances in the given area. This requires a shape component present
    # on the specified entity
    num_found = vegetation.VegetationSpawnerRequestBus(bus.Event, "GetAreaProductCount", entity_id)
    result = (num_expected == num_found)
    print(f'instance count validation: {result} (found={num_found}, expected={num_expected})')
    return result
