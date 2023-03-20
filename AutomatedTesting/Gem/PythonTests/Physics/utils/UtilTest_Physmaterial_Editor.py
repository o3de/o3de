"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

'''
This unittest might have to be refactored once changes to Physmaterial_Editor.py are made. 
These changes will occur after the but, reverence below is resloved.
Bug: LY-107392
'''
class Tests:
    opening_bad_file         = ("Bad file could not be opened",           "Bad file was opened")
    opened_good_file         = ("Valid file was opened",                  "Valid file was not opened")
    opened_default_file      = ("Default file was opened",                "Default file was not opened")
    number_of_materials      = ("File has correct number of materials",   "File has wrong number of materials")
    material_deleted         = ("Material successfully deleted",          "Material was not deleted")
    material_modified        = ("Material successfully modified",         "Material was not modified")
    modify_attribute_error   = ("Invalid attribute raises error",         "Invalid attribute doesn't raise error")
    modify_combine_error     = ("Invalid combine value raises error",     "Invalid combine value doesn't raise error")
    modify_friction_error    = ("Invalid friction value raises error",    "Invalid restitution value doesn't raise error")
    delete_value_error       = ("Nonexistent material can't be deleted",  "Nonexistend material deleted")
    no_overwrite             = ("File not updated with no write() call",  "File updated with no write() call")
    overwrite_works          = ("File can be updated",                    "File can not be updated")


def run():
    """
    Summary: Tests the utility of the Physmaterial_Editor

    Level Description: Completely empty and used as method to run material editing methods from the editor

    Expected Results: All Physmaterial tests work as expected.

    Test Steps:
    1) Open Level
    2) Check that the correct files open
    3) Check library properties
    4) Check editor methods
    5) Check write() works as expected
    6) Close Editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    import os
    import sys


    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    import azlmbr.legacy.general as general
    from Physmaterial_Editor import Physmaterial_Editor

    # Test file names
    MATERIAL_LIBRARY = "UtilTest_Physmaterial_Editor_TestLibrary.physmaterial"
    MATERIAL_LIBRARY_BAD = "UtilTest_Physmaterial_Editor_TestLibrary_Bad.physmaterial"
    NUMBER_OF_MATERIALS = 21

    # Test Functions
    def test_open_good_file():
        # type () -> bool
        try:
            Physmaterial_Editor(MATERIAL_LIBRARY)
        except Exception:
            return False
        return True

    def test_fail_opening_bad_file():
        # type () -> bool
        try:
            Physmaterial_Editor(MATERIAL_LIBRARY_BAD)
        except Exception:
            return True
        return False
    
    def test_open_default_file():
        # type () -> bool
        try:
            Physmaterial_Editor()
        except Exception:
            return False
        return True

    def test_number_of_materials():
        # type () -> bool
        try:
            material_library_object = Physmaterial_Editor(MATERIAL_LIBRARY)
        except Exception as e:
            print (e)
            raise Exception("Something went wrong with test_number_of materials")
        return material_library_object.number_of_materials == NUMBER_OF_MATERIALS

    def test_can_modify_material_element():
        # type () -> bool
        try:
            material_library_object = Physmaterial_Editor(MATERIAL_LIBRARY)
        except Exception as e:
            print (e)
            raise Exception("Something went wrong with test_can_modify_material_element")
        return material_library_object.modify_material("canopy", "StaticFriction", 1.56)

    def test_bad_attribute_passed_modify_material():
        # type () -> bool
        material_library_object = Physmaterial_Editor(MATERIAL_LIBRARY)
        try:
            material_library_object.modify_material("canopy", "Friction", 1.56)
        except Exception:
            return True
        return False

    def test_bad_value_passed_modify_combine():
        # type () -> bool
        material_library_object = Physmaterial_Editor(MATERIAL_LIBRARY)
        try:
            material_library_object.modify_material("canopy", "FrictionCombine", 1.56)
        except Exception:
            return True
        return False
    
    def test_bad_value_passed_modify_friction():
        # type () -> bool
        material_library_object = Physmaterial_Editor(MATERIAL_LIBRARY)
        try:
            material_library_object.modify_material("canopy", "StaticFriction", "1.56")
        except Exception:
            return True
        return False
    
    def test_can_delete_material():
        # type () -> bool
        try:
            material_library_object = Physmaterial_Editor(MATERIAL_LIBRARY)
        except Exception as e:
            print (e)
            raise Exception("Something went wrong with test_can_delete_material")
        return material_library_object.delete_material("default")
    
    def test_can_not_delete_nonexistant_material():
        # type () -> bool
        try:
            material_library_object = Physmaterial_Editor(MATERIAL_LIBRARY)
        except Exception as e:
            print (e)
            raise Exception("Something went wrong with test_can_not_delete_nonexistant_material")
        return material_library_object.delete_material("default")
        
    def test_file_does_not_update_without_overwrite():
        # type () -> bool
        try:
            material_library_object = Physmaterial_Editor(MATERIAL_LIBRARY)
            material_deleted = material_library_object.delete_material("fabric")
        
            material_library_object = Physmaterial_Editor(MATERIAL_LIBRARY)
        except Exception as e:
            print(e)
            raise Exception("Something went wrong with test_file_does_not_update_without_overwrite")
        return  material_library_object.number_of_materials != (NUMBER_OF_MATERIALS - 1) and material_deleted

    def test_file_does_update_with_overwrite():
        # type () -> bool
        try:
            material_library_object = Physmaterial_Editor(MATERIAL_LIBRARY)
            material_deleted = material_library_object.delete_material("glass")
            material_library_object.save_changes()
            material_library_object = Physmaterial_Editor(MATERIAL_LIBRARY)
        except Exception as e:
            print(e)
            raise Exception("Something went wrong with test_file_does_update_with_overwrite")
        return material_library_object.number_of_materials == (NUMBER_OF_MATERIALS - 1) or material_deleted

    # Test Script
    # 1) Open Level
    helper.init_idle()
    helper.open_level('physics', 'Physmaterial_Editor_Test')

    # 2) Check that the correct files open
    Report.result(Tests.opening_bad_file, test_fail_opening_bad_file())
    Report.result(Tests.opened_good_file, test_open_good_file())
    Report.result(Tests.opened_default_file, test_open_default_file())

    # 3) Check library properties
    Report.result(Tests.number_of_materials, test_number_of_materials())

    # 4) Check editor methods
    Report.result(Tests.material_deleted, test_can_delete_material())
    Report.result(Tests.material_modified, test_can_modify_material_element())
    Report.result(Tests.modify_attribute_error, test_bad_attribute_passed_modify_material())
    Report.result(Tests.modify_combine_error, test_bad_value_passed_modify_combine())
    Report.result(Tests.modify_friction_error, test_bad_value_passed_modify_friction())
    Report.result(Tests.delete_value_error, test_can_not_delete_nonexistant_material())

    # 5) Check write() works as expected
    Report.result(Tests.no_overwrite, test_file_does_not_update_without_overwrite())
    Report.result(Tests.overwrite_works, test_file_does_update_with_overwrite())

if __name__ == "__main__":
    run()