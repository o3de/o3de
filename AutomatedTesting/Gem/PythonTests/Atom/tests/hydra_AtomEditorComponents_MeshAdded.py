"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    creation_undo = (
        "UNDO Entity creation success",
        "P0: UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "P0: REDO Entity creation failed")
    mesh_entity_creation = (
        "Mesh Entity successfully created",
        "P0: Mesh Entity failed to be created")
    mesh_component_added = (
        "Entity has a Mesh component",
        "P0: Entity failed to find Mesh component")
    model_asset_specified = (
        "Model Asset set",
        "P0: Model Asset not set")
    enter_game_mode = (
        "Entered game mode",
        "P0: Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "P0: Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "P0: Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "P0: Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "P0: Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "P0: UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "P0: REDO deletion failed")
    mesh_sort_key = (
        "Mesh Sort key property set",
        "P1: Mesh Sort key property not correctly set")
    mesh_ray_tracing = (
        "Mesh Use ray tracing property set",
        "P1: Mesh Use ray tracing property not correctly set")
    mesh_cubemap = (
        "Mesh Exclude from reflection cubemaps property set",
        "P1: Mesh Exclude from reflection cubemaps property not correctly set")
    mesh_forward_pass = (
        "Mesh Use Forward Pass IBL Specular property set",
        "P1: Mesh Use Forward Pass IBL Specular property not correctly set")
    mesh_lod_type_screen_coverage = (
        "Mesh LOD type screen coverage set",
        "P1: Mesh LOD type screen coverage not correctly set")
    mesh_lod_type_specific_lod = (
        "Mesh LOD type Specific LOD set",
        "P1: Mesh LOD type Specific LOD not correctly set")
    mesh_lod_type_default = (
        "Mesh LOD type default set",
        "P1: Mesh LOD type default not correctly set")
    mesh_screen_coverage = (
        "Mesh Minimum Screen Coverage property set",
        "P1: Mesh Minimum Screen Coverage property not correctly set")
    mesh_quality_decay = (
        "Mesh Quality Decay Rate property set",
        "P1: Mesh Quality Decay Rate property not correctly set")
    mesh_lod_override = (
        "Mesh Lod Override property set",
        "P1: Mesh Lod Override property property not correctly set")
    has_material = (
        "Mesh entity has a material component",
        "P1: Mesh entity did not add a material component")
    mesh_component_removed = (
        "Mesh component removed successfully",
        "P1: Mesh component was not correctly removed from the entity")
    model_asset_is_optimized = (
        "tube.fbx.azmodel has <= 66 vertices",
        "P0: Model has not been fully optimized")
    model_different_bone_ids_same_position_should_weld_vertices = (
        "sameposition_differentboneIds_shouldnotweldvertices.fbx.azmodel has 48 vertices",
        "P0: Vertices were welded when they shouldnt be")


def AtomEditorComponents_Mesh_AddedToEntity():
    """
    Summary:
    Tests the Mesh component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Mesh entity with no components.
    2) Add a Mesh component to Mesh entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Specify the Mesh component asset
    6) Set Mesh component Sort Key property
    7) Set Mesh component Use ray tracing property
    8) Set Mesh component Exclude from reflection cubemaps property
    9) Set Mesh component Use Forward Pass IBL Specular property
    10) Set Mesh component LOD Type: Screen Coverage property
    11) Set Mesh component Minimum Screen Coverage property
    12) Set Mesh component Quality Decay Rate property
    13) Set Mesh component LOD Type: Specific Lod property
    14) Set Mesh component Lod Override property
    15) Set Mesh component LOD Type: Default property
    16) Set Mesh component Add Material Component and confirm a Material component added
    17) Remove Mesh component then UNDO the remove
    18) Enter/Exit game mode.
    19) Test IsHidden.
    20) Test IsVisible.
    21) Verify that vertex welding is functioning
    22) Verify that vertices with the same position but different joint ids aren't welded
    23) Delete Mesh entity.
    24) UNDO deletion.
    25) REDO deletion.
    26) Look for errors.

    :return: None
    """

    import os

    from PySide2 import QtWidgets

    import azlmbr.bus
    from functools import partial
    import azlmbr.legacy.general as general
    from Atom.atom_utils.atom_constants import (MESH_LOD_TYPE,
                                                AtomComponentProperties)
    import pyside_utils
    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, TestHelper, Tracer

    class OnModelReadyHelper:
        def __init__(self):
            self.isModelReady = False
        
        def model_is_ready_predicate(self):
            """
            A predicate function what will be used in wait_for_condition.
            """
            return self.isModelReady

        def on_model_ready(self, parameters):
            self.isModelReady = True

        def wait_for_on_model_ready(self, entityId, mesh_component, model_id):
            self.isModelReady = False
            # Connect to the MeshNotificationBus
            # Listen for notifications when entities are created/deleted
            self.onModelReadyHandler = azlmbr.bus.NotificationHandler('MeshComponentNotificationBus')
            self.onModelReadyHandler.connect(entityId)
            self.onModelReadyHandler.add_callback('OnModelReady', self.on_model_ready)
            
            waitCondition = partial(self.model_is_ready_predicate)
            
            mesh_component.set_component_property_value(AtomComponentProperties.mesh('Model Asset'), model_id)
            if TestHelper.wait_for_condition(waitCondition, 20.0):
                return True
            else:
                return False

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a Mesh entity with no components.
        mesh_entity = EditorEntity.create_editor_entity(AtomComponentProperties.mesh())
        Report.critical_result(Tests.mesh_entity_creation, mesh_entity.exists())

        # 2. Add a Mesh component to Mesh entity.
        mesh_component = mesh_entity.add_component(AtomComponentProperties.mesh())
        Report.critical_result(
            Tests.mesh_component_added,
            mesh_entity.has_component(AtomComponentProperties.mesh()))

        # 3. UNDO the entity creation and component addition.
        # -> UNDO component addition.
        general.undo()
        # -> UNDO naming entity.
        general.undo()
        # -> UNDO selecting entity.
        general.undo()
        # -> UNDO entity creation.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_undo, not mesh_entity.exists())

        # 4. REDO the entity creation and component addition.
        # -> REDO entity creation.
        general.redo()
        # -> REDO selecting entity.
        general.redo()
        # -> REDO naming entity.
        general.redo()
        # -> REDO component addition.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_redo, mesh_entity.exists())

        # 5. Set Mesh component asset property
        model_path = os.path.join('testdata', 'objects', 'modelhotreload', 'sphere_5lods.fbx.azmodel')
        model = Asset.find_asset_by_path(model_path)
        mesh_component.set_component_property_value(AtomComponentProperties.mesh('Model Asset'), model.id)
        Report.result(Tests.model_asset_specified,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Model Asset')) == model.id)

        # 6. Set Mesh component Sort Key property
        # This part of the test is currently disabled due to a bug.
        # It will be re-enabled in a future update once the bug is fixed.
        # mesh_component.set_component_property_value(
        #     AtomComponentProperties.mesh('Sort Key'), value=23456789)
        # Report.result(Tests.mesh_sort_key,
        #               mesh_component.get_component_property_value(
        #                   AtomComponentProperties.mesh('Sort Key')) == 23456789)

        # 7. Set Mesh component Use ray tracing property
        mesh_component.set_component_property_value(AtomComponentProperties.mesh('Use ray tracing'), value=False)
        Report.result(Tests.mesh_ray_tracing,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Use ray tracing')) is False)

        # 8. Set Mesh component Exclude from reflection cubemaps property
        mesh_component.set_component_property_value(
            AtomComponentProperties.mesh('Exclude from reflection cubemaps'), value=True)
        Report.result(Tests.mesh_cubemap,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Exclude from reflection cubemaps')) is True)

        # 9. Set Mesh component Use Forward Pass IBL Specular property
        mesh_component.set_component_property_value(
            AtomComponentProperties.mesh('Use Forward Pass IBL Specular'), value=True)
        Report.result(Tests.mesh_forward_pass,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Use Forward Pass IBL Specular')) is True)

        # 10. Set Mesh component LOD Type: Screen Coverage property
        print(mesh_component.get_component_property_value(AtomComponentProperties.mesh('Lod Type')))
        mesh_component.set_component_property_value(
            AtomComponentProperties.mesh('Lod Type'), value=MESH_LOD_TYPE['screen coverage'])
        Report.result(Tests.mesh_lod_type_screen_coverage,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Lod Type')) == MESH_LOD_TYPE['screen coverage'])

        # 11. Set Mesh component Minimum Screen Coverage property
        mesh_component.set_component_property_value(
            AtomComponentProperties.mesh('Minimum Screen Coverage'), value=1.0)
        Report.result(Tests.mesh_screen_coverage,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Minimum Screen Coverage')) == 1.0)

        # 12. Set Mesh component Quality Decay Rate property
        mesh_component.set_component_property_value(
            AtomComponentProperties.mesh('Quality Decay Rate'), value=1.0)
        Report.result(Tests.mesh_quality_decay,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Quality Decay Rate')) == 1.0)

        # 13. Set Mesh component LOD Type: Specific Lod property
        mesh_component.set_component_property_value(
            AtomComponentProperties.mesh('Lod Type'), value=MESH_LOD_TYPE['specific lod'])
        Report.result(Tests.mesh_lod_type_specific_lod,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Lod Type')) == MESH_LOD_TYPE['specific lod'])

        # 14. Set Mesh component Lod Override property
        mesh_component.set_component_property_value(
            AtomComponentProperties.mesh('Lod Override'), value=2)
        Report.result(Tests.mesh_lod_override,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Lod Override')) == 2)

        # 15. Set Mesh component LOD Type: Default property
        mesh_component.set_component_property_value(
            AtomComponentProperties.mesh('Lod Type'), value=MESH_LOD_TYPE['default'])
        Report.result(Tests.mesh_lod_type_default,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Lod Type')) == MESH_LOD_TYPE['default'])

        # 16. Set Mesh component Add Material Component and confirm a Material component added
        # Make sure the Entity Inspector is open and trigger the "Add Material Component" button
        general.open_pane("Inspector")
        editor_window = pyside_utils.get_editor_main_window()
        entity_inspector = editor_window.findChild(QtWidgets.QDockWidget, "Inspector")
        add_material_component_button = pyside_utils.find_child_by_pattern(entity_inspector, "Add Material Component")
        add_material_component_button.click()
        Report.result(Tests.has_material, mesh_entity.has_component(AtomComponentProperties.material()))

        # 17. Remove Mesh component then UNDO the remove
        mesh_component.remove()
        general.idle_wait_frames(1)
        Report.result(Tests.mesh_component_removed, not mesh_entity.has_component(AtomComponentProperties.mesh()))
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.mesh_component_added, mesh_entity.has_component(AtomComponentProperties.mesh()))

        # 18. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 19. Test IsHidden.
        mesh_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, mesh_entity.is_hidden() is True)

        # 20. Test IsVisible.
        mesh_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, mesh_entity.is_visible() is True)
        
        # 21. Test that vertex welding is functioning
        model_path = os.path.join('testdata', 'objects', 'tube.fbx.azmodel')
        model = Asset.find_asset_by_path(model_path)
        onModelReadyHelper = OnModelReadyHelper()
        onModelReadyHelper.wait_for_on_model_ready(mesh_entity.id, mesh_component, model.id)
        Report.result(Tests.model_asset_specified,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Model Asset')) == model.id)

        Report.result(Tests.model_asset_is_optimized,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Vertex Count LOD0')) <= 66)

        # 22. Test that vertices with the same position but different boneId's aren't unintentionally welded
        model_path = os.path.join('testdata', 'objects', 'skinnedmesh', 'meshoptimization', 'sameposition_differentjointids_shouldnotweldvertices.fbx.azmodel')
        model = Asset.find_asset_by_path(model_path)
        onModelReadyHelper = OnModelReadyHelper()
        onModelReadyHelper.wait_for_on_model_ready(mesh_entity.id, mesh_component, model.id)
        Report.result(Tests.model_asset_specified,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Model Asset')) == model.id)

        Report.result(Tests.model_different_bone_ids_same_position_should_weld_vertices,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Vertex Count LOD0')) == 48)

        # 23. Delete Mesh entity.
        mesh_entity.delete()
        Report.result(Tests.entity_deleted, not mesh_entity.exists())

        # 24. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, mesh_entity.exists())

        # 25. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not mesh_entity.exists())

        # 26. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Mesh_AddedToEntity)
