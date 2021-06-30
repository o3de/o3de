"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.asset as asset
import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.math as math

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestPhysXColliderSurfaceTagEmitter(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="PhysXColliderSurfaceTagEmitter_E2E_Editor", args=["level"])

    def validate_behavior_context(self):
        # Verify that we can create the component through the BehaviorContext
        behavior_context_test_success = True
        test_component = azlmbr.surface_data.SurfaceDataColliderComponent()
        behavior_context_test_success = behavior_context_test_success and (test_component is not None)
        behavior_context_test_success = behavior_context_test_success and (test_component.typename ==
                                                                           'SurfaceDataColliderComponent')

        # Verify that we can get/set the tags through the BehaviorContext
        provider_tag1 = azlmbr.surface_data.SurfaceTag('provider_tag1')
        provider_tag2 = azlmbr.surface_data.SurfaceTag('provider_tag2')
        modifier_tag1 = azlmbr.surface_data.SurfaceTag('modifier_tag1')
        modifier_tag2 = azlmbr.surface_data.SurfaceTag('modifier_tag2')
        behavior_context_test_success = behavior_context_test_success and hydra.get_set_property_test(test_component,
                                                                                                      'providerTags',
                                                                                                      [provider_tag1,
                                                                                                       provider_tag2])
        behavior_context_test_success = behavior_context_test_success and hydra.get_set_property_test(test_component,
                                                                                                      'modifierTags',
                                                                                                      [modifier_tag1,
                                                                                                       modifier_tag2])
        self.log(f'SurfaceDataColliderComponent() BehaviorContext test: {behavior_context_test_success}')
        return behavior_context_test_success

    def run_test(self):
        """
        Summary:
        Test aspects of the PhysX Collider Surface Tag Emitter Component through the BehaviorContext and the Property Tree.

        :return: None
        """
        # Create an empty level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # Verify all of the BehaviorContext API:
        self.test_success = self.test_success and self.validate_behavior_context()

        # Set up a test environment to validate the PhysX Collider Surface Tag Emitter Component.
        # The test environment will consist of the following:
        # - a 32 x 32 x 1 box shape that emits a surface with no tags
        # - a 32 x 32 x 32 vegetation area that will only place vegetation on surfaces with the 'test' tag
        # With this setup, no vegetation will appear until a Surface Tag Emitter either emits new points with
        # the correct tag, or modifies points on our box shape to emit the correct tag.

        # Initialize some arbitrary constants for our test
        entity_center_point = math.Vector3(512.0, 512.0, 100.0)
        invalid_tag = azlmbr.surface_data.SurfaceTag('invalid')
        surface_tag = azlmbr.surface_data.SurfaceTag('test')
        test_box_size = 32.0
        baseline_surface_height = 1.0
        collider_radius = 4.0
        collider_diameter = collider_radius * 2.0

        # Set viewport view of area under test, and toggle helpers back on
        general.set_current_view_position(512.0, 485.0, 110.0)
        general.set_current_view_rotation(-35.0, 0.0, 0.0)
        general.toggle_helpers()

        # Create the "flat surface" entity to use as our baseline surface
        dynveg.create_surface_entity("Baseline Surface", entity_center_point, 32.0, 32.0, 1.0)

        # Create a new entity with required vegetation area components
        asset_path = os.path.join("Slices", "PinkFlower.dynamicslice")
        spawner_entity = dynveg.create_vegetation_area("Veg Area", entity_center_point, 32.0, 32.0, 32.0, asset_path)

        # Add a Vegetation Surface Mask Filter component to the spawner entity and set it to include the "test" tag
        spawner_entity.add_component("Vegetation Surface Mask Filter")
        spawner_entity.get_set_test(3, "Configuration|Inclusion|Surface Tags", [surface_tag])

        # At this point, there should be 0 instances within our entire veg area
        initial_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(entity_center_point, 16.0, 0),
                                                  5.0)
        self.test_success = self.test_success and initial_success

        # Create an entity with a PhysX Collider and our PhysX Collider Surface Tag Emitter
        collider_entity = hydra.Entity("Collider Surface")
        collider_entity.create_entity(
            entity_center_point, 
            ["PhysX Collider", "PhysX Collider Surface Tag Emitter"]
            )
        if collider_entity.id.IsValid():
            self.log(f"'{collider_entity.name}' created")

        # Set up the PhysX Collider so that each shape type (sphere, box, capsule) has the same test height.
        hydra.get_set_test(collider_entity, 0, "Shape Configuration|Sphere|Radius", collider_radius)
        hydra.get_set_test(collider_entity, 0, "Shape Configuration|Box|Dimensions", math.Vector3(collider_diameter,
                                                                                                  collider_diameter,
                                                                                                  collider_diameter))
        hydra.get_set_test(collider_entity, 0, "Shape Configuration|Capsule|Height", collider_diameter)

        # Run through each collider shape type (sphere, box, capsule) and verify the surface generation
        # and surface modification of the PhysX Collision Surface Tag Emitter Component.
        for collider_shape in range(0, 3):
            hydra.get_set_test(collider_entity, 0, "Shape Configuration|Shape", collider_shape)

            # Test:  Generate a new surface on the collider.
            # There should be one instance at the very top of the collider sphere, and none on the baseline surface
            # (We use a small query box to only check for one placed instance point)
            hydra.get_set_test(collider_entity, 1, "Configuration|Generated Tags", [surface_tag])
            hydra.get_set_test(collider_entity, 1, "Configuration|Extended Tags", [invalid_tag])
            top_point = math.Vector3(entity_center_point.x, entity_center_point.y, entity_center_point.z +
                                     collider_radius)
            baseline_surface_point = math.Vector3(entity_center_point.x, entity_center_point.y, entity_center_point.z +
                                                  (baseline_surface_height / 2.0))
            top_point_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(top_point, 0.25, 1), 5.0)
            self.test_success = self.test_success and top_point_success
            baseline_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(baseline_surface_point,
                                                                                              0.25, 0), 5.0)
            self.test_success = self.test_success and baseline_success

            # Test:  Modify an existing surface inside the collider.
            # There should be no instances at the very top of the collider sphere, and one on the baseline surface
            # within our query box.
            # (We use a small query box to only check for one placed instance point)
            hydra.get_set_test(collider_entity, 1, "Configuration|Generated Tags", [invalid_tag])
            hydra.get_set_test(collider_entity, 1, "Configuration|Extended Tags", [surface_tag])
            top_point_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(top_point, 0.25, 0), 5.0)
            self.test_success = self.test_success and top_point_success
            baseline_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(baseline_surface_point,
                                                                                              0.25, 1), 5.0)
            self.test_success = self.test_success and baseline_success

        # Setup collider entity with a PhysX Mesh
        test_physx_mesh_asset_path = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetIdByPath", os.path.join("levels", "physics",
                                                            "c4044697_material_perfacematerialvalidation",
                                                            "test.pxmesh"), math.Uuid(), False)
        hydra.get_set_test(collider_entity, 0, "Shape Configuration|Shape", 7)
        hydra.get_set_test(collider_entity, 0, "Shape Configuration|Asset|PhysX Mesh", test_physx_mesh_asset_path)

        # Set the asset scale to match the test heights of the shapes tested
        asset_scale = math.Vector3(1.0, 1.0, 9.0)
        collider_entity.get_set_test(0, "Shape Configuration|Asset|Configuration|Asset Scale", asset_scale)

        # Test:  Generate a new surface on the collider.
        # There should be one instance at the very top of the collider mesh, and none on the baseline surface
        # (We use a small query box to only check for one placed instance point)
        self.log("Starting PhysX Mesh Collider Test")
        hydra.get_set_test(collider_entity, 1, "Configuration|Generated Tags", [surface_tag])
        hydra.get_set_test(collider_entity, 1, "Configuration|Extended Tags", [invalid_tag])
        top_point_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(top_point, 0.25, 1), 5.0)
        self.test_success = self.test_success and top_point_success
        baseline_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(baseline_surface_point,
                                                                                          0.25, 0), 5.0)
        self.test_success = self.test_success and baseline_success

        # Test:  Modify an existing surface inside the collider.
        # There should be no instances at the very top of the collider mesh, and none on the baseline surface within
        # our query box as PhysX meshes are treated as hollow shells, not solid volumes.
        # (We use a small query box to only check for one placed instance point)
        hydra.get_set_test(collider_entity, 1, "Configuration|Generated Tags", [invalid_tag])
        hydra.get_set_test(collider_entity, 1, "Configuration|Extended Tags", [surface_tag])
        top_point_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(top_point, 0.25, 0), 5.0)
        self.test_success = self.test_success and top_point_success
        baseline_success = self.wait_for_condition(lambda: dynveg.validate_instance_count(baseline_surface_point,
                                                                                          0.25, 0), 5.0)
        self.test_success = self.test_success and baseline_success


test = TestPhysXColliderSurfaceTagEmitter()
test.run()
