"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt: off
class Tests:
    camera_creation =             ("Camera Entity successfully created",                  "Camera Entity failed to be created")
    camera_component_added =      ("Camera component was added to entity",                "Camera component failed to be added to entity")
    camera_component_check =      ("Entity has a Camera component",                       "Entity failed to find Camera component")
    creation_undo =               ("UNDO Entity creation success",                        "UNDO Entity creation failed")
    creation_redo =               ("REDO Entity creation success",                        "REDO Entity creation failed")
    global_skylight_creation =    ("Global Skylight (IBL) Entity successfully created",   "Global Skylight (IBL) Entity failed to be created")
    global_skylight_component =   ("Entity has a Global Skylight (IBL) component",        "Entity failed to find Global Skylight (IBL) component")
    diffuse_image_set =           ("Entity has the Diffuse Image set",                    "Entity did not the Diffuse Image set")
    specular_image_set =          ("Entity has the Specular Image set",                   "Entity did not the Specular Image set")
    enter_game_mode =             ("Entered game mode",                                   "Failed to enter game mode")
    exit_game_mode =              ("Exited game mode",                                    "Couldn't exit game mode")
    is_visible =                  ("Entity is visible",                                   "Entity was not visible")
    is_hidden =                   ("Entity is hidden",                                    "Entity was not hidden")
    entity_deleted =              ("Entity deleted",                                      "Entity was not deleted")
    deletion_undo =               ("UNDO deletion success",                               "UNDO deletion failed")
    deletion_redo =               ("REDO deletion success",                               "REDO deletion failed")
    no_error_occurred =           ("No errors detected",                                  "Errors were detected")
# fmt: on


def AtomEditorComponents_GlobalSkylightIBL_AddedToEntity():
    """
    Summary:
    Tests the Global Skylight (IBL) component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Global Skylight (IBL) entity with no components.
    2) UNDO the entity creation.
    3) REDO the entity creation.
    4) Enter/Exit game mode.
    5) Test IsHidden.
    6) Test IsVisible.
    7) Add Post FX Layer component.
    8) Add Camera component
    9) Delete Global Skylight (IBL) entity.
    10) UNDO deletion.
    11) REDO deletion.
    12) Look for errors.

    :return: None
    """
    import os

    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.entity as entity
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.paths

    from editor_python_test_tools import hydra_editor_utils as hydra
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper as helper

    with Tracer() as error_tracer:
        # Wait for Editor idle loop before executing Python hydra scripts.
        helper.init_idle()
        helper.open_level(os.path.join(azlmbr.paths.devassets, "Levels"), "Base")

        # Delete all existing entities initially for test setup.
        search_filter = azlmbr.entity.SearchFilter()
        all_entities = entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", all_entities)

        # 1. Create a Global Skylight (IBL) entity with no components.
        global_skylight = "Global Skylight (IBL)"
        global_skylight_entity = hydra.Entity(f"{global_skylight}")
        global_skylight_entity.create_entity(math.Vector3(512.0, 512.0, 34.0), [global_skylight])
        has_component = hydra.has_components(global_skylight_entity.id, [global_skylight])
        Report.result(Tests.global_skylight_creation, global_skylight_entity.id.IsValid())
        Report.critical_result(Tests.global_skylight_component, has_component)

        # 2. UNDO the entity creation.
        general.undo()
        Report.result(Tests.creation_undo, len(hydra.find_entity_by_name(global_skylight_entity)) == 0)

        # 3. REDO the entity creation.
        general.redo()
        Report.result(Tests.creation_redo, len(hydra.find_entity_by_name(global_skylight_entity)) == 1)

        # 4. Enter/Exit game mode.
        helper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        helper.exit_game_mode(Tests.exit_game_mode)

        # 5. Test IsHidden.
        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", global_skylight_entity.id, False)
        is_hidden = editor.EditorEntityInfoRequestBus(bus.Event, 'IsHidden', global_skylight_entity.id)
        Report.result(Tests.is_hidden, is_hidden is True)

        # 6. Test IsVisible.
        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", global_skylight_entity.id, True)
        is_visible = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', global_skylight_entity.id)
        Report.result(Tests.is_visible, is_visible is True)

        # 7. Set the Diffuse Image asset on the Global Skylight (IBL) entity.
        diffuse_image_path = os.path.join("LightingPresets", "greenwich_park_02_4k_iblskyboxcm.exr.streamingimage")
        diffuse_image_asset = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetIdByPath", diffuse_image_path, math.Uuid(), False)
        diffuse_image_added = global_skylight_entity.get_set_test(
            0, "Controller|Configuration|Diffuse Image", diffuse_image_asset)
        Report.result(Tests.diffuse_image_set, diffuse_image_added)

        # 8. Set the Specular Image asset on the Global Light (IBL) entity.
        specular_image_path = os.path.join("LightingPresets", "greenwich_park_02_4k_iblskyboxcm.exr.streamingimage")
        specular_image_asset = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetIdByPath", specular_image_path, math.Uuid(), False)
        specular_image_added = global_skylight_entity.get_set_test(
            0, "Controller|Configuration|Specular Image", specular_image_asset)
        Report.result(Tests.specular_image_set, specular_image_added)

        # 9. Delete Global Skylight (IBL) entity.
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntityById", global_skylight_entity.id)
        Report.result(Tests.entity_deleted, len(hydra.find_entity_by_name(global_skylight)) == 0)

        # 10. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, len(hydra.find_entity_by_name(global_skylight)) == 1)

        # 11. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, len(hydra.find_entity_by_name(global_skylight)) == 0)

        # 12. Look for errors.
        helper.wait_for_condition(lambda: error_tracer.has_errors, 1.0)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_GlobalSkylightIBL_AddedToEntity)
