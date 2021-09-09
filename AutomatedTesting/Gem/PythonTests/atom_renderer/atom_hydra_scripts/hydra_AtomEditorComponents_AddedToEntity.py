"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Hydra script that creates an entity and attaches Atom components to it for test verification.
"""

# fmt: off
class Tests:
    camera_creation = ("camera_entity Entity successfully created", "camera_entity Entity failed to be created")
    camera_component_added = ("Camera component was added to entity", "Camera component failed to be added to entity")
    camera_component_check = ("Entity has a Camera component", "Entity failed to find Camera component")
    creation_undo = ("UNDO Entity creation success", "UNDO Entity creation failed")
    creation_redo = ("REDO Entity creation success", "REDO Entity creation failed")
    depth_of_field_creation = ("DepthOfField Entity successfully created", "DepthOfField Entity failed to be created")
    depth_of_field_check = ("Entity has a DepthOfField component", "Entity failed to find DepthOfField component")
    post_fx_check = ("Entity has a Post FX Layer component", "Entity did not have a Post FX Layer component")
    enter_game_mode = ("Entered game mode", "Failed to enter game mode")
    exit_game_mode = ("Exited game mode", "Couldn't exit game mode")
    is_visible = ("Entity is visible", "Entity was not visible")
    is_hidden = ("Entity is hidden", "Entity was not hidden")
    entity_deleted = ("Entity deleted", "Entity was not deleted")
    deletion_undo = ("UNDO deletion success", "UNDO deletion failed")
    deletion_redo = ("REDO deletion success", "REDO deletion failed")
# fmt: on


def launch_atom_component_test(component_name, *additional_tests):
    """
    # TODO: REPLACE THIS.
    Summary:
    Test launcher for each component test.
    Below are the common tests done for each of the Atom components.
    1) Addition of component to the entity
    2) UNDO/REDO of addition of component
    3) Enter/Exit game mode
    4) Hide/Show entity containing component
    5) Deletion of component
    6) UNDO/REDO of deletion of component

    Some additional tests for specific components include
    1) Assigning value to some properties of each component
    2) Verifying if the component is activated only when the required components are added

    Expected Result:
    1) Component can be added to an entity.
    2) The addition of component can be undone and redone.
    3) Game mode can be entered/exited without issue.
    4) Entity with component can be hidden/shown.
    5) Component can be deleted.
    6) The deletion of component can be undone and redone.
    7) Component is activated only when the required components are added
    8) Values can be assigned to the properties of the component

    :return: None
    """
    import azlmbr.bus as bus
    import azlmbr.entity as entity
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.paths

    from editor_python_test_tools import hydra_editor_utils as hydra
    from editor_python_test_tools.utils import TestHelper as helper

    # Wait for Editor idle loop before executing Python hydra scripts.
    helper.init_idle()

    # Delete all existing entities initially
    search_filter = azlmbr.entity.SearchFilter()
    all_entities = entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
    editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", all_entities)

    def create_entity_undo_redo_component_addition(component_name):
        new_entity = hydra.Entity(f"{component_name}")
        new_entity.create_entity(math.Vector3(512.0, 512.0, 34.0), [component_name])

        general.undo()
        helper.wait_for_condition(lambda: not hydra.has_components(new_entity.id, [component_name]), 2.0)
        general.redo()
        helper.wait_for_condition(lambda: hydra.has_components(new_entity.id, [component_name]), 2.0)

        return new_entity

    def verify_hide_unhide_entity(entity_obj):
        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", entity_obj.id, False)
        general.idle_wait_frames(1)
        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", entity_obj.id, True)
        general.idle_wait_frames(1)

    def verify_deletion_undo_redo(entity_obj):
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntityById", entity_obj.id)
        helper.wait_for_condition(lambda: not hydra.find_entity_by_name(entity_obj.name), 2.0)

        general.undo()
        helper.wait_for_condition(lambda: hydra.find_entity_by_name(entity_obj.name) is not None, 2.0)

        general.redo()
        helper.wait_for_condition(lambda: not hydra.find_entity_by_name(entity_obj.name), 2.0)

    def verify_required_component_addition(entity_obj, components_to_add, component_name):

        def is_component_enabled(entity_componentid_pair):
            return editor.EditorComponentAPIBus(bus.Broadcast, "IsComponentEnabled", entity_componentid_pair)

        for component in components_to_add:
            entity_obj.add_component(component)
        helper.wait_for_condition(lambda: is_component_enabled(entity_obj.components[0]), 2.0)

    def verify_set_property(entity_obj, path, value):
        entity_obj.get_set_test(0, path, value)

#### TEST RUN CODE COPYPASTA ####
    # # Run common and additional tests
    # entity_obj = create_entity_undo_redo_component_addition(component_name)
    #
    # # Enter/Exit game mode test
    # helper.enter_game_mode(Tests.enter_game_mode)
    # helper.exit_game_mode(Tests.exit_game_mode)
    #
    # # Any additional tests are executed here
    # for test in additional_tests:
    #     test(entity_obj)
    #
    # # Hide/Unhide entity test
    # verify_hide_unhide_entity(entity_obj)
    #
    # # Deletion/Undo/Redo test
    # verify_deletion_undo_redo(entity_obj)


def AtomEditorComponents_DepthOfField_AddedToEntity():
    """
    Summary:
    Tests the DepthOfField component can be added to an entity and has the expected functionality.

    Expected Behavior:


    Test Steps:
    1) Addition of component to the entity
    2) UNDO/REDO of addition of component
    3) Enter/Exit game mode
    4) Hide/Show entity containing component
    5) Deletion of component
    6) UNDO/REDO of deletion of component
    7) Assigning value to some properties of each component
    8) Verifying if the component is activated only when the required components are added

    Expected Result:
    1) Component can be added to an entity.
    2) The addition of component can be undone and redone.
    3) Game mode can be entered/exited without issue.
    4) Entity with component can be hidden/shown.
    5) Component can be deleted.
    6) The deletion of component can be undone and redone.
    7) Component is activated only when the required components are added
    8) Values can be assigned to the properties of the component

    :return: None
    """
    import azlmbr.bus as bus
    import azlmbr.entity as entity
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.paths

    from editor_python_test_tools import hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    with Tracer() as error_tracer:
        # Wait for Editor idle loop before executing Python hydra scripts.
        helper.init_idle()

        # Delete all existing entities initially for test setup.
        search_filter = azlmbr.entity.SearchFilter()
        all_entities = entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", all_entities)

        # Set up camera entity and component.
        camera_entity = hydra.Entity("camera_entity")
        camera_entity.create_entity(math.Vector3(512.0, 512.0, 34.0), ["Camera"])
        Report.result(Tests.camera_creation, camera_entity.id.IsValid())

        # Create DepthOfField Entity with a DepthOfField component.
        depth_of_field = "DepthOfField"
        depth_of_field_entity = hydra.Entity(f"{depth_of_field}")
        depth_of_field_entity.create_entity(math.Vector3(512.0, 512.0, 34.0), [depth_of_field])
        Report.result(Tests.depth_of_field_creation, depth_of_field_entity.id.IsValid())

        # UNDO creation of DepthOfField entity.
        general.undo()
        Report.result(Tests.creation_undo, depth_of_field_entity.id.IsValid())

        # REDO creation of DepthOfField entity.
        general.redo()
        Report.result(Tests.creation_redo, depth_of_field_entity.id.isValid())

        # Enter/Exit game mode test.
        helper.enter_game_mode(Tests.enter_game_mode)
        helper.exit_game_mode(Tests.exit_game_mode)

        # Hidden test.
        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", depth_of_field_entity.id, False)
        is_hidden = editor.EditorEntityInfoRequestBus(bus.Event, 'IsHidden', depth_of_field_entity.id)
        Report.result(Tests.is_hidden, is_hidden is True)

        # Visible test.
        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", depth_of_field_entity.id, True)
        is_visible = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', depth_of_field_entity.id)
        Report.result(Tests.is_visible, is_visible is True)

        # Add PostFX Layer component.
        depth_of_field_entity.add_component("PostFX Layer")
        post_fx_component_added = editor.EditorComponentAPIBus(
            bus.Broadcast, "IsComponentEnabled", depth_of_field_entity.components[0])
        Report.result(Tests.post_fx_check, post_fx_component_added)

        # Add camera component.
        camera_component_added = depth_of_field_entity.get_set_test(
            0, "Controller|Configuration|Camera Entity", camera_entity.id)
        Report.result(Tests.camera_component_added, camera_component_added)

        # Deletion test.
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntityById", depth_of_field_entity.id)
        deleted_entity = hydra.find_entity_by_name(depth_of_field)
        Report.result(Tests.entity_deleted, len(deleted_entity) == 0)

        # Undo test.
        general.undo()
        entity_id = general.find_editor_entity(depth_of_field_entity)
        Report.result(Tests.deletion_undo, entity_id.IsValid())

        # Redo test
        general.redo()
        entity_id = general.find_editor_entity(depth_of_field)
        Report.result(Tests.deletion_redo, not entity_id.IsValid())

        # Look for errors
        helper.wait_for_condition(lambda: error_tracer.has_errors, 1.0)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)

        # general.undo()
        # helper.wait_for_condition(lambda: hydra.find_entity_by_name(depth_of_field_entity.name) is not None, 2.0)
        # general.redo()
        # helper.wait_for_condition(lambda: not hydra.find_entity_by_name(depth_of_field_entity.name), 2.0)


def AtomEditorComponents_Decal_AddedToEntity():
    pass


def AtomEditorComponents_DirectionalLight_AddedToEntity():
    pass


def AtomEditorComponents_ExposureControl_AddedToEntity():
    pass


def AtomEditorComponents_GlobalSkylightIBL_AddedToEntity():
    pass


def AtomEditorComponents_PhysicalSky_AddedToEntity():
    pass


def AtomEditorComponents_PostFXLayer_AddedToEntity():
    pass


def AtomEditorComponents_PostFXRadiusWeightModifier_AddedToEntity():
    pass


def AtomEditorComponents_Light_AddedToEntity():
    pass


def AtomEditorComponents_DisplayMapper_AddedToEntity():
    pass

    # # Decal Component
    # material_asset_path = os.path.join("AutomatedTesting", "Materials", "basic_grey.material")
    # material_asset = asset.AssetCatalogRequestBus(
    #     bus.Broadcast, "GetAssetIdByPath", material_asset_path, math.Uuid(), False)
    # ComponentTests(
    #     "Decal (Atom)", lambda entity_obj: verify_set_property(
    #         entity_obj, "Controller|Configuration|Material", material_asset))
    #
    # # Directional Light Component
    # ComponentTests(
    #     "Directional Light",
    #     lambda entity_obj: verify_set_property(
    #         entity_obj, "Controller|Configuration|Shadow|Camera", camera_entity.id))
    #
    # # Exposure Control Component
    # ComponentTests(
    #     "Exposure Control", lambda entity_obj: verify_required_component_addition(
    #         entity_obj, ["PostFX Layer"], "Exposure Control"))
    #
    # # Global Skylight (IBL) Component
    # diffuse_image_path = os.path.join("LightingPresets", "greenwich_park_02_4k_iblskyboxcm.exr.streamingimage")
    # diffuse_image_asset = asset.AssetCatalogRequestBus(
    #     bus.Broadcast, "GetAssetIdByPath", diffuse_image_path, math.Uuid(), False)
    # specular_image_path = os.path.join("LightingPresets", "greenwich_park_02_4k_iblskyboxcm.exr.streamingimage")
    # specular_image_asset = asset.AssetCatalogRequestBus(
    #     bus.Broadcast, "GetAssetIdByPath", specular_image_path, math.Uuid(), False)
    # ComponentTests(
    #     "Global Skylight (IBL)",
    #     lambda entity_obj: verify_set_property(
    #         entity_obj, "Controller|Configuration|Diffuse Image", diffuse_image_asset),
    #     lambda entity_obj: verify_set_property(
    #         entity_obj, "Controller|Configuration|Specular Image", specular_image_asset))
    #
    # # Physical Sky Component
    # ComponentTests("Physical Sky")
    #
    # # PostFX Layer Component
    # ComponentTests("PostFX Layer")
    #
    # # PostFX Radius Weight Modifier Component
    # ComponentTests("PostFX Radius Weight Modifier")
    #
    # # Light Component
    # ComponentTests("Light")
    #
    # # Display Mapper Component
    # ComponentTests("Display Mapper")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_DepthOfField_AddedToEntity)
