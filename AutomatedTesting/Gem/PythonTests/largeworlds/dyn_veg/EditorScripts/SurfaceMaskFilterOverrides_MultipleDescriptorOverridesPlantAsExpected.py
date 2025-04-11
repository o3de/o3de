"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    initial_surface_validation = (
        "Found all expected instances on all surfaces with initial setup",
        "Found an unexpected number of instances on all surfaces with initial setup"
    )
    surface_a_validation = (
        "Found the expected number of instances on Surface A",
        "Found an unexpected number of instances on Surface A"
    )
    surface_b_validation = (
        "Found the expected number of instances on Surface B",
        "Found an unexpected number of instances on Surface B"
    )
    surface_c_validation = (
        "Found the expected number of instances on Surface C",
        "Found an unexpected number of instances on Surface C"
    )


def SurfaceMaskFilterOverrides_MultipleDescriptorOverridesPlantAsExpected():
    """
    Summary:
    A new level is created. An instance spawner with 3 descriptors is created. 3 planting surfaces of different
    sizes are created and different surface tags are applied to each. Descriptor surface mask filter overrides are
    set and instance counts are validated.

    Expected Behavior:
    Instances plant on surfaces based on surface mask filter overrides.

    Test Steps:
     1) Open an existing level
     2) An instance spawner with 3 descriptors is created, and a Surface Mask Filter is added to the entity
     3) 3 surfaces of different sizes are created, and set to emit different tags
     4) Pre-test validation of instances
     5) Test 1 setup and validation: Inclusion tag matching surface a is set on a single descriptor
     6) Test 2 setup and validation: Inclusion tag matching surface b is set on a single descriptor
     7) Test 3 setup and validation: Inclusion tag matching surface c is set on a single descriptor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.surface_data as surface_data

    import editor_python_test_tools.hydra_editor_utils as hydra
    from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    surface_tag_list = [surface_data.SurfaceTag("test_tag"), surface_data.SurfaceTag("test_tag2"),
                        surface_data.SurfaceTag("test_tag3")]

    # 1) Open an existing simple level
    hydra.open_base_level()

    # Set view of planting area for visual debugging
    general.set_current_view_position(512.0, 500.0, 38.0)
    general.set_current_view_rotation(-20.0, 0.0, 0.0)

    # 2) Create a new instance spawner entity with multiple Prefab Instance Spawner descriptors
    spawner_center_point = math.Vector3(512.0, 512.0, 32.0)
    pink_flower_asset_path = os.path.join("assets", "objects", "foliage", "grass_flower_pink.fbx.azmodel")
    pink_flower_prefab = dynveg.create_temp_mesh_prefab(pink_flower_asset_path, "SurfaceMaskOverrides_PinkFlower")[0]
    spawner_entity = dynveg.create_temp_prefab_vegetation_area("Instance Spawner", spawner_center_point, 16.0, 16.0,
                                                               16.0, pink_flower_prefab)
    asset_list_component = spawner_entity.components[2]
    desc_asset = hydra.get_component_property_value(asset_list_component,
                                                    "Configuration|Embedded Assets")[0]
    desc_list = [desc_asset, desc_asset, desc_asset]
    spawner_entity.get_set_test(2, "Configuration|Embedded Assets", desc_list)

    # Add a Surface Mask Filter component to the spawner entity and toggle on Allow Overrides
    spawner_entity.add_component("Vegetation Surface Mask Filter")
    spawner_entity.get_set_test(3, "Configuration|Allow Per-Item Overrides", True)

    # 3) Create 3 surfaces for planting, spaced out vertically, and set expected instance counts for each surface
    surface_entity_a = dynveg.create_surface_entity("Surface Entity A", math.Vector3(512.0, 512.0, 32.0),
                                                    16.0, 16.0, 1.0)
    num_expected_surface_a = 20 * 20    # 20x20 instances on a 16x16 meter surface
    surface_entity_b = dynveg.create_surface_entity("Surface Entity B", math.Vector3(512.0, 512.0, 35.0),
                                                    12.0, 12.0, 1.0)
    num_expected_surface_b = 15 * 15    # 15x15 instances on a 12x12 meter surface
    surface_entity_c = dynveg.create_surface_entity("Surface Entity C", math.Vector3(512.0, 512.0, 38.0),
                                                    8.0, 8.0, 1.0)
    num_expected_surface_c = 10 * 10    # 10x10 instances on a 8x8 meter surface

    # Set each surface to emit a different tag
    surface_entity_a.get_set_test(1, "Configuration|Generated Tags", [surface_tag_list[0]])
    surface_entity_b.get_set_test(1, "Configuration|Generated Tags", [surface_tag_list[1]])
    surface_entity_c.get_set_test(1, "Configuration|Generated Tags", [surface_tag_list[2]])

    # 4) Initial Validation: Validate instance count in the spawner area. Instances should plant on all surfaces
    num_expected = num_expected_surface_a + num_expected_surface_b + num_expected_surface_c
    initial_success = helper.wait_for_condition(
        lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected), 5.0)
    Report.result(Tests.initial_surface_validation, initial_success)

    # 5)
    # Test #1 Setup: Set test_tag to inclusion list for descriptor 1. Set other descriptors to exclude all surfaces

    # Toggle on Display Per-Item Overrides and Surface Mask Filter Override for each descriptor
    for index in range(3):
        spawner_entity.get_set_test(2, f"Configuration|Embedded Assets|[{index}]|Display Per-Item Overrides", True)
        spawner_entity.get_set_test(2,
                                    f"Configuration|Embedded Assets|[{index}]|Surface Mask Filter|Override Mode", 1)

    spawner_entity.get_set_test(2,
                                "Configuration|Embedded Assets|[0]|Surface Mask Filter|Inclusion Tags",
                                [surface_tag_list[0]])
    spawner_entity.get_set_test(2,
                                "Configuration|Embedded Assets|[1]|Surface Mask Filter|Exclusion Tags",
                                surface_tag_list)
    spawner_entity.get_set_test(2,
                                "Configuration|Embedded Assets|[2]|Surface Mask Filter|Exclusion Tags",
                                surface_tag_list)

    # Test #1 Validation: Validate instance count. Should only plant on a single surface for 400 instances
    test_1_success = helper.wait_for_condition(
        lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected_surface_a), 5.0)
    Report.result(Tests.surface_a_validation, test_1_success)

    # 6)
    # Test #2 Setup: Set test_tag2 to inclusion for descriptor 1.
    spawner_entity.get_set_test(2,
                                "Configuration|Embedded Assets|[0]|Surface Mask Filter|Inclusion Tags",
                                [surface_tag_list[1]])

    # Test #2 Validation: Validate instance count. Should only plant on a single surface for 225 instances
    test_2_success = helper.wait_for_condition(
        lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected_surface_b), 5.0)
    Report.result(Tests.surface_b_validation, test_2_success)

    # 7)
    # Test #3 Setup: Set test_tag3 to inclusion for descriptor 1.
    spawner_entity.get_set_test(2,
                                "Configuration|Embedded Assets|[0]|Surface Mask Filter|Inclusion Tags",
                                [surface_tag_list[2]])

    # Test #3 Validation: Validate instance count. Should only plant on a single surface for 100 instances
    test_3_success = helper.wait_for_condition(
        lambda: dynveg.validate_instance_count_in_entity_shape(spawner_entity.id, num_expected_surface_c), 5.0)
    Report.result(Tests.surface_c_validation, test_3_success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(SurfaceMaskFilterOverrides_MultipleDescriptorOverridesPlantAsExpected)
