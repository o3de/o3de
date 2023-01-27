"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    changed_original_material_document_color = (
        "original_material_document_color baseColor.color property value was changed successfully.",
        "P0: original_material_document_color failed to change baseColor.color property value.")
    changed_new_material_document_color = (
        "new_material_document_color baseColor.color property value was changed successfully.",
        "P0: new_material_document_color failed to change baseColor.color property value.")
    changed_child_material_document_color = (
        "child_material_document baseColor.color property value was changed successfully.",
        "P0: child_material_document failed to change baseColor.color property value.")
    changed_first_material_document_color = (
        "first_material_document color factor property value changed successfully.",
        "P0: Failed to change the first_material_document color factor property.")
    changed_second_material_document_color = (
        "second_material_document baseColor.color property value was changed successfully.",
        "P0: second_material_document failed to change baseColor.color property value.")
    closed_original_new_and_child_material_documents = (
        "Closed original_material_document, new_material_document, & child_material_document successfully.",
        "P0: Failed to close original_material_document, new_material_document, & child_material_document.")
    closed_first_material_document = (
        "Closed first_material_document successfully.",
        "P0: Failed to close first_material_document.")
    closed_second_material_document = (
        "Closed second_material_document successfully.",
        "P0: Failed to close second_material_document.")
    closed_first_and_second_material_documents = (
        "Closed first_material_document & second_material_document successfully.",
        "P0: Failed to close first_material_document & second_material_document.")
    opened_original_material_document = (
        "original_material_document opened successfully",
        "P0: Failed to open original_material_document.")
    opened_original_new_child_material_documents = (
        "Opened original_material_document, new_material_document, & child_material_document.",
        "P0: Failed to open original_material_document, new_material_document, & child_material_document.")
    opened_first_material_document = (
        "Opened first_material_document.",
        "P0: Failed to open first_material_document.")
    opened_second_material_document = (
        "Opened second_material_document.",
        "P0: Failed to open second_material_document.")
    reclosed_original_new_child_material_documents = (
        "Re-closed original_material_document, new_material_document, & child_material_document.",
        "P0: Failed to re-close original_material_document, new_material_document, & child_material_document.")
    reclosed_first_material_document = (
        "Re-closed first_material_document.",
        "P0: Failed to re-close first_material_document.")
    reclosed_second_material_document = (
        "Re-closed second material document.",
        "P0: Failed to re-cloe second material document.")
    reopened_first_material_document = (
        "Re-opened first_material_document successfully",
        "P0: Failed to re-open first_material_document.")
    reopened_second_material_document = (
        "Re-opened second_material_document successfully",
        "P0: Failed to re-open second_material_document.")
    resaved_original_material_document = (
        "original_material_document was re-saved successfully.",
        "P0: original_material_document could not be re-saved.")
    resaved_first_material_document = (
        "first_material_document was re-saved successfully.",
        "P0: first_material_document could not be re-saved.")
    resaved_second_material_document = (
        "second_material_document was re-saved successfully.",
        "P0: second_material_document could not be re-saved.")
    reverted_original_material_document_color = (
        "original_material_document baseColor.color property value reverted to original value before changing it.",
        "P0: Failed to revert original_material_document baseColor.color property value to original value.")
    reverted_first_material_document_color = (
        "first_material_document baseColor.color property value reverted to original value before changing it.",
        "P0: Failed to revert first_material_document baseColor.color property value to original value.")
    reverted_second_material_document_color = (
        "second_material_document baseColor.color property value reverted to original value before changing it.",
        "P0: Failed to revert second_material_document baseColor.color property value to original value.")
    saved_original_material_document = (
        "original_material_document was saved successfully.",
        "P0: original_material_document could not be saved.")
    saved_new_material_document = (
        "new_material_document was saved successfully.",
        "P0: new_material_document could not be saved.")
    saved_child_material_document = (
        "child_material_document was saved successfully.",
        "P0: child_material_document could not be saved.")
    saved_changes_to_original_material_document = (
        "Changes saved successfully for original_material_document",
        "P0: original_material_document changes could not be saved.")
    saved_changes_to_new_material_document = (
        "Changes saved successfully for new_material_document",
        "P0: new_material_document changes could not be saved.")
    saved_first_material_document = (
        "Changes saved successfully for first_material_document",
        "P0: first_material_document changes could not be saved.")
    saved_changes_to_child_material_document = (
        "Changes saved successfully for child_material_document",
        "P0: child_material_document changes could not be saved.")
    saved_second_material_document = (
        "Changes saved successfully for second_material_document",
        "P0: second_material_document changes could not be saved.")
    verified_first_material_document_color = (
        "Verified first_material_document property value for baseColor.color is accurate",
        "P0: Unexpected first_material_document property value for baseColor.color returned.")
    verified_second_material_document_color = (
        "Verified second_material_document property value for baseColor.color is accurate",
        "P0: Unexpected second_material_document property value for baseColor.color returned.")


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
    15) Change the baseColor.color property value of first_material_document to 0.4156, 0.0196, 0.6862, 1.0.
    16) Save the first_material_document as a new material document file.
    17) Open a second material document referred to as "second_material_document".
    18) Change the baseColor.color property value of the second_material_document to 0.4156, 0.0196, 0.6862, 1.0.
    19) Save the second_material_document as a new material document file.
    20) Close the first_material_document and second_material_document.
    21) Open the first_material_document & verify the changes are saved.
    22) Open the second_material_document & verify the changes are saved.
    23) Revert changes to first_material_document and second_material_document.
    24) Re-save first_material_document and second_material_document to restore their original values.
    25) Close all currently opened documents.
    26) Look for errors and asserts.

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
            Tests.opened_original_material_document,
            atom_tools_utils.is_document_open(original_material_document_id) is True)

        # 2. Change the baseColor.color property value of original_material_document to 0.25, 0.25, 0.25, 1.0.
        original_material_document_color = azlmbr.math.Color(0.25, 0.25, 0.25, 1.0)
        material_editor_utils.set_property(
            original_material_document_id, base_color_property_name, original_material_document_color)
        Report.result(
            Tests.changed_original_material_document_color,
            material_editor_utils.get_property(
                original_material_document_id, base_color_property_name) == original_material_document_color)

        # 3. Save over original_material_document.
        original_material_document_saved = atom_tools_utils.save_document(original_material_document_id)
        Report.result(
            Tests.saved_original_material_document,
            original_material_document_saved is True)

        # 4. Change the baseColor.color property value of original_material_document to 0.5, 0.5, 0.5, 1.0.
        new_material_document_color = azlmbr.math.Color(0.5, 0.5, 0.5, 1.0)
        material_editor_utils.set_property(
            original_material_document_id, base_color_property_name, new_material_document_color)
        Report.result(
            Tests.changed_new_material_document_color,
            material_editor_utils.get_property(
                original_material_document_id, base_color_property_name) == new_material_document_color)

        # 5. Save a new material document from original_material_document referred to as "new_material_document".
        new_material_document_saved = atom_tools_utils.save_document_as_copy(
            original_material_document_id, new_material_document)
        Report.result(
            Tests.saved_new_material_document,
            new_material_document_saved is True)

        # 6. Change the baseColor.color property value of original_material_document to 0.75, 0.75, 0.75, 1.0.
        child_material_document_color = azlmbr.math.Color(0.75, 0.75, 0.75, 1.0)
        material_editor_utils.set_property(
            original_material_document_id, base_color_property_name, child_material_document_color)
        Report.result(
            Tests.changed_child_material_document_color,
            material_editor_utils.get_property(
                original_material_document_id, base_color_property_name) == child_material_document_color)

        # 7. Save a new child material document referred to as "child_material_document".
        child_material_document_saved = atom_tools_utils.save_document_as_child(
            original_material_document_id, child_material_document)
        Report.result(
            Tests.saved_child_material_document,
            child_material_document_saved is True)

        # 8. Close & open previously saved original_material_document, new_material_document, & child_material_document.
        closed_all_material_documents = atom_tools_utils.close_all_documents()
        Report.result(
            Tests.closed_original_new_and_child_material_documents,
            closed_all_material_documents is True)
        original_material_document_id = atom_tools_utils.open_document(original_material_document)
        new_material_document_id = atom_tools_utils.open_document(new_material_document)
        child_material_document_id = atom_tools_utils.open_document(child_material_document)
        material_document_ids = [original_material_document_id, new_material_document_id, child_material_document_id]
        for material_document_id in material_document_ids:
            Report.result(
                Tests.opened_original_new_child_material_documents,
                atom_tools_utils.is_document_open(material_document_id) is True)

        # 9. Verify the changes are saved in original_material_document.
        Report.result(
            Tests.saved_changes_to_original_material_document,
            material_editor_utils.get_property(
                original_material_document_id, base_color_property_name) == original_material_document_color)

        # 10. Verify the changes are saved in new_material_document.
        Report.result(
            Tests.saved_changes_to_new_material_document,
            material_editor_utils.get_property(
                new_material_document_id, base_color_property_name) == new_material_document_color)

        # 11. Verify the changes are saved in child_material_document.
        Report.result(
            Tests.saved_changes_to_child_material_document,
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
            Tests.resaved_original_material_document,
            original_document_saved is True)

        # 13. Close all currently opened documents.
        closed_all_material_documents = atom_tools_utils.close_all_documents()
        Report.result(
            Tests.reclosed_original_new_child_material_documents,
            closed_all_material_documents is True)
        for material_document_id in material_document_ids:
            Report.result(
                Tests.reclosed_original_new_child_material_documents,
                atom_tools_utils.is_document_open(material_document_id) is False)

        # 14. Open a material document referred to as "first_material_document".
        first_material_document_id = atom_tools_utils.open_document(first_material_document)
        Report.result(
            Tests.opened_first_material_document,
            atom_tools_utils.is_document_open(first_material_document_id) is True)

        # 15. Change the baseColor.color property value of first_material_document to 0.0156, 0.0196, 0.0862, 1.0.
        starting_first_material_document_color = material_editor_utils.get_property(
            first_material_document_id, base_color_property_name)
        first_material_document_color = azlmbr.math.Color(0.0156, 0.0196, 0.0862, 1.0)
        material_editor_utils.set_property(
            first_material_document_id, base_color_property_name, first_material_document_color)
        Report.result(
            Tests.changed_first_material_document_color,
            material_editor_utils.get_property(
                first_material_document_id, base_color_property_name) == first_material_document_color)

        # 16. Save the first_material_document as a new material document file.
        first_material_document_saved = atom_tools_utils.save_document(first_material_document_id)
        Report.result(
            Tests.saved_first_material_document,
            first_material_document_saved is True)

        # 17. Open a second material document referred to as "second_material_document".
        second_material_document_id = atom_tools_utils.open_document(second_material_document)
        Report.result(
            Tests.opened_second_material_document,
            atom_tools_utils.is_document_open(second_material_document_id) is True)

        # 18. Change the baseColor.color property value of the second_material_document to 0.4156, 0.0196, 0.6862, 1.0.
        starting_second_material_document_color = material_editor_utils.get_property(
            second_material_document_id, base_color_property_name)
        second_material_document_color = azlmbr.math.Color(0.4156, 0.0196, 0.6862, 1.0)
        material_editor_utils.set_property(
            second_material_document_id, base_color_property_name, second_material_document_color)
        Report.result(
            Tests.changed_second_material_document_color,
            material_editor_utils.get_property(
                second_material_document_id, base_color_property_name) == second_material_document_color)

        # 19. Save the second_material_document as a new material document file.
        second_material_document_saved = atom_tools_utils.save_document(second_material_document_id)
        Report.result(
            Tests.saved_second_material_document,
            second_material_document_saved is True)

        # 20. Close the first_material_document and second_material_document.
        first_material_document_closed = atom_tools_utils.close_document(first_material_document_id)
        Report.result(
            Tests.closed_first_material_document,
            first_material_document_closed is True)
        second_material_document_closed = atom_tools_utils.close_document(second_material_document_id)
        Report.result(
            Tests.closed_second_material_document,
            second_material_document_closed is True)

        # 21. Open the first_material_document & verify the changes are saved.
        first_material_document_id = atom_tools_utils.open_document(first_material_document)
        Report.result(
            Tests.reopened_first_material_document,
            atom_tools_utils.is_document_open(first_material_document_id))
        Report.result(
            Tests.verified_first_material_document_color,
            material_editor_utils.get_property(
                first_material_document_id, base_color_property_name) == first_material_document_color)

        # 22. Open the second_material_document & verify the changes are saved.
        second_material_document_id = atom_tools_utils.open_document(second_material_document)
        Report.result(
            Tests.reopened_second_material_document,
            atom_tools_utils.is_document_open(first_material_document_id))
        Report.result(
            Tests.verified_second_material_document_color,
            material_editor_utils.get_property(
                second_material_document_id, base_color_property_name) == second_material_document_color)

        # 23. Revert changes to first_material_document and second_material_document.
        material_editor_utils.set_property(
            first_material_document_id, base_color_property_name, starting_first_material_document_color)
        Report.result(
            Tests.reverted_first_material_document_color,
            material_editor_utils.get_property(
                first_material_document_id,
                base_color_property_name) == starting_first_material_document_color)
        material_editor_utils.set_property(
            second_material_document_id, base_color_property_name, starting_second_material_document_color)
        Report.result(
            Tests.reverted_second_material_document_color,
            material_editor_utils.get_property(
                second_material_document_id,
                base_color_property_name) == starting_second_material_document_color)

        # 24. Re-save first_material_document and second_material_document to restore their original values.
        first_material_document_resaved = atom_tools_utils.save_document(first_material_document_id)
        Report.result(
            Tests.resaved_first_material_document,
            first_material_document_resaved is True)
        second_material_document_resaved = atom_tools_utils.save_document(second_material_document_id)
        Report.result(
            Tests.resaved_second_material_document,
            second_material_document_resaved is True)

        # 25. Close all currently opened documents.
        closed_all_material_documents = atom_tools_utils.close_all_documents()
        Report.result(
            Tests.closed_first_and_second_material_documents,
            closed_all_material_documents is True)
        Report.result(
            Tests.reclosed_first_material_document,
            atom_tools_utils.is_document_open(first_material_document_id) is False)
        Report.result(
            Tests.reclosed_second_material_document,
            atom_tools_utils.is_document_open(second_material_document_id) is False)

        # 26. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MaterialEditor_FileSaveChecks_AllChecksPass)
