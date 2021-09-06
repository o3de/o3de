"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID :  C29279329
# Test Case Title : White Box mesh shape can be changed with the Default Shape dropdown on the Component


# fmt:off
class Tests():
    default_shape_cube = ("Default shape set to cube",                   "Default shape is not set to cube")
    default_shape_tetrahedron = ("Default shape changed to tetrahedron", "Failed to change default shape to tetrahedron")
    default_shape_icosahedron = ("Default shape changed to icosahedron", "Failed to change default shape to icosahedron")
    default_shape_cylinder = ("Default shape changed to cylinder",       "Failed to change default shape to cylinder")
    default_shape_sphere = ("Default shape changed to sphere",           "Failed to change default shape to sphere")
# fmt:on

critical_shape_check = ("Default shape has more than 0 sides", "default shape has 0 sides")

def C29279329_WhiteBox_SetDefaultShape():
    import os
    import sys
    from Gems.WhiteBox.Editor.Scripts import WhiteBoxInit as init

    import azlmbr.whitebox.api as api
    import azlmbr.bus as bus
    import azlmbr.legacy.general as general
    
    from editor_python_test_tools.utils import Report
    
    from editor_python_test_tools.utils import TestHelper as helper

    def check_shape_result(success_fail_tuple, condition):
        result = Report.result(success_fail_tuple, condition)
        if not result:
            face_count = white_box_handle.MeshFaceCount()
            Report.critical_result(critical_shape_check,  face_count > 0)
            Report.info(f"Face count = {face_count}")

    shape_types = azlmbr.globals.property

    # expected results
    expected_results = {
        shape_types.CUBE: 12,
        shape_types.TETRAHEDRON: 4,
        shape_types.ICOSAHEDRON: 20,
        shape_types.CYLINDER: 64,
        shape_types.SPHERE: 320
    }

    # open level
    helper.init_idle()
    general.open_level("EmptyLevel")

    # create white box entity and attach component
    white_box_entity = init.create_white_box_entity()
    white_box_mesh_component = init.create_white_box_component(white_box_entity)
    white_box_handle = init.create_white_box_handle(white_box_mesh_component)

    # verify results
    # cube
    check_shape_result(
        Tests.default_shape_cube,
        white_box_handle.MeshFaceCount() == expected_results.get(shape_types.CUBE))

    # tetrahedron
    azlmbr.whitebox.request.bus.EditorWhiteBoxComponentRequestBus(
        bus.Event, 'SetDefaultShape', white_box_mesh_component, shape_types.TETRAHEDRON)
    check_shape_result(
        Tests.default_shape_tetrahedron,
        white_box_handle.MeshFaceCount() == expected_results.get(shape_types.TETRAHEDRON))

    # icosahedron
    azlmbr.whitebox.request.bus.EditorWhiteBoxComponentRequestBus(
        bus.Event, 'SetDefaultShape', white_box_mesh_component, shape_types.ICOSAHEDRON)
    check_shape_result(
        Tests.default_shape_icosahedron,
        white_box_handle.MeshFaceCount() == expected_results.get(shape_types.ICOSAHEDRON))

    # cylinder
    azlmbr.whitebox.request.bus.EditorWhiteBoxComponentRequestBus(
        bus.Event, 'SetDefaultShape', white_box_mesh_component, shape_types.CYLINDER)
    check_shape_result(
        Tests.default_shape_cylinder,
        white_box_handle.MeshFaceCount() == expected_results.get(shape_types.CYLINDER))

    # sphere
    azlmbr.whitebox.request.bus.EditorWhiteBoxComponentRequestBus(
        bus.Event, 'SetDefaultShape', white_box_mesh_component, shape_types.SPHERE)
    check_shape_result(
        Tests.default_shape_sphere,
        white_box_handle.MeshFaceCount() == expected_results.get(shape_types.SPHERE))

    # close editor
    helper.close_editor()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(C29279329_WhiteBox_SetDefaultShape)
