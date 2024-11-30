"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def LayerSpawner_InstancesPlantInAllSupportedShapes():
    """
    Summary:
    The level is loaded and vegetation area is created. Then the Shape Reference
    component of vegetation area is pinned with entities of different shape components to check
    if the vegetation plants in different shaped areas.

    Expected Behavior:
    Vegetation properly plants in areas of any shape.

    Test Steps:
     1) Open a level
     2) Create basic vegetation area entity and set the properties
     3) Box Shape Entity: create, set properties and pin to vegetation
     4) Capsule Shape Entity: create, set properties and pin to vegetation
     5) Tube Shape Entity: create, set properties and pin to vegetation
     6) Sphere Shape Entity: create, set properties and pin to vegetation
     7) Cylinder Shape Entity: create, set properties and pin to vegetation
     8) Prism Shape Entity: create, set properties and pin to vegetation
     9) Compound Shape Entity: create, set properties and pin to vegetation

    Note:
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.legacy.general as general
    import azlmbr.entity as EntityId
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def pin_shape_and_check_count(entity, count):
        hydra.get_set_test(vegetation, 2, "Configuration|Shape Entity Id", entity.id)
        result = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(vegetation.id,
                                                                                                  count), 2.0)
        success = (
            f"Found the expected number of instances in {entity.name} shape",
            f"Unexpected number of instances found in {entity.name} shape"
        )
        Report.result(success, result)

    # 1) Open an existing simple level
    hydra.open_base_level()

    # 2) Create basic vegetation area entity and set the properties
    entity_position = math.Vector3(125.0, 136.0, 32.0)
    pink_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
    pink_flower_prefab = dynveg.create_temp_mesh_prefab(pink_flower_asset_path, "SpawnerShapePlant_PinkFlower")[0]
    vegetation = dynveg.create_temp_prefab_vegetation_area("Instance Spawner", entity_position, 10.0, 10.0, 10.0,
                                                           pink_flower_prefab)
    vegetation.remove_component("Box Shape")
    vegetation.add_component("Shape Reference")

    # Create surface for planting on
    dynveg.create_surface_entity("Surface Entity", entity_position, 60.0, 60.0, 1.0)

    # Adjust camera to be close to the vegetation entity
    general.set_current_view_position(135.0, 102.0, 39.0)
    general.set_current_view_rotation(-15.0, 0, 0)

    # 3) Box Shape Entity: create, set properties and pin to vegetation
    box = hydra.Entity("Box")
    box.create_entity(math.Vector3(124.0, 126.0, 32.0), ["Box Shape"])
    new_box_dimension = math.Vector3(10.0, 10.0, 1.0)
    hydra.get_set_test(box, 0, "Box Shape|Box Configuration|Dimensions", new_box_dimension)
    # This and subsequent counts are the number of "PurpleFlower" that spawn in the shape with given dimensions
    pin_shape_and_check_count(box, 156)

    # 4) Capsule Shape Entity: create, set properties and pin to vegetation
    capsule = hydra.Entity("Capsule")
    capsule.create_entity(math.Vector3(120.0, 142.0, 32.0), ["Capsule Shape"])
    hydra.get_set_test(capsule, 0, "Capsule Shape|Capsule Configuration|Height", 10.0)
    hydra.get_set_test(capsule, 0, "Capsule Shape|Capsule Configuration|Radius", 2.0)
    pin_shape_and_check_count(capsule, 20)

    # 5) Tube Shape Entity: create, set properties and pin to vegetation
    tube = hydra.Entity("Tube")
    tube.create_entity(math.Vector3(124.0, 136.0, 32.0), ["Tube Shape", "Spline"])
    pin_shape_and_check_count(tube, 27)

    # 6) Sphere Shape Entity: create, set properties and pin to vegetation
    sphere = hydra.Entity("Sphere")
    sphere.create_entity(math.Vector3(112.0, 143.0, 32.0), ["Sphere Shape"])
    hydra.get_set_test(sphere, 0, "Sphere Shape|Sphere Configuration|Radius", 5.0)
    pin_shape_and_check_count(sphere, 122)

    # 7) Cylinder Shape Entity: create, set properties and pin to vegetation
    cylinder = hydra.Entity("Cylinder")
    cylinder.create_entity(math.Vector3(136.0, 143.0, 32.0), ["Cylinder Shape"])
    hydra.get_set_test(cylinder, 0, "Cylinder Shape|Cylinder Configuration|Radius", 5.0)
    hydra.get_set_test(cylinder, 0, "Cylinder Shape|Cylinder Configuration|Height", 5.0)
    pin_shape_and_check_count(cylinder, 124)

    # 8) Prism Shape Entity: create, set properties and pin to vegetation
    polygon_prism = hydra.Entity("Polygon Prism")
    polygon_prism.create_entity(math.Vector3(127.0, 142.0, 32.0), ["Polygon Prism Shape"])
    pin_shape_and_check_count(polygon_prism, 20)

    # 9) Compound Shape Entity: create, set properties and pin to vegetation
    compound = hydra.Entity("Compound")
    compound.create_entity(math.Vector3(125.0, 136.0, 32.0), ["Compound Shape"])
    pte = hydra.get_property_tree(compound.components[0])
    shapes = [box.id, capsule.id, tube.id, sphere.id, cylinder.id, polygon_prism.id]
    for index in range(6):
        pte.add_container_item("Configuration|Child Shape Entities", index, EntityId.EntityId())
    for index, element in enumerate(shapes):
        hydra.get_set_test(compound, 0, f"Configuration|Child Shape Entities|[{index}]", element)
    pin_shape_and_check_count(compound, 469)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(LayerSpawner_InstancesPlantInAllSupportedShapes)
