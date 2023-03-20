"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    creation_undo = (
        "UNDO Entity creation success",
        "UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "REDO Entity creation failed")
    hdri_skybox_entity_creation = (
        "HDRi Skybox successfully created",
        "HDRi Skybox failed to be created")
    hdri_skybox_component = (
        "Entity has an HDRi Skybox component",
        "Entity failed to find HDRi Skybox component")
    cubemap_property_set = (
        "Cubemap property set on HDRi Skybox component",
        "Couldn't set Cubemap property on HDRi Skybox component")
    exposure_set_to_negative_five = (
        "Exposure set to -5.0",
        "Exposure could not be set to -5.0")
    exposure_set_to_default = (
        "Exposure set to 0",
        "Exposure could not be set to 0")
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


def AtomEditorComponents_HDRiSkybox_AddedToEntity():
    """
    Summary:
    Tests the HDRi Skybox component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an HDRi Skybox with no components.
    2) Add an HDRi Skybox component to HDRi Skybox.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Set Cubemap Texture on HDRi Skybox component.
    6) Set Exposure value to -5.0.
    7) Set Exposure value back to default, 0.0.
    8) Enter/Exit game mode.
    9) Test IsHidden.
    10) Test IsVisible.
    11) Delete HDRi Skybox.
    12) UNDO deletion.
    13) REDO deletion.
    14) Look for errors.

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
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create an HDRi Skybox with no components.
        hdri_skybox_entity = EditorEntity.create_editor_entity(
            AtomComponentProperties.hdri_skybox())
        Report.critical_result(Tests.hdri_skybox_entity_creation, 
                               hdri_skybox_entity.exists())

        # 2. Add an HDRi Skybox component to HDRi Skybox.
        hdri_skybox_component = hdri_skybox_entity.add_component(
            AtomComponentProperties.hdri_skybox())
        Report.critical_result(
            Tests.hdri_skybox_component,
            hdri_skybox_entity.has_component(AtomComponentProperties.hdri_skybox()))

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
        Report.result(Tests.creation_undo, not hdri_skybox_entity.exists())

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
        Report.result(Tests.creation_redo, hdri_skybox_entity.exists())

        # 5. Set Cubemap Texture on HDRi Skybox component.
        skybox_cubemap_asset_path = os.path.join("lightingpresets", "default_iblskyboxcm.exr.streamingimage")
        skybox_cubemap_material_asset = Asset.find_asset_by_path(skybox_cubemap_asset_path, False)
        hdri_skybox_component.set_component_property_value(
            AtomComponentProperties.hdri_skybox('Cubemap Texture'), skybox_cubemap_material_asset.id)
        get_cubemap_property = hdri_skybox_component.get_component_property_value(
            AtomComponentProperties.hdri_skybox('Cubemap Texture'))
        Report.result(Tests.cubemap_property_set, get_cubemap_property == skybox_cubemap_material_asset.id)

        # 6. Set Exposure value to -5.0.
        hdri_skybox_component.set_component_property_value(
            AtomComponentProperties.hdri_skybox('Exposure'), -5.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.exposure_set_to_negative_five,
            hdri_skybox_component.get_component_property_value(
                AtomComponentProperties.hdri_skybox('Exposure')) == -5.0)

        # 7. Set Exposure value back to default, 0.0.
        hdri_skybox_component.set_component_property_value(
            AtomComponentProperties.hdri_skybox('Exposure'), 0.0)
        general.idle_wait_frames(1)
        Report.result(
            Tests.exposure_set_to_default,
            hdri_skybox_component.get_component_property_value(
                AtomComponentProperties.hdri_skybox('Exposure')) == 0.0)

        # 8. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 9. Test IsHidden.
        hdri_skybox_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, hdri_skybox_entity.is_hidden() is True)

        # 10. Test IsVisible.
        hdri_skybox_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, hdri_skybox_entity.is_visible() is True)

        # 11. Delete hdri_skybox entity.
        hdri_skybox_entity.delete()
        Report.result(Tests.entity_deleted, not hdri_skybox_entity.exists())

        # 12. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, hdri_skybox_entity.exists())

        # 13. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not hdri_skybox_entity.exists())

        # 14. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_HDRiSkybox_AddedToEntity)
