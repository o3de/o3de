"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    camera_creation = (
        "Camera Entity successfully created",
        "Camera Entity failed to be created")
    camera_component_added = (
        "Camera component was added to entity",
        "Camera component failed to be added to entity")
    camera_component_check = (
        "Entity has a Camera component",
        "Entity failed to find Camera component")
    creation_undo = (
        "UNDO Entity creation success",
        "UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "REDO Entity creation failed")
    global_skylight_creation = (
        "Global Skylight (IBL) Entity successfully created",
        "Global Skylight (IBL) Entity failed to be created")
    global_skylight_component = (
        "Entity has a Global Skylight (IBL) component",
        "Entity failed to find Global Skylight (IBL) component")
    diffuse_image_set = (
        "Entity has the Diffuse Image set",
        "Entity did not the Diffuse Image set")
    specular_image_set = (
        "Entity has the Specular Image set",
        "Entity did not the Specular Image set")
    enter_game_mode = (
        "Entered game mode",
        "Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "REDO deletion failed")


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
    2) Add Global Skylight (IBL) component to Global Skylight (IBL) entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Enter/Exit game mode.
    6) Test IsHidden.
    7) Test IsVisible.
    8) Add Post FX Layer component.
    9) Add Camera component
    10) Delete Global Skylight (IBL) entity.
    11) UNDO deletion.
    12) REDO deletion.
    13) Look for errors.

    :return: None
    """
    import os

    import azlmbr.legacy.general as general

    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Test steps begin.
        # 1. Create a Global Skylight (IBL) entity with no components.
        global_skylight_entity = EditorEntity.create_editor_entity(AtomComponentProperties.global_skylight())
        Report.critical_result(Tests.global_skylight_creation, global_skylight_entity.exists())

        # 2. Add Global Skylight (IBL) component to Global Skylight (IBL) entity.
        global_skylight_component = global_skylight_entity.add_component(AtomComponentProperties.global_skylight())
        Report.critical_result(
            Tests.global_skylight_component,
            global_skylight_entity.has_component(AtomComponentProperties.global_skylight()))

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
        Report.result(Tests.creation_undo, not global_skylight_entity.exists())

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
        Report.result(Tests.creation_redo, global_skylight_entity.exists())

        # 5. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 6. Test IsHidden.
        global_skylight_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, global_skylight_entity.is_hidden() is True)

        # 7. Test IsVisible.
        global_skylight_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, global_skylight_entity.is_visible() is True)

        # 8. Set the Diffuse Image asset on the Global Skylight (IBL) entity.
        diffuse_image_path = os.path.join("LightingPresets", "greenwich_park_02_4k_iblskyboxcm.exr.streamingimage")
        diffuse_image_asset = Asset.find_asset_by_path(diffuse_image_path, False)
        global_skylight_component.set_component_property_value(
            AtomComponentProperties.global_skylight('Diffuse Image'), diffuse_image_asset.id)
        Report.result(
            Tests.diffuse_image_set,
            diffuse_image_asset.id == global_skylight_component.get_component_property_value(
                                          AtomComponentProperties.global_skylight('Diffuse Image')))

        # 9. Set the Specular Image asset on the Global Light (IBL) entity.
        specular_image_path = os.path.join("LightingPresets", "greenwich_park_02_4k_iblskyboxcm.exr.streamingimage")
        specular_image_asset = Asset.find_asset_by_path(specular_image_path, False)
        global_skylight_component.set_component_property_value(
            AtomComponentProperties.global_skylight('Specular Image'), specular_image_asset.id)
        Report.result(
            Tests.specular_image_set,
            specular_image_asset.id == global_skylight_component.get_component_property_value(
                                           AtomComponentProperties.global_skylight('Specular Image')))

        # 10. Delete Global Skylight (IBL) entity.
        global_skylight_entity.delete()
        Report.result(Tests.entity_deleted, not global_skylight_entity.exists())

        # 11. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, global_skylight_entity.exists())

        # 12. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not  global_skylight_entity.exists())

        # 13. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_GlobalSkylightIBL_AddedToEntity)
