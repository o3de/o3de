"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

animation_components = [
    "Actor",
    "Anim Graph",
    "Attachment",
    "Simple LOD Distance",
    "Simple Motion"
]

audio_components = [
    "Audio Animation",
    "Audio Area Environment",
    "Audio Environment",
    "Audio Listener",
    "Audio Preload",
    "Audio Proxy",
    "Audio Rtpc",
    "Audio Switch",
    "Audio Trigger",
    "Multi-Position Audio"
]

automated_testing_components = [
    "Network Test Level Entity",
    "Network Test Player",
    "Simple Script Player"
]

camera_components = [
    "Camera",
    "Camera Rig"
]

debugging_components = [
    "DebugDraw Line",
    "DebugDraw Obb",
    "DebugDraw Ray",
    "DebugDraw Sphere",
    "DebugDraw Text"
]

editor_components = [
    "Comment"
]

gameplay_components = [
    "Fly Camera Input",
    "Input",
    "Look At",
    "Random Timed Spawner",
    "Simple State",
    "Spawner",
    "Tag"
]

gradient_modifier_components = [
    "Dither Gradient Modifier",
    "Gradient Mixer",
    "Gradient Transform Modifier",
    "Invert Gradient Modifier",
    "Levels Gradient Modifier",
    "Posterize Gradient Modifier",
    "Smooth-Step Gradient Modifier",
    "Threshold Gradient Modifier"
]


gradient_components = [
    "Altitude Gradient",
    "Constant Gradient",
    "FastNoise Gradient",
    "Gradient Baker",
    "Image Gradient",
    "Perlin Noise Gradient",
    "Random Noise Gradient",
    "Reference Gradient",
    "Shape Falloff Gradient",
    "Slope Gradient",
    "Surface Mask Gradient"
]

graphics_environment_components = [
    "Deferred Fog",
    "HDRi Skybox",
    "Physical Sky",
    "Sky Atmosphere",
    "Stars"
]

graphics_lighting_components = [
    "CubeMap Capture",
    "Diffuse Probe Grid",
    "Directional Light",
    "Global Skylight (IBL)",
    "Light",
    "Reflection Probe"
]

graphics_mesh_components = [
    "Atom Hair",
    "Decal",
    "Material",
    "Mesh"
]

graphics_occlusion_components = [
    "Occlusion Culling Plane"
]

other_components = [
    "Grid"
]

postfx_components = [
    "Bloom",
    "Chromatic Aberration",
    "Depth Of Field",
    "Display Mapper",
    "Exposure Control",
    "HDR Color Grading",
    "Look Modification",
    "PostFX Gradient Weight Modifier",
    "PostFX Layer",
    "PostFX Radius Weight Modifier",
    "PostFX Shape Weight Modifier",
    "SSAO"
]

input_components = [
    "Input Context"
]

miscellaneous_components = [
    "Detour Navigation Component",
    "Entity Reference",
    "Recast Navigation Mesh",
    "Recast Navigation PhysX Provider"
]

multiplayer_components = [
    "Local Prediction Player Input",
    "Network Binding",
    "Network Character",
    "Network Debug Player Id",
    "Network Hierarchy Child",
    "Network Hierarchy Root",
    "Network Hit Volumes",
    "Network Rigid Body",
    "Network Transform"
]

physx_components = [
    "Cloth",
    "PhysX Ball Joint",
    "PhysX Character Controller",
    "PhysX Character Gameplay",
    "PhysX Dynamic Rigid Body",
    "PhysX Fixed Joint",
    "PhysX Force Region",
    "PhysX Heightfield Collider",
    "PhysX Hinge Joint",
    "PhysX Mesh Collider",
    "PhysX Primitive Collider",
    "PhysX Prismatic Joint",
    "PhysX Ragdoll",
    "PhysX Shape Collider",
    "PhysX Static Rigid Body"
]

scripting_components = [
    "Lua Script",
    "Script Canvas"
]

shape_components = [
    "Axis Aligned Box Shape",
    "Box Shape",
    "Capsule Shape",
    "Compound Shape",
    "Cylinder Shape",
    "Disk Shape",
    "Polygon Prism Shape",
    "Quad Shape",
    "Shape Reference",
    "Sphere Shape",
    "Spline",
    "Tube Shape",
    "White Box",
    "White Box Collider"
]

surface_data_components = [
    "Gradient Surface Tag Emitter",
    "Mesh Surface Tag Emitter",
    "PhysX Collider Surface Tag Emitter",
    "Shape Surface Tag Emitter"
]

terrain_components = [
    "Terrain Height Gradient List",
    "Terrain Layer Spawner",
    "Terrain Macro Material",
    "Terrain Physics Heightfield Collider",
    "Terrain Surface Gradient List",
    "Terrain Surface Materials List"
]

test_components = [
    "AssetCollectionAsyncLoaderTest"
]

ui_components = [
    "UI Canvas Asset Ref",
    "UI Canvas Proxy Ref",
    "UI Canvas on Mesh"
]

vegetation_components = [
    "Landscape Canvas",
    "Vegetation Asset List",
    "Vegetation Asset List Combiner",
    "Vegetation Asset Weight Selector",
    "Vegetation Layer Blender",
    "Vegetation Layer Blocker",
    "Vegetation Layer Blocker (Mesh)",
    "Vegetation Layer Debugger",
    "Vegetation Layer Spawner"
]

vegetation_filter_components = [
    "Vegetation Altitude Filter",
    "Vegetation Distance Between Filter",
    "Vegetation Distribution Filter",
    "Vegetation Shape Intersection Filter",
    "Vegetation Slope Filter",
    "Vegetation Surface Mask Depth Filter",
    "Vegetation Surface Mask Filter"
]

vegetation_modifier_components = [
    "Vegetation Position Modifier",
    "Vegetation Rotation Modifier",
    "Vegetation Scale Modifier",
    "Vegetation Slope Alignment Modifier"
]

all_component_categories = [
    animation_components,
    audio_components,
    automated_testing_components,
    camera_components,
    debugging_components,
    editor_components,
    gameplay_components,
    gradient_modifier_components,
    gradient_components,
    graphics_environment_components,
    graphics_lighting_components,
    graphics_mesh_components,
    graphics_occlusion_components,
    other_components,
    postfx_components,
    input_components,
    miscellaneous_components,
    multiplayer_components,
    physx_components,
    scripting_components,
    shape_components,
    surface_data_components,
    terrain_components,
    test_components,
    ui_components,
    vegetation_components,
    vegetation_filter_components,
    vegetation_modifier_components
]


class Tests:
    dpe_enabled = (
        "DPE is enabled",
        "DPE is not enabled"
    )
    test_entity_created = (
        "Test entity created successfully",
        "Failed to create test entity"
    )


def DPE_AllComponentsAddedRemoved():
    """
    With the DPE Enabled, validates that every component available in the AutomatedTesting project can be successfully
    added and removed from an entity.
    """

    import azlmbr.legacy.general as general

    from editor_python_test_tools.utils import TestHelper, Report, Tracer
    from editor_python_test_tools.editor_entity_utils import EditorEntity

    def add_remove_component(component_name):
        # Create a new entity for component operations
        entity = EditorEntity.create_editor_entity()
        Report.critical_result(Tests.test_entity_created, entity.exists())

        # Add the specified component and validate
        entity.add_component(component_name)
        component_added = TestHelper.wait_for_condition(lambda: entity.has_component(component_name), 5.0)
        assert component_added, f"Failed to add {component_name} to entity"

        # Undo the component add and validate
        general.undo()
        undo_removes_component = TestHelper.wait_for_condition(lambda: not entity.has_component(component_name), 5.0)
        assert undo_removes_component, f"Undo failed: {component_name} still found on entity"
        # Redo the component add and validate
        general.redo()
        redo_readds_component = TestHelper.wait_for_condition(lambda: entity.has_component(component_name), 5.0)
        assert redo_readds_component, f"Redo failed: {component_name} not found on entity"

        # Remove the component and validate
        entity.remove_component(component_name)
        component_removed = TestHelper.wait_for_condition(lambda: not entity.has_component(component_name), 5.0)
        assert component_removed, f"Failed to remove {component_name} from entity"
        
        # Undo the removal and validate
        general.undo()
        undo_readds_component = TestHelper.wait_for_condition(lambda: entity.has_component(component_name), 5.0)
        assert undo_readds_component, f"Undo failed: {component_name} not found on entity"

        # Redo the removal and validate
        general.redo()
        redo_removes_component = TestHelper.wait_for_condition(lambda: not entity.has_component(component_name), 5.0)
        assert redo_removes_component, f"Redo failed: {component_name} still found on entity"

        # Delete the test entity, and return True if we have not triggered an assertion
        entity.delete()
        return True

    with Tracer() as error_tracer:
        # Open the base level
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Verify the DPE is enabled
        Report.critical_result(Tests.dpe_enabled, general.get_cvar("ed_enableDPEInspector") == "true")

        # Loop through list of components
        for component_category in all_component_categories:
            for component in component_category:
                component_results = (
                    f"{component} component: All Add/Remove operations successful",
                    f"{component} component: Add/Remove operation(s) failed"
                )
                Report.result(component_results, add_remove_component(component))

    # Look for errors and asserts
    TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
    for error_info in error_tracer.errors:
        Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
    for assert_info in error_tracer.asserts:
        Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(DPE_AllComponentsAddedRemoved)
