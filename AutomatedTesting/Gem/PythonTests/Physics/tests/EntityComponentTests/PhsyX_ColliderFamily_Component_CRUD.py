"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def PhysX_ColliderFamily_Component_CRUD():
    """
    Summary:
    Test Does Something

    Expected Behavior:
    Some Behavior

    PhysX Collider Components:
     - PhysX Collider

    Test Steps:
     1) Add an Entity to manipulate
     2) Add a PhysX Collider Component to Entity
     3) Set Box Shape and Child Properties
     4) Set Capsule Shape and Child Properties
     5) Set Cylinder Shape and Child Properties
     6) Set Sphere Shape and Child Properties
     7) Set General Properties
     8) Set PhyicsAsset Shape and Child Properties
     9) Delete Component

    :return: None
    """

    # Helper file Imports
    import os
    import azlmbr.legacy.general as general

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    from editor_python_test_tools.editor_component.physx_collider import PhysxCollider
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Tracer, TestHelper


    # 0) Pre-conditions
    physx_mesh = os.path.join("objects", "_primitives", "_box_1x1.pxmesh")
    physx_material = os.path.join("physx", "glass.physxmaterial")

    TestHelper.init_idle()
    TestHelper.open_level("", "Base")



    with Tracer() as section_tracer:
        # 1 ) Add an Entity to manipulate
        phsyx_collider_entity = EditorEntity.create_editor_entity("TestEntity")

        # 2) Add a PhysX Collider Component to Entity
        physx_collider = PhysxCollider(phsyx_collider_entity)

        # 3) Set Box Shape and Child Properties
        physx_collider.set_box_shape()
        physx_collider.set_box_dimensions(0.0, 0.0, 0.0)
        physx_collider.set_box_dimensions(256.0, 256.0, 256.0)
        physx_collider.set_box_dimensions(2, 2.5, 2.5)


        # 4) Setting Physx Collider Shapes



        # test_entity = EE.create_editor_entity("Test")
        # test_component = test_entity.add_component("PhysX Collider")
        # output = [test_component.get_property_type_visibility(), test_component.get_property_type_visibility()]
        #
        # print(test_component.get_property_type_visibility())
        #
        # path = test_component.get_component_property_value("somepath")
        #
        #
        # Report.critical_result(output, False)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PhysX_ColliderFamily_Component_CRUD)

