"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests a portion of the Python API from ObjectManager.cpp while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math
import azlmbr.legacy.general as general

def fetch_vector3_parts(vec3):
    x = vec3.get_property('x')
    y = vec3.get_property('y')
    z = vec3.get_property('z')
    return (x, y, z)

general.idle_enable(True)

# Try to open the WaterSample level. If not, fail the test.
# We need to rely on an existing level since the API does not provide
# a way to create entities, but only lets us manipulate them.
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'WaterSample')

if (editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'GetCurrentLevelName') == 'WaterSample'):
    
    objs_list = general.get_all_objects()
    
    if(len(objs_list) > 0):
        print("get_all_objects works")
    
    general.clear_selection()
    general.select_object(objs_list[0])
    
    selected_objs_list = general.get_names_of_selected_objects()
    
    if(len(selected_objs_list) == 1):
        print("get_names_of_selected_objects works")
    
    select = [objs_list[1], objs_list[2]]
    general.select_objects(select)
    
    selected_objs_list = general.get_names_of_selected_objects()
    
    if(len(selected_objs_list) == 3):
        print("select_objects works")
    
    sel_position = general.get_selection_center()
    sel_aabb = general.get_selection_aabb()
    centerX = sel_position.get_property("x")
    cornerX = sel_aabb.get_property("min").get_property("x")
    
    if not(centerX == cornerX):
        print("sel_position and sel_aabb both work")
    
    unselect = [objs_list[0], objs_list[2]]
    general.unselect_objects(unselect)
    
    if(general.get_num_selected() == 1):
        print("get_num_selected and unselect_objects both work")
    
    general.clear_selection()
    
    if(general.get_num_selected() == 0):
        print("clear_selection works")
    
    position = general.get_position(objs_list[1])
    px1, py1, pz1 = fetch_vector3_parts(position)
    general.set_position(objs_list[1], px1 + 10, py1 - 4, pz1 + 3)
    new_position = general.get_position(objs_list[1])
    px2, py2, pz2 = fetch_vector3_parts(new_position)
    
    if(px2 > px1) and (py2 < py1) and (pz2 > pz1):
        print("position setter/getter works")
    
    rotation = general.get_rotation(objs_list[1])
    rx1, ry1, rz1 = fetch_vector3_parts(rotation)
    general.set_rotation(objs_list[1], rx1 + 10, ry1 - 4, rz1 + 3)
    new_rotation = general.get_rotation(objs_list[1])
    rx2, ry2, rz2 = fetch_vector3_parts(new_rotation)
    
    if(rx2 > rx1) and (ry2 < ry1) and (rz2 > rz1):
        print("rotation setter/getter works")
    
    scale = general.get_scale(objs_list[1])
    sx1, sy1, sz1 = fetch_vector3_parts(scale)
    general.set_scale(objs_list[1], sx1 + 10, sy1 + 4, sz1 + 3)
    new_scale = general.get_scale(objs_list[1])
    sx2, sy2, sz2 = fetch_vector3_parts(new_scale)
    
    if(sx2 > sx1) and (sy2 > sy1) and (sz2 > sz1):
        print("scale setter/getter works")
    
    general.select_object(objs_list[2])
    general.delete_selected()
    new_objs_list = general.get_all_objects()
    
    if(len(new_objs_list) < len(objs_list)):
        print("delete_selected works")
    
    general.delete_object(objs_list[0])
    new_objs_list = general.get_all_objects()
    
    if(len(new_objs_list) < len(objs_list)):
        print("delete_object works")
    
    general.rename_object(objs_list[1], "some_test_name")
    new_objs_list = general.get_all_objects()
    
    for elem in new_objs_list:
        if(elem == "some_test_name"):
            print("rename_object works")

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
