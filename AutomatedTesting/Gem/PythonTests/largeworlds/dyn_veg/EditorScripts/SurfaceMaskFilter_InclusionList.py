"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    default_inclusion_weight = (
        "Found the expected number of instances with Inclusion Weight set to defaults",
        "Found an unexpected number of instances with Inclusion Weight set to defaults"
    )
    inclusion_weight_below_one = (
        "Found the expected number of instances with Inclusion Weight set below 1",
        "Found an unexpected number of instances with Inclusion Weight set below 1"
    )



def SurfaceMaskFilter_InclusionList():
    """
    Summary:
    New level is created and set up with surface shapes with varying surface tags. A simple vegetation area has been
    created and Vegetation Surface Mask Filter component is added to entity with terrain hole inclusion tag.

    Expected Behavior:
    With default Inclusion Weights, vegetation draws over the terrain holes.
    With Inclusion Weight Max set below 1.0, vegetation stops drawing over the terrain holes.

    Test Steps:
     1) Open an existing level
     2) Create entity with components "Vegetation Layer Spawner", "Vegetation Asset List", "Box Shape"
     3) Add a Vegetation Surface Mask Filter component to the entity.
     4) Create 2 surface entities to represent terrain and terrain hole surfaces
     5) Add an Inclusion List tag to the component, and set it to "terrainHole".
     6) Check spawn count with default Inclusion Weights
     7) Check spawn count with Inclusion Weight Max set below 1.0

    Note:
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.surface_data as surface_data

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def update_surface_tag_inclusion_list(Entity, component_index, surface_tag):
        tag_list = [surface_data.SurfaceTag()]

        # assign list with one surface tag to inclusion list
        hydra.get_set_test(Entity, component_index, "Configuration|Inclusion|Surface Tags", tag_list)

        # set that one surface tag element to required surface tag
        component = Entity.components[component_index]
        path = "Configuration|Inclusion|Surface Tags|[0]|Surface Tag"
        editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", component, path, surface_tag)
        new_value = hydra.get_component_property_value(component, path)

        if new_value == surface_tag:
            Report.info("Inclusive surface mask filter of terrainHole is added successfully")
        else:
            Report.info("Failed to add an Inclusive surface mask filter of terrainHole")

    def update_generated_surface_tag(Entity, component_index, surface_tag):
        tag_list = [surface_data.SurfaceTag()]

        # assign list with one surface tag to Generated Tags list
        hydra.get_set_test(Entity, component_index, "Configuration|Generated Tags", tag_list)

        # set that one surface tag element to required surface tag
        component = Entity.components[component_index]
        path = "Configuration|Generated Tags|[0]|Surface Tag"
        editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", component, path, surface_tag)
        new_value = hydra.get_component_property_value(component, path)

        if new_value == surface_tag:
            Report.info(f"Generated surface tag of {surface_tag} is added successfully")
        else:
            Report.info(f"Failed to add Generated surface tag of {surface_tag}")

    # 1) Open an existing simple level
    hydra.open_base_level()

    general.set_current_view_position(512.0, 480.0, 38.0)

    # 2) Create entity with components "Vegetation Layer Spawner", "Vegetation Asset List", "Box Shape"
    entity_position = math.Vector3(512.0, 512.0, 32.0)
    pink_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
    pink_flower_prefab = dynveg.create_temp_mesh_prefab(pink_flower_asset_path, "SurfaceMaskTagInclusion_PinkFlower")[0]
    spawner_entity = dynveg.create_temp_prefab_vegetation_area("Instance Spawner", entity_position, 10.0, 10.0, 10.0,
                                                               pink_flower_prefab)

    # 3) Add a Vegetation Surface Mask Filter component to the entity.
    spawner_entity.add_component("Vegetation Surface Mask Filter")

    # 4) Create 2 surface entities to represent terrain and terrain hole surfaces
    surface_tags: dict = {"terrainHole": 1327698037, "terrain": 3363197873}
    entity_position = math.Vector3(510.0, 512.0, 32.0)
    surface_entity_1 = dynveg.create_surface_entity("Surface Entity 1",
                                                    entity_position,
                                                    10.0, 10.0, 1.0)
    update_generated_surface_tag(surface_entity_1, 1, surface_tags["terrainHole"])

    entity_position = math.Vector3(520.0, 512.0, 32.0)
    surface_entity_2 = dynveg.create_surface_entity("Surface Entity 2",
                                                    entity_position,
                                                    10.0, 10.0, 1.0)
    update_generated_surface_tag(surface_entity_2, 1, surface_tags["terrain"])

    # 5) Add an Inclusion List tag to the component, and set it to "terrainHole".
    update_surface_tag_inclusion_list(spawner_entity, 3, surface_tags["terrainHole"])

    # 6) Check spawn count with default Inclusion Weights
    num_expected_instances = 130
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                               num_expected_instances), 2.0)
    Report.result(Tests.default_inclusion_weight, success)

    # 7) Check spawn count with Inclusion Weight Max set below 1.0
    hydra.get_set_test(spawner_entity, 3, "Configuration|Inclusion|Weight Max", 0.9)
    num_expected_instances = 0
    success = helper.wait_for_condition(lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id,
                                                                                               num_expected_instances), 2.0)
    Report.result(Tests.inclusion_weight_below_one, success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(SurfaceMaskFilter_InclusionList)
