"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

def Editor_ObjectManagerCommands_Works(BaseClass):
    # Description: 
    # Tests a portion of the Python API from ObjectManager.cpp while the Editor is running

    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.legacy.general as general

        currentLevelName = editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'GetCurrentLevelName')
        BaseClass.check_result(currentLevelName == 'TestDependenciesLevel', "Not TestDependenciesLevel level")
    
        objs_list = general.get_all_objects()
    
        BaseClass.check_result(len(objs_list) > 0, "get_all_objects works")
    
        general.clear_selection()
        general.select_object(objs_list[0])
    
        selected_objs_list = general.get_names_of_selected_objects()
    
        BaseClass.check_result(len(selected_objs_list) == 1, "get_names_of_selected_objects works")
    
        select = [objs_list[1], objs_list[2]]
        general.select_objects(select)
    
        selected_objs_list = general.get_names_of_selected_objects()
    
        BaseClass.check_result(len(selected_objs_list) == 3, f"select_objects works {selected_objs_list}")
    
        sel_position = general.get_selection_center()
        sel_aabb = general.get_selection_aabb()
        centerX = sel_position.get_property("x")
        cornerX = sel_aabb.get_property("min").get_property("x")
    
        BaseClass.check_result(not(centerX == cornerX), "sel_position and sel_aabb both work")
    
        unselect = [objs_list[0], objs_list[2]]
        general.unselect_objects(unselect)
    
        BaseClass.check_result(general.get_num_selected() == 1, "get_num_selected and unselect_objects both work")
    
        general.clear_selection()
    
        BaseClass.check_result(general.get_num_selected() == 0, "clear_selection works")

        def fetch_vector3_parts(vec3):
            x = vec3.get_property('x')
            y = vec3.get_property('y')
            z = vec3.get_property('z')
            return (x, y, z)
    
        position = general.get_position(objs_list[1])
        px1, py1, pz1 = fetch_vector3_parts(position)
        general.set_position(objs_list[1], px1 + 10, py1 - 4, pz1 + 3)
        new_position = general.get_position(objs_list[1])
        px2, py2, pz2 = fetch_vector3_parts(new_position)
    
        BaseClass.check_result((px2 > px1) and (py2 < py1) and (pz2 > pz1), "position setter/getter works")
    
        rotation = general.get_rotation(objs_list[1])
        rx1, ry1, rz1 = fetch_vector3_parts(rotation)
        general.set_rotation(objs_list[1], rx1 + 10, ry1 - 4, rz1 + 3)
        new_rotation = general.get_rotation(objs_list[1])
        rx2, ry2, rz2 = fetch_vector3_parts(new_rotation)
    
        BaseClass.check_result((rx2 > rx1) and (ry2 < ry1) and (rz2 > rz1), "rotation setter/getter works")
    
        scale = general.get_scale(objs_list[1])
        sx1, sy1, sz1 = fetch_vector3_parts(scale)
        general.set_scale(objs_list[1], sx1 + 10, sy1 + 4, sz1 + 3)
        new_scale = general.get_scale(objs_list[1])
        sx2, sy2, sz2 = fetch_vector3_parts(new_scale)
    
        BaseClass.check_result((sx2 > sx1) and (sy2 > sy1) and (sz2 > sz1), "scale setter/getter works")
    
        general.select_object(objs_list[2])
        general.delete_selected()
        new_objs_list = general.get_all_objects()
    
        BaseClass.check_result((len(new_objs_list) < len(objs_list)), "delete_selected works")
    
        general.delete_object(objs_list[0])
        new_objs_list = general.get_all_objects()
    
        BaseClass.check_result((len(new_objs_list) < len(objs_list)), "delete_object works")
    
        general.rename_object(objs_list[1], "some_test_name")
        new_objs_list = general.get_all_objects()
    
        BaseClass.check_result("some_test_name" in new_objs_list, "rename_object works")

if __name__ == "__main__":
    tester = Editor_ObjectManagerCommands_Works()
    tester.test_case(tester.test, level="TestDependenciesLevel")
