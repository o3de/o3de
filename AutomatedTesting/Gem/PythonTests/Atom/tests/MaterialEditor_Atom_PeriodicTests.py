"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    open_existing_material_document = (
        "Existing material document opened successfully.",
        "P0: Failed to open existing material document.")
    close_opened_material_document = (
        "Closed existing material document.",
        "P0: Failed to close existing material document.")
    close_all_opened_material_documents = (
        "Closed all currently opened material documents.",
        "P0: Failed to close all currently opened material documents.")
    changed_material_document_color = (
        "Material document color property value changed successfully.",
        "P0: Failed to change the color property values of the material document.")
    changed_material_document_metallic_factor = (
        "Material document metallic factor property value changed successfully.",
        "P0: Failed to change the metallic factor property of the material document.")
    saved_material_document = (
        "Material document was saved successfully.",
        "P0: Material document could not be saved.")
    verify_all_material_documents_are_opened = (
        "Expected material documents are opened.",
        "P0: Failed to verify the expected material documents are opened.")
    verify_file_changes_saved = (
        "Material document has expected changes saved.",
        "P0: Failed to find expected changes saved for material document.")
    reverted_original_material_document_color = (
        "Material document color reverted to original value before saving.",
        "P0: Failed to revert 'original_material_document material' document color to original value.")


def MaterialEditor_FileSaveChecks_AllChecksPass():
    """
    Summary:
    Tests basic MaterialEditor functionality that deals with file I/O writes (i.e. saving materials).

    Expected Behavior:
    All MaterialEditor tests dealing with file saving all pass without issues.

    Test Steps:
    1) Open an existing material document referred to as "original_material_document".
    2) Change the baseColor.color property value of original_material_document to 0.25, 0.25, 0.25, 1.0.
    3) Save over original_material_document.
    4) Change the baseColor.color property value of original_material_document to 0.5, 0.5, 0.5, 1.0.
    5) Save a new material document from original_material_document referred to as "new_material_document".
    6) Change the baseColor.color property value of original_material_document to 0.75, 0.75, 0.75, 1.0.
    7) Save a new child material document referred to as "child_material_document".
    8) Close & open previously saved original_material_document, new_material_document, & child_material_document.
    9) Verify the changes are saved in original_material_document.
    10) Verify the changes are saved in new_material_document.
    11) Verify the changes are saved in child_material_document.
    12) Revert changes to original_material_document and save them.
    13) Close all currently opened documents.
    14) Open a material document referred to as "first_material_document".
    15) Change the metallic.factor property value of first_material_document to 0.444.
    16) Save the first_material_document as a new material document file.
    17) Open a second material document referred to as "second_material_document".
    18) Change the baseColor.color property value of the second_material_document to 0.4156, 0.0196, 0.6862, 1.0.
    19) Save the second_material_document as a new material document file.
    20) Close the first_material_document and second_material_document.
    21) Open the first_material_document & verify the changes are saved.
    22) Open the second_material_document & verify the changes are saved.
    23) Revert changes to first_material_document and second_material_document then save.
    24) Close all currently opened documents.
    25) Look for errors and asserts.

    :return: None
    """
    import os

    import azlmbr.math
    import azlmbr.paths

    import Atom.atom_utils.atom_tools_utils as atom_tools_utils
    import Atom.atom_utils.material_editor_utils as material_editor_utils
    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    with Tracer() as error_tracer:

        # Set constants before starting test steps.
        base_color_property_name = "baseColor.color"
        metallic_factor_property_name = "metallic.factor"
        original_material_document = os.path.join(
            atom_tools_utils.TEST_DATA_MATERIALS_PATH, "StandardPbrTestCases", "001_DefaultWhite.material")
        new_material_document = os.path.join(
            atom_tools_utils.TEST_DATA_MATERIALS_PATH, "new_material_document.material")
        child_material_document = os.path.join(
            atom_tools_utils.TEST_DATA_MATERIALS_PATH, "child_material_document.material")
        first_material_document = os.path.join(
            atom_tools_utils.TEST_DATA_MATERIALS_PATH, "StandardPbrTestCases", "003_MetalMatte.material")
        second_material_document = os.path.join(
            atom_tools_utils.TEST_DATA_MATERIALS_PATH, "StandardPbrTestCases", "004_MetalMap.material")

        # 1. Open an existing material document referred to as "original_material_document".
        original_material_document_id = atom_tools_utils.open_document(original_material_document)
        starting_material_document_color = material_editor_utils.get_property(
            original_material_document_id, base_color_property_name)
        Report.result(
            Tests.open_existing_material_document,
            atom_tools_utils.is_document_open(original_material_document_id) is True)

        # 2. Change the baseColor.color property value of original_material_document to 0.25, 0.25, 0.25, 1.0.
        original_material_document_color = azlmbr.math.Color(0.25, 0.25, 0.25, 1.0)
        material_editor_utils.set_property(
            original_material_document_id, base_color_property_name, original_material_document_color)
        Report.result(
            Tests.changed_material_document_color,
            material_editor_utils.get_property(
                original_material_document_id, base_color_property_name) == original_material_document_color)

        # 3. Save over original_material_document.
        original_material_document_saved = atom_tools_utils.save_document(original_material_document_id)
        Report.result(
            Tests.saved_material_document,
            original_material_document_saved is True)

        # 4. Change the baseColor.color property value of original_material_document to 0.5, 0.5, 0.5, 1.0.
        new_material_document_color = azlmbr.math.Color(0.5, 0.5, 0.5, 1.0)
        material_editor_utils.set_property(
            original_material_document_id, base_color_property_name, new_material_document_color)
        Report.result(
            Tests.changed_material_document_color,
            material_editor_utils.get_property(
                original_material_document_id, base_color_property_name) == new_material_document_color)

        # 5. Save a new material document from original_material_document referred to as "new_material_document".
        new_material_document_saved = atom_tools_utils.save_document_as_copy(
            original_material_document_id, new_material_document)
        Report.result(
            Tests.saved_material_document,
            new_material_document_saved is True)

        # 6. Change the baseColor.color property value of original_material_document to 0.75, 0.75, 0.75, 1.0.
        child_material_document_color = azlmbr.math.Color(0.75, 0.75, 0.75, 1.0)
        material_editor_utils.set_property(
            original_material_document_id, base_color_property_name, child_material_document_color)
        Report.result(
            Tests.changed_material_document_color,
            material_editor_utils.get_property(
                original_material_document_id, base_color_property_name) == child_material_document_color)

        # 7. Save a new child material document referred to as "child_material_document".
        child_material_document_saved = atom_tools_utils.save_document_as_child(
            original_material_document_id, child_material_document)
        Report.result(
            Tests.saved_material_document,
            child_material_document_saved is True)

        # 8. Close & open previously saved original_material_document, new_material_document, & child_material_document.
        closed_all_material_documents = atom_tools_utils.close_all_documents()
        Report.result(
            Tests.close_all_opened_material_documents,
            closed_all_material_documents is True)

        original_material_document_id = atom_tools_utils.open_document(original_material_document)
        new_material_document_id = atom_tools_utils.open_document(new_material_document)
        child_material_document_id = atom_tools_utils.open_document(child_material_document)
        material_document_ids = [original_material_document_id, new_material_document_id, child_material_document_id]
        for material_document_id in material_document_ids:
            Report.result(
                Tests.verify_all_material_documents_are_opened,
                atom_tools_utils.is_document_open(material_document_id) is True)

        # 9. Verify the changes are saved in original_material_document.
        Report.result(
            Tests.verify_file_changes_saved,
            material_editor_utils.get_property(
                original_material_document_id, base_color_property_name) == original_material_document_color)

        # 10. Verify the changes are saved in new_material_document.
        Report.result(
            Tests.verify_file_changes_saved,
            material_editor_utils.get_property(
                new_material_document_id, base_color_property_name) == new_material_document_color)

        # 11. Verify the changes are saved in child_material_document.
        Report.result(
            Tests.verify_file_changes_saved,
            material_editor_utils.get_property(
                child_material_document_id, base_color_property_name) == child_material_document_color)

        # 12. Revert original_material_document color to starting_material_document_color and save it.
        material_editor_utils.set_property(
            original_material_document_id, base_color_property_name, starting_material_document_color)
        Report.result(
            Tests.reverted_original_material_document_color,
            material_editor_utils.get_property(
                original_material_document_id, base_color_property_name) == starting_material_document_color)
        original_document_saved = atom_tools_utils.save_document(original_material_document_id)
        Report.result(
            Tests.saved_material_document,
            original_document_saved is True)

        # 13. Close all currently opened documents.
        closed_all_material_documents = atom_tools_utils.close_all_documents()
        Report.result(
            Tests.close_all_opened_material_documents,
            closed_all_material_documents is True)
        for material_document_id in material_document_ids:
            Report.result(
                Tests.verify_all_material_documents_are_opened,
                atom_tools_utils.is_document_open(material_document_id) is False)

        # 14. Open a material document referred to as "first_material_document".
        first_material_document_id = atom_tools_utils.open_document(first_material_document)
        Report.result(
            Tests.open_existing_material_document,
            atom_tools_utils.is_document_open(first_material_document_id) is True)

        # 15. Change the metallic.factor property value of first_material_document to 0.444.
        starting_first_material_document_metallic_factor = material_editor_utils.get_property(
            first_material_document_id, metallic_factor_property_name)
        first_material_document_metallic_factor = 0.444
        material_editor_utils.set_property(
            first_material_document_id, metallic_factor_property_name, first_material_document_metallic_factor)
        Report.result(
            Tests.changed_material_document_metallic_factor,
            material_editor_utils.get_property(
                first_material_document_id, metallic_factor_property_name) == first_material_document_metallic_factor)

        # 16. Save the first_material_document as a new material document file.
        first_material_document_saved = atom_tools_utils.save_document(first_material_document_id)
        Report.result(
            Tests.saved_material_document,
            first_material_document_saved is True)

        # 17. Open a second material document referred to as "second_material_document".
        second_material_document_id = atom_tools_utils.open_document(second_material_document)
        Report.result(
            Tests.open_existing_material_document,
            atom_tools_utils.is_document_open(second_material_document_id) is True)

        # 18. Change the baseColor.color property value of the second_material_document to 0.4156, 0.0196, 0.6862, 1.0.
        starting_second_material_document_color = material_editor_utils.get_property(
            second_material_document_id, base_color_property_name)
        second_material_document_color = azlmbr.math.Color(0.4156, 0.0196, 0.6862, 1.0)
        material_editor_utils.set_property(
            second_material_document_id, base_color_property_name, second_material_document_color)
        Report.result(
            Tests.changed_material_document_color,
            material_editor_utils.get_property(
                second_material_document_id, base_color_property_name) == second_material_document_color)

        # 19. Save the second_material_document as a new material document file.
        second_material_document_saved = atom_tools_utils.save_document(second_material_document_id)
        Report.result(
            Tests.saved_material_document,
            second_material_document_saved is True)

        # 20. Close the first_material_document and second_material_document.
        first_material_document_closed = atom_tools_utils.close_document(first_material_document_id)
        Report.result(
            Tests.close_opened_material_document,
            first_material_document_closed is True)
        second_material_document_closed = atom_tools_utils.close_document(second_material_document_id)
        Report.result(
            Tests.close_opened_material_document,
            second_material_document_closed is True)

        # 21. Open the first_material_document & verify the changes are saved.
        first_material_document_id = atom_tools_utils.open_document(first_material_document)
        Report.result(
            Tests.changed_material_document_metallic_factor,
            material_editor_utils.get_property(
                first_material_document_id, metallic_factor_property_name) == first_material_document_metallic_factor)

        # 22. Open the second_material_document & verify the changes are saved.
        second_material_document_id = atom_tools_utils.open_document(second_material_document)
        Report.result(
            Tests.changed_material_document_color,
            material_editor_utils.get_property(
                second_material_document_id, base_color_property_name) == second_material_document_color)

        # 23. Revert changes to first_material_document and second_material_document then save.
        material_editor_utils.set_property(
            first_material_document_id, metallic_factor_property_name, starting_first_material_document_metallic_factor)
        Report.result(
            Tests.changed_material_document_metallic_factor,
            material_editor_utils.get_property(
                first_material_document_id,
                metallic_factor_property_name) == starting_first_material_document_metallic_factor)
        material_editor_utils.set_property(
            second_material_document_id, base_color_property_name, starting_second_material_document_color)
        Report.result(
            Tests.changed_material_document_color,
            material_editor_utils.get_property(
                second_material_document_id,
                metallic_factor_property_name) == starting_second_material_document_color)

        first_material_document_resaved = atom_tools_utils.save_document(first_material_document_id)
        Report.result(
            Tests.saved_material_document,
            first_material_document_resaved is True)
        second_material_document_resaved = atom_tools_utils.save_document(second_material_document_id)
        Report.result(
            Tests.saved_material_document,
            second_material_document_resaved is True)

        # 24. Close all currently opened documents.
        closed_all_material_documents = atom_tools_utils.close_all_documents()
        Report.result(
            Tests.close_all_opened_material_documents,
            closed_all_material_documents is True)
        Report.result(
            Tests.close_opened_material_document,
            atom_tools_utils.is_document_open(first_material_document_id) is False)
        Report.result(
            Tests.close_opened_material_document,
            atom_tools_utils.is_document_open(second_material_document_id) is False)

        # 25. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MaterialEditor_FileSaveChecks_AllChecksPass)
