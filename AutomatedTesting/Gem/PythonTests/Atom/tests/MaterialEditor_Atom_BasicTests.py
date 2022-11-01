"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    open_existing_asset = (
        "Existing material asset file opened successfully.",
        "P0: Failed to open existing material asset file.")
    close_opened_asset = (
        "Closed existing material asset file.",
        "P0: Failed to close existing material asset file.")
    close_all_opened_assets = (
        "Closed all currently opened asset document files.",
        "P0: Failed to close all currently opened asset document files.")
    close_all_opened_assets_except_one = (
        "Closed all currently opened asset document files except for one file.",
        "P0: Failed to close all currently opened asset document files except for one file.")
    inspector_pane_visible = (
        "Inspector pane is visible, MaterialEditor launch succeeded",
        "P0: Inspector pane not visible, MaterialEditor launch failed")
    changed_material_asset_color = (
        "Material asset color changed successfully.",
        "P0: Failed to change the color values of a material asset file.")
    undo_material_asset_color_change = (
        "Material asset color change was reverted using undo.",
        "P0: Failed to undo material asset color change.")
    redo_material_asset_color_change = (
        "Material asset color changed again successfully using redo.",
        "P0: Failed to change material asset color again using redo.")


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
    4) Open multiple material documents then use the CloseAllDocumentsExcept bus call to close all but one.
    5) Verify Material Asset Browser pane visibility.
    6) Change the baseColor.color property of the test_material_1 material document.
    7) Revert the baseColor.color property of the test_material_1 material document using undo.
    8) Use redo to change the baseColor.color property again.
    9) Look for errors and asserts.

    :return: None
    """
    import os

    import azlmbr.math as math
    import azlmbr.paths

    import Atom.atom_utils.atom_tools_utils as atom_tools_utils
    import Atom.atom_utils.material_editor_utils as material_editor_utils
    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    with Tracer() as error_tracer:
        # Set constants before starting test steps.
        material_type_path = os.path.join(
            azlmbr.paths.engroot, "Gems", "Atom", "Feature", "Common", "Assets", "Materials", "Types",
            "StandardPBR.materialtype")
        test_data_path = os.path.join(
            azlmbr.paths.engroot, "Gems", "Atom", "TestData", "TestData", "Materials", "StandardPbrTestCases")
        test_material_1 = "001_DefaultWhite.material"
        test_material_2 = "002_BaseColorLerp.material"
        test_material_3 = "003_MetalMatte.material"

        # 1. Open an existing material document.
        material_document_id = atom_tools_utils.open_document(material_type_path)
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
            atom_tools_utils.open_document(os.path.join(test_data_path, material))
        Report.result(
            Tests.close_all_opened_assets,
            atom_tools_utils.close_all_documents() is True)

        # 4. Open multiple material documents then use the CloseAllDocumentsExcept bus call to close all but one.
        test_material_1_document_id = atom_tools_utils.open_document(os.path.join(test_data_path, test_material_1))
        test_material_2_document_id = atom_tools_utils.open_document(os.path.join(test_data_path, test_material_2))
        test_material_3_document_id = atom_tools_utils.open_document(os.path.join(test_data_path, test_material_3))
        atom_tools_utils.close_all_except_selected(test_material_1_document_id)
        Report.result(
            Tests.close_all_opened_assets_except_one,
            atom_tools_utils.is_document_open(test_material_1_document_id) is True and
            atom_tools_utils.is_document_open(test_material_2_document_id) is False and
            atom_tools_utils.is_document_open(test_material_3_document_id) is False)

        # 5. Verify Material Asset Browser pane visibility.
        atom_tools_utils.set_pane_visibility("Asset Browser", True)
        Report.result(
            Tests.inspector_pane_visible,
            atom_tools_utils.is_pane_visible("Asset Browser") is True)

        # 6. Change the baseColor.color property of the test_material_1 material document.
        base_color_property_name = "baseColor.color"
        document_id = atom_tools_utils.open_document(os.path.join(test_data_path, test_material_1))
        initial_color = material_editor_utils.get_property(document_id, base_color_property_name)
        expected_color = math.Color(0.25, 0.25, 0.25, 1.0)
        atom_tools_utils.begin_edit(document_id)
        material_editor_utils.set_property(document_id, base_color_property_name, expected_color)
        atom_tools_utils.end_edit(document_id)
        Report.result(
            Tests.changed_material_asset_color,
            material_editor_utils.get_property(document_id, base_color_property_name) == expected_color)

        # 7. Revert the baseColor.color property of the test_material_1 material document using undo.
        atom_tools_utils.undo(document_id)
        Report.result(
            Tests.undo_material_asset_color_change,
            material_editor_utils.get_property(document_id, base_color_property_name) == initial_color)

        # 8. Use redo to change the baseColor.color property again.
        atom_tools_utils.redo(document_id)
        Report.result(
            Tests.redo_material_asset_color_change,
            material_editor_utils.get_property(document_id, base_color_property_name) == expected_color)

        # 9. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MaterialEditor_BasicFunctionalityChecks_AllChecksPass)
