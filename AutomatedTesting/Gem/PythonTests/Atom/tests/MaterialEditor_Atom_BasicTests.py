"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    open_existing_asset = (
        "Existing material document opened successfully.",
        "P0: Failed to open existing material document.")
    close_opened_asset = (
        "Closed existing material document.",
        "P0: Failed to close existing material document.")
    close_all_opened_assets = (
        "Closed all currently opened documents.",
        "P0: Failed to close all currently opened documents.")
    close_all_opened_assets_except_one = (
        "Closed all currently opened documents except for one file.",
        "P0: Failed to close all currently opened documents except for one file.")
    inspector_pane_visible = (
        "Inspector pane is visible, MaterialEditor launch succeeded",
        "P0: Inspector pane not visible, MaterialEditor launch failed")
    changed_material_asset_color = (
        "Material document color property value changed successfully.",
        "P0: Failed to change the color property values of the material document.")
    undo_material_asset_color_change = (
        "Material document color property value change was reverted using undo.",
        "P0: Failed to undo material document color property value.")
    redo_material_asset_color_change = (
        "Material document color property value changed again successfully using redo.",
        "P0: Failed to change material document color property value again using redo.")
    verify_all_documents_are_opened = (
        "Expected material documents are opened.",
        "P0: Failed to verify the expected material documents are opened.")
    viewport_background_selected = (
        "Viewport lighting background successfully selected.",
        "P0: Viewport lighting background was not selected.")
    viewport_background_has_expected_asset = (
        "Viewport lighting background has the expected asset set.",
        "P0: Viewport lighting background did not have the expected asset selected.")
    viewport_background_changed = (
        "Viewport lighting background changed successfully.",
        "P0: Viewport lighting background change failed.")
    viewport_background_has_changed_asset = (
        "Viewport lighting background successfully changed assets.",
        "P0: Viewport lighting background did not change assets.")
    viewport_model_selected = (
        "Viewport model successfully selected.",
        "P0: Viewport model was not selected.")
    viewport_model_has_expected_asset = (
        "Viewport model has the expected asset set.",
        "P0: Viewport model did not have the expected asset selected")
    viewport_model_changed = (
        "Viewport model changed successfully",
        "P0: Viewport model change failed.")
    viewport_model_has_changed_asset = (
        "Viewport model successfully changed.",
        "P0: Viewport model did not change assets.")
    grid_disabled = (
        "Grid disabled successfully.",
        "P0: Grid failed to disable.")
    grid_enabled = (
        "Grid enabled successfully.",
        "P0: Grid failed to enable.")
    shadow_catcher_disabled = (
        "Shadow Catcher disabled successfully.",
        "P0: Shadow Catcher failed to disable.")
    shadow_catcher_enabled = (
        "Shadow Catcher enabled successfully",
        "P0: Shadow Catcher failed to enable.")


def MaterialEditor_BasicFunctionalityChecks_AllChecksPass():
    """
    Summary:
    Tests basic MaterialEditor functionality but does not deal with file I/O (i.e. saving).

    Expected Behavior:
    All MaterialEditor basic tests pass.

    Test Steps:
    1) Open an existing material document.
    2) Close the selected material document.
    3) Open multiple material documents hen use the CloseAllDocuments bus call to close them all.
    4) Open multiple material documents then verify all material documents are opened.
    5) Use the CloseAllDocumentsExcept bus call to close all but one.
    6) Verify Material Asset Browser pane visibility.
    7) Change the baseColor.color property of the test_material_1 material document.
    8) Revert the baseColor.color property of the test_material_1 material document using undo.
    9) Use redo to change the baseColor.color property again.
    10) Select the lighting background and verify the selection succeeded.
    11) Verify the lighting background asset is the expected value.
    12) Change the lighting background and verify the change succeeded.
    13) Verify the lighting background asset is changed to a new expected value.
    14) Select the model and verify the selection  succeeded.
    15) Verify the model asset is the expected value.
    16) Change the model asset and verify the change succeeded.
    17) Verify the model asset is changed to a new expected value.
    18) Disable the Grid.
    19) Enable the Grid.
    20) Disable the Shadow Catcher.
    21) Enable the Shadow Catcher.
    22) Look for errors and asserts.

    :return: None
    """
    import os

    import azlmbr.math
    import azlmbr.paths

    import Atom.atom_utils.atom_tools_utils as atom_tools_utils
    import Atom.atom_utils.material_editor_utils as material_editor_utils
    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    with Tracer() as error_tracer:
        # Disable registry settings to prevent message boxes from blocking test progression.
        atom_tools_utils.disable_document_message_box_settings()

        # Set constants before starting test steps.
        standard_pbr_material_type = os.path.join(atom_tools_utils.MATERIAL_TYPES_PATH, "StandardPBR.materialtype")
        standard_pbr_test_cases = os.path.join(atom_tools_utils.TEST_DATA_MATERIALS_PATH, "StandardPbrTestCases")
        test_material_1 = os.path.join(standard_pbr_test_cases, "001_DefaultWhite.material")
        test_material_2 = os.path.join(standard_pbr_test_cases, "002_BaseColorLerp.material")
        test_material_3 = os.path.join(standard_pbr_test_cases, "003_MetalMatte.material")
        test_material_4 = os.path.join(standard_pbr_test_cases, "004_MetalMap.material")
        test_material_5 = os.path.join(standard_pbr_test_cases, "005_RoughnessMap.material")
        test_material_6 = os.path.join(standard_pbr_test_cases, "006_SpecularF0Map.material")
        test_materials = [
            test_material_1, test_material_2, test_material_3, test_material_4, test_material_5, test_material_6]

        # 1. Open an existing material document.
        material_document_id = atom_tools_utils.open_document(standard_pbr_material_type)
        Report.result(
            Tests.open_existing_asset,
            atom_tools_utils.is_document_open(material_document_id) is True)

        # 2. Close the selected material document.
        atom_tools_utils.close_document(material_document_id)
        Report.result(
            Tests.close_opened_asset,
            atom_tools_utils.is_document_open(material_document_id) is False)

        # 3. Open multiple material documents then use the CloseAllDocuments bus call to close them all.
        for material in [test_material_1, test_material_2, test_material_3]:
            atom_tools_utils.open_document(material)
        Report.result(
            Tests.close_all_opened_assets,
            atom_tools_utils.close_all_documents() is True)

        # 4. Open multiple material documents then verify all material documents are opened.
        material_document_ids = []
        for material in test_materials:
            material_document_id = atom_tools_utils.open_document(material)
            Report.result(
                Tests.verify_all_documents_are_opened,
                atom_tools_utils.is_document_open(material_document_id) is True)
            material_document_ids.append(material_document_id)

        # 5. Use the CloseAllDocumentsExcept bus call to close all but one.
        atom_tools_utils.close_all_except_selected(material_document_ids[0])
        Report.result(
            Tests.close_all_opened_assets_except_one,
            atom_tools_utils.verify_one_material_document_opened(material_document_ids, 0) is True)

        # 6. Verify Material Asset Browser pane visibility.
        atom_tools_utils.set_pane_visibility("Asset Browser", True)
        Report.result(
            Tests.inspector_pane_visible,
            atom_tools_utils.is_pane_visible("Asset Browser") is True)

        # 7. Change the baseColor.color property of the test_material_1 material document.
        base_color_property_name = "baseColor.color"
        document_id = atom_tools_utils.open_document(test_material_1)
        initial_color = material_editor_utils.get_property(document_id, base_color_property_name)
        expected_color = azlmbr.math.Color(0.25, 0.25, 0.25, 1.0)
        atom_tools_utils.begin_edit(document_id)
        material_editor_utils.set_property(document_id, base_color_property_name, expected_color)
        atom_tools_utils.end_edit(document_id)
        Report.result(
            Tests.changed_material_asset_color,
            material_editor_utils.get_property(document_id, base_color_property_name) == expected_color)

        # 8. Revert the baseColor.color property of the test_material_1 material document using undo.
        atom_tools_utils.undo(document_id)
        Report.result(
            Tests.undo_material_asset_color_change,
            material_editor_utils.get_property(document_id, base_color_property_name) == initial_color)

        # 9. Use redo to change the baseColor.color property again.
        atom_tools_utils.redo(document_id)
        Report.result(
            Tests.redo_material_asset_color_change,
            material_editor_utils.get_property(document_id, base_color_property_name) == expected_color)

        # 10. Select the lighting background and verify the selection succeeded.
        neutral_urban_background_path = os.path.join(
            atom_tools_utils.VIEWPORT_LIGHTING_PRESETS_PATH,
            "neutral_urban.lightingpreset.azasset").replace(os.sep, '/')
        neutral_urban_background_loaded = atom_tools_utils.load_lighting_preset_by_path(neutral_urban_background_path)
        Report.result(Tests.viewport_background_selected, neutral_urban_background_loaded is True)

        # 11. Verify the lighting background asset is the expected value.
        Report.result(
            Tests.viewport_background_has_expected_asset,
            atom_tools_utils.get_last_lighting_preset_path().replace(os.sep, '/') == neutral_urban_background_path)

        # 12. Change the lighting background again and verify the change succeeded.
        lythwood_room_background_path = os.path.join(
            atom_tools_utils.VIEWPORT_LIGHTING_PRESETS_PATH,
            "lythwood_room.lightingpreset.azasset").replace(os.sep, '/')
        lythwood_room_asset_loaded = atom_tools_utils.load_lighting_preset_by_path(lythwood_room_background_path)
        Report.result(Tests.viewport_background_changed, lythwood_room_asset_loaded is True)

        # 13. Verify the lighting background asset is changed to a new expected value.
        Report.result(
            Tests.viewport_background_has_changed_asset,
            atom_tools_utils.get_last_lighting_preset_path().replace(os.sep, '/') == lythwood_room_background_path)

        # 14. Select the model and verify the selection  succeeded.
        beveled_cone_model_asset_path = os.path.join(
            atom_tools_utils.VIEWPORT_MODELS_PRESETS_PATH, "BeveledCone.modelpreset.azasset").replace(os.sep, '/')
        beveled_cone_model_asset = atom_tools_utils.load_model_preset_by_path(beveled_cone_model_asset_path)
        Report.result(Tests.viewport_model_selected, beveled_cone_model_asset is True)

        # 15. Verify the model asset is the expected value.
        Report.result(
            Tests.viewport_model_has_expected_asset,
            atom_tools_utils.get_last_model_preset_path().replace(os.sep, '/') == beveled_cone_model_asset_path)

        # 16. Change the model asset and verify the change succeeded.
        cone_model_asset_path = os.path.join(
            atom_tools_utils.VIEWPORT_MODELS_PRESETS_PATH, "Cone.modelpreset.azasset").replace(os.sep, '/')
        cone_model_asset = atom_tools_utils.load_model_preset_by_path(cone_model_asset_path)
        Report.result(Tests.viewport_model_changed, cone_model_asset is True)

        # 17. Verify the model asset is changed to a new expected value.
        Report.result(
            Tests.viewport_model_has_changed_asset,
            atom_tools_utils.get_last_model_preset_path().replace(os.sep, '/') == cone_model_asset_path)

        # 18. Disable the Grid.
        atom_tools_utils.set_grid_enabled(False)
        Report.result(Tests.grid_disabled, atom_tools_utils.get_grid_enabled() is False)

        # 19. Enable the Grid.
        atom_tools_utils.set_grid_enabled(True)
        Report.result(Tests.grid_enabled, atom_tools_utils.get_grid_enabled() is True)

        # 20. Disable the Shadow Catcher.
        atom_tools_utils.set_shadow_catcher_enabled(False)
        Report.result(Tests.shadow_catcher_disabled, atom_tools_utils.get_shadow_catcher_enabled() is False)

        # 21. Enable the Shadow Catcher.
        atom_tools_utils.set_shadow_catcher_enabled(True)
        Report.result(Tests.shadow_catcher_enabled, atom_tools_utils.get_shadow_catcher_enabled() is True)

        # 22. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MaterialEditor_BasicFunctionalityChecks_AllChecksPass)
