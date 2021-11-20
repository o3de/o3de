"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# import azlmbr.materialeditor will fail with a ModuleNotFound error when using this script with Editor.exe
# This is because azlmbr.materialeditor only binds to MaterialEditor.exe and not Editor.exe
# You need to launch this script with MaterialEditor.exe in order for azlmbr.materialeditor to appear.

import os
import sys
import time

import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.projectroot, "Gem", "PythonTests"))

import Atom.atom_utils.material_editor_utils as material_editor

NEW_MATERIAL = "test_material.material"
NEW_MATERIAL_1 = "test_material_1.material"
NEW_MATERIAL_2 = "test_material_2.material"
TEST_MATERIAL_1 = "001_DefaultWhite.material"
TEST_MATERIAL_2 = "002_BaseColorLerp.material"
TEST_MATERIAL_3 = "003_MetalMatte.material"
TEST_DATA_PATH = os.path.join(
    azlmbr.paths.engroot, "Gems", "Atom", "TestData", "TestData", "Materials", "StandardPbrTestCases"
)
MATERIAL_TYPE_PATH = os.path.join(
    azlmbr.paths.engroot, "Gems", "Atom", "Feature", "Common", "Assets",
    "Materials", "Types", "StandardPBR.materialtype",
)
CACHE_FILE_EXTENSION = ".azmaterial"


def run():
    """
    Summary:
    Material Editor basic tests including the below
    1. Opening an Existing Asset
    2. Creating a New Asset
    3. Closing Selected Material
    4. Closing All Materials
    5. Closing all but Selected Material
    6. Saving Material
    7. Saving as a New Material
    8. Saving as a Child Material
    9. Saving all Open Materials

    Expected Result:
    All the above functions work as expected in Material Editor.

    :return: None
    """

    # 1) Test Case: Opening an Existing Asset
    document_id = material_editor.open_material(MATERIAL_TYPE_PATH)
    print(f"Material opened: {material_editor.is_open(document_id)}")

    # Verify if the test material exists initially
    target_path = os.path.join(azlmbr.paths.projectroot, "Materials", NEW_MATERIAL)
    print(f"Test asset doesn't exist initially: {not os.path.exists(target_path)}")

    # 2) Test Case: Creating a New Material Using Existing One
    material_editor.save_document_as_child(document_id, target_path)
    material_editor.wait_for_condition(lambda: os.path.exists(target_path), 2.0)
    print(f"New asset created: {os.path.exists(target_path)}")
    time.sleep(2.0)

    # Verify if the newly created document is open
    new_document_id = material_editor.open_material(target_path)
    material_editor.wait_for_condition(lambda: material_editor.is_open(new_document_id))
    print(f"New Material opened: {material_editor.is_open(new_document_id)}")

    # 3) Test Case: Closing Selected Material
    print(f"Material closed: {material_editor.close_document(new_document_id)}")

    # Open materials initially
    document1_id, document2_id, document3_id = (
        material_editor.open_material(os.path.join(TEST_DATA_PATH, material))
        for material in [TEST_MATERIAL_1, TEST_MATERIAL_2, TEST_MATERIAL_3]
    )

    # 4) Test Case: Closing All Materials
    print(f"All documents closed: {material_editor.close_all_documents()}")

    # 5) Test Case: Closing all but Selected Material
    document1_id, document2_id, document3_id = (
        material_editor.open_material(os.path.join(TEST_DATA_PATH, material))
        for material in [TEST_MATERIAL_1, TEST_MATERIAL_2, TEST_MATERIAL_3]
    )
    result = material_editor.close_all_except_selected(document1_id)
    print(f"Close All Except Selected worked as expected: {result and material_editor.is_open(document1_id)}")

    # 6) Test Case: Saving Material
    document_id = material_editor.open_material(os.path.join(TEST_DATA_PATH, TEST_MATERIAL_1))
    property_name = azlmbr.name.Name("baseColor.color")
    initial_color = material_editor.get_property(document_id, property_name)
    # Assign new color to the material file and save the actual material
    expected_color = math.Color(0.25, 0.25, 0.25, 1.0)
    material_editor.set_property(document_id, property_name, expected_color)
    material_editor.save_document(document_id)
    time.sleep(2.0)

    # 7) Test Case: Saving as a New Material
    # Assign new color to the material file and save the document as copy
    expected_color_1 = math.Color(0.5, 0.5, 0.5, 1.0)
    material_editor.set_property(document_id, property_name, expected_color_1)
    target_path_1 = os.path.join(azlmbr.paths.projectroot, "Materials", NEW_MATERIAL_1)
    cache_file_name_1 = os.path.splitext(NEW_MATERIAL_1)  # Example output: ('test_material_1', '.material')
    cache_file_1 = f"{cache_file_name_1[0]}{CACHE_FILE_EXTENSION}"
    target_path_1_cache = os.path.join(azlmbr.paths.products, "materials", cache_file_1)
    material_editor.save_document_as_copy(document_id, target_path_1)
    material_editor.wait_for_condition(lambda: os.path.exists(target_path_1_cache), 4.0)

    # 8) Test Case: Saving as a Child Material
    # Assign new color to the material file save the document as child
    expected_color_2 = math.Color(0.75, 0.75, 0.75, 1.0)
    material_editor.set_property(document_id, property_name, expected_color_2)
    target_path_2 = os.path.join(azlmbr.paths.projectroot, "Materials", NEW_MATERIAL_2)
    cache_file_name_2 = os.path.splitext(NEW_MATERIAL_1)  # Example output: ('test_material_2', '.material')
    cache_file_2 = f"{cache_file_name_2[0]}{CACHE_FILE_EXTENSION}"
    target_path_2_cache = os.path.join(azlmbr.paths.products, "materials", cache_file_2)
    material_editor.save_document_as_child(document_id, target_path_2)
    material_editor.wait_for_condition(lambda: os.path.exists(target_path_2_cache), 4.0)

    # Close/Reopen documents
    material_editor.close_all_documents()
    document_id = material_editor.open_material(os.path.join(TEST_DATA_PATH, TEST_MATERIAL_1))
    document1_id = material_editor.open_material(target_path_1)
    document2_id = material_editor.open_material(target_path_2)

    # Verify if the changes are saved in the actual document
    actual_color = material_editor.get_property(document_id, property_name)
    print(f"Actual Document saved with changes: {material_editor.compare_colors(actual_color, expected_color)}")

    # Verify if the changes are saved in the document saved as copy
    actual_color = material_editor.get_property(document1_id, property_name)
    result_copy = material_editor.compare_colors(actual_color, expected_color_1)
    print(f"Document saved as copy is saved with changes: {result_copy}")

    # Verify if the changes are saved in the document saved as child
    actual_color = material_editor.get_property(document2_id, property_name)
    result_child = material_editor.compare_colors(actual_color, expected_color_2)
    print(f"Document saved as child is saved with changes: {result_child}")

    # Revert back the changes in the actual document
    material_editor.set_property(document_id, property_name, initial_color)
    material_editor.save_document(document_id)
    material_editor.close_all_documents()

    # 9) Test Case: Saving all Open Materials
    # Open first material and make change to the values
    document1_id = material_editor.open_material(os.path.join(TEST_DATA_PATH, TEST_MATERIAL_1))
    property1_name = azlmbr.name.Name("metallic.factor")
    initial_metallic_factor = material_editor.get_property(document1_id, property1_name)
    expected_metallic_factor = 0.444
    material_editor.set_property(document1_id, property1_name, expected_metallic_factor)

    # Open second material and make change to the values
    document2_id = material_editor.open_material(os.path.join(TEST_DATA_PATH, TEST_MATERIAL_2))
    property2_name = azlmbr.name.Name("baseColor.color")
    initial_color = material_editor.get_property(document2_id, property2_name)
    expected_color = math.Color(0.4156, 0.0196, 0.6862, 1.0)
    material_editor.set_property(document2_id, property2_name, expected_color)

    # Save all and close all documents
    material_editor.save_all()
    material_editor.close_all_documents()

    # Reopen materials and verify values
    document1_id = material_editor.open_material(os.path.join(TEST_DATA_PATH, TEST_MATERIAL_1))
    result = material_editor.is_close(
        material_editor.get_property(document1_id, property1_name), expected_metallic_factor, 0.00001
    )
    document2_id = material_editor.open_material(os.path.join(TEST_DATA_PATH, TEST_MATERIAL_2))
    result = result and material_editor.compare_colors(
        expected_color, material_editor.get_property(document2_id, property2_name))
    print(f"Save All worked as expected: {result}")

    # Revert the changes made
    material_editor.set_property(document1_id, property1_name, initial_metallic_factor)
    material_editor.set_property(document2_id, property2_name, initial_color)
    material_editor.save_all()
    material_editor.close_all_documents()


if __name__ == "__main__":
    run()
