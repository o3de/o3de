"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests():
    create_collider_entity = (
        "PhysX Collider Entity Created",
        "PhysX Collider Entity failed to respond after creation.")
    create_shape_collider_entity = (
        "PhysX Shape Collider Entity Created.",
        "PhysX Shape Collider Entity failed to respond after creation.")
    create_physx_collider_component = (
        "PhysX Collider Component Added.",
        "PhysX Collider Component failed to respond after creation.")
    create_shape_collider_component = (
        "PhysX Shape Collider Component Added.",
        "PhysX Shape Collider Component failed to respond after creation.")
    set_physx_box_shape = (
        "PhysX Collider Box Shape set.",
        "PhysX Collider Box Shape failed to set."
    )
    set_physx_sphere_shape = (
        "PhysX Collider Sphere Shape set.",
        "PhysX Collider Sphere Shape failed to set."
    )
    set_physx_capsule_shape = (
        "PhysX Collider Capsule Shape set.",
        "PhysX Collider Capsule Shape failed to set."
    )
    add_box_shape_component = (
        "Box Shape component added to PhysX Shape Collider entity successfully.",
        "Box Shape component failed to add to PhysX Shape Collider entity")
    add_capsule_shape_component = (
        "Capsule Shape component added to PhysX Shape Collider entity successfully.",
        "Capsule Shape component failed to add to PhysX Shape Collider entity"
    )
    add_cylinder_shape_component = (
        "Cylinder Shape component added to PhysX Shape Collider entity successfully.",
        "Cylinder Shape component failed to add to PhysX Shape Collider entity"
    )
    add_polygon_prism_shape_component = (
        "Polygon Prism Shape component added to PhysX Shape Collider entity successfully.",
        "Polygon Prism Shape component failed to add to PhysX Shape Collider entity"
    )
    add_quad_shape_component = (
        "Quad Shape component added to PhysX Shape Collider entity successfully.",
        "Quad Shape component failed to add to PhysX Shape Collider entity"
    )
    add_sphere_shape_component = (
        "Sphere Shape component added to PhysX Shape Collider entity successfully.",
        "Sphere Shape component failed to add to PhysX Shape Collider entity"
    )


def PhysX_ColliderFamily_Component_CRUD():
    """
    Summary:
    Test Does Something

    Expected Behavior:
    Some Behavior

    PhysX Collider Family Components:
     - PhysX Collider
     - PhysX Shape Collider

    Test Steps:
     1) Add an Entity for each Collider Family Component
     2) Add each Collider Family Component to its own entity (PhysX Collider, PhysX Shape Collider)
     3) Manipulate each modifiable field and validate they are in expected state
     4) Validate Fields are in expected state
     5) Delete components and validate its removal

    :return: None
    """

    # Helper file Imports
    import os

    import azlmbr.physics as physics
    import azlmbr.legacy.general as general
    import editor_python_test_tools.hydra_editor_utils as hydra

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper


    from editor_python_test_tools.editor_component.physx_collider import PhysxCollider
    from editor_python_test_tools.editor_entity_utils import EditorEntity as EE
    from editor_python_test_tools.utils import Tracer


    # 0) Pre-conditions
    physx_mesh = os.path.join("objects", "_primitives", "_box_1x1.pxmesh")
    hydra.open_base_level()

    # 1) Create entities to hold PhysX Collider Components
    physx_collider_box_entity = EE.create_editor_entity("physx_collider_box_entity")
    physx_collider_capsule_entity = EE.create_editor_entity("physx_collider_capsule_entity")
    physx_collider_sphere_entity = EE.create_editor_entity("physx_collider_sphere_entity")
    physx_collider_cylinder_entity = EE.create_editor_entity("physx_collider_cylinder_entity")
    physx_collider_physicsasset_entity = EE.create_editor_entity("physx_collider_physicsasset_entity")


    physx_shape_collider_box_entity = EE.create_editor_entity("physx_shape_collider_box_entity")
    physx_shape_collider_capsule_entity = EE.create_editor_entity("physx_shape_collider_capsule_entity")
    physx_shape_collider_cylinder_entity = EE.create_editor_entity("physx_shape_collider_cylinder_entity")
    physx_shape_collider_polygon_prism_entity = EE.create_editor_entity("physx_shape_collider_polygon_prism_entity")
    physx_shape_collider_quad_entity = EE.create_editor_entity("physx_shape_collider_quad_entity")
    physx_shape_collider_sphere_entity = EE.create_editor_entity("physx_shape_collider_sphere_entity")

    with Tracer() as section_tracer:

        # 2) Adding Collider Components to their entities
        physx_collider_box_shape_component = PhysxCollider(physx_collider_box_entity)
        physx_collider_capsule_shape_component = PhysxCollider(physx_collider_capsule_entity)
        physx_collider_sphere_shape_component = PhysxCollider(physx_collider_sphere_entity)
        physx_collider_cylinder_shape_component = PhysxCollider(physx_collider_cylinder_entity)
        physx_collider_physicsasset_component = PhysxCollider(physx_collider_physicsasset_entity)


        # 3) Adding Physx Shape Collider Components to corresponding Shape Collider Entities

        # 4) Setting Physx Collider Shapes
        physx_collider_box_shape_component.set_box_shape()
        physx_collider_capsule_shape_component.set_capsule_shape()
        physx_collider_sphere_shape_component.set_sphere_shape()
        physx_collider_cylinder_shape_component.set_cylinder_shape()
        physx_collider_physicsasset_component.set_physx_mesh_from_path(physx_mesh)

        physx_collider_box_shape_component.set_box_dimensions(1.0, 2.0, 3.0)


        test_entity = EE.create_editor_entity("TestEntity")
        test_collider = PhysxCollider(test_entity)
        test_collider.set_box_shape()
        test_collider.set_box_dimensions(5.0, 1.0, 2.7)

        test_collider.set_collision_layer()


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

