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
     1) Open Base Level
     2) Add an Entity for each Collider Family Component
     3) Add each Collider Family Component to its own entity
     4) Manipulate each modifiable field and validate they are in expected state
     5) Validate Fields are in expected state
     6) Delete components and validate its removal

    :return: None
    """

    # Helper file Imports

    import azlmbr.math as math
    import azlmbr.entity as EntityId
    import azlmbr.editor as editor
    import azlmbr.bus as bus
    import azlmbr.physics as physics

    import editor_python_test_tools.hydra_editor_utils as hydra


    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.editor_entity_utils import EditorComponent
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer
    from editor_python_test_tools.asset_utils import Asset

    from Physics.utils.physics_tools import (create_validated_entity, add_validated_component)
    from Physics.utils.physics_constants import (PHYSX_COLLIDER, PHYSX_SHAPE_COLLIDER,
                                                 BOX_SHAPE_COMPONENT, CAPSULE_SHAPE_COMPONENT,
                                                 PHSX_SHAPE_COLLIDER_SHAPE,
                                                 CYLINDER_SHAPE_COMPONENT, POLYGON_PRISM_SHAPE_COMPONENT,
                                                 QUAD_SHAPE_COMPONENT, SPHERE_SHAPE_COMPONENT)


    # 1) Load the level
    hydra.open_base_level()

    # 2) Create entities to hold PhysX Collider Components
    physx_collider_box_entity = create_validated_entity("physx_collider_box_entity", Tests.create_collider_entity)
    physx_collider_capsule_entity = create_validated_entity("physx_collider_capsule_entity", Tests.create_collider_entity)
    physx_collider_sphere_entity = create_validated_entity("physx_collider_sphere_entity", Tests.create_collider_entity)
    physx_collider_physicsasset_entity = create_validated_entity("physx_collider_physicsasset_entity", Tests.create_collider_entity)

    physx_shape_collider_box_entity = create_validated_entity("physx_shape_collider_box_entity", Tests.create_shape_collider_entity)
    physx_shape_collider_capsule_entity = create_validated_entity("physx_shape_collider_capsule_entity", Tests.create_shape_collider_entity)
    physx_shape_collider_cylinder_entity = create_validated_entity("physx_shape_collider_cylinder_entity", Tests.create_shape_collider_entity)
    physx_shape_collider_polygon_prism_entity = create_validated_entity("physx_shape_collider_polygon_prism_entity", Tests.create_shape_collider_entity)
    physx_shape_collider_quad_entity = create_validated_entity("physx_shape_collider_quad_entity", Tests.create_shape_collider_entity)
    physx_shape_collider_sphere_entity = create_validated_entity("physx_shape_collider_sphere_entity", Tests.create_shape_collider_entity)

    with Tracer() as section_tracer:

        # 3) Adding Collider Components to their entities
        physx_collider_box_shape_component = add_validated_component(physx_collider_box_entity, PHYSX_COLLIDER, Tests.create_physx_collider_component)
        physx_collider_capsule_shape_component = add_validated_component(physx_collider_capsule_entity, PHYSX_COLLIDER, Tests.create_physx_collider_component)
        physx_collider_sphere_shape_component = add_validated_component(physx_collider_sphere_entity, PHYSX_COLLIDER, Tests.create_physx_collider_component)
        physx_collider_physicsasset_component = add_validated_component(physx_collider_physicsasset_entity, PHYSX_COLLIDER, Tests.create_physx_collider_component)

        physx_shape_collider_box_component = add_validated_component(physx_shape_collider_box_entity, PHYSX_SHAPE_COLLIDER, Tests.add_box_shape_component)
        physx_shape_collider_capsule_component = add_validated_component(physx_shape_collider_capsule_entity, PHYSX_SHAPE_COLLIDER, Tests.add_capsule_shape_component)
        physx_shape_collider_cylinder_component = add_validated_component(physx_shape_collider_cylinder_entity, PHYSX_SHAPE_COLLIDER, Tests.add_cylinder_shape_component)
        physx_shape_collider_polygon_prism_component = add_validated_component(physx_shape_collider_polygon_prism_entity, PHYSX_SHAPE_COLLIDER, Tests.add_polygon_prism_shape_component)
        physx_shape_collider_quad_component = add_validated_component(physx_shape_collider_quad_entity, PHYSX_SHAPE_COLLIDER, Tests.add_quad_shape_component)
        physx_shape_collider_sphere_component = add_validated_component(physx_shape_collider_sphere_entity, PHYSX_SHAPE_COLLIDER, Tests.add_sphere_shape_component)

        # 4) Setting Physx Collider Shapes
        physx_collider_box_shape_component.set_component_property_value(PHSX_SHAPE_COLLIDER_SHAPE, physics.ShapeType_Box)
        physx_collider_capsule_shape_component.set_component_property_value(PHSX_SHAPE_COLLIDER_SHAPE, physics.ShapeType_Capsule)
        physx_collider_sphere_shape_component.set_component_property_value(PHSX_SHAPE_COLLIDER_SHAPE, physics.ShapeType_Sphere)

        # 5) Adding Physx Shape Collider Components to corresponding Shape Collider Entities
        box_shape_component = add_validated_component(physx_shape_collider_box_entity, BOX_SHAPE_COMPONENT, Tests.add_box_shape_component)
        capsule_shape_component = add_validated_component(physx_shape_collider_capsule_entity, CAPSULE_SHAPE_COMPONENT, Tests.add_capsule_shape_component)
        cylinder_shape_component = add_validated_component(physx_shape_collider_cylinder_entity, CYLINDER_SHAPE_COMPONENT, Tests.add_cylinder_shape_component)
        polygon_prism_shape_component = add_validated_component(physx_shape_collider_polygon_prism_entity, POLYGON_PRISM_SHAPE_COMPONENT, Tests.add_polygon_prism_shape_component)
        quad_shape_component = add_validated_component(physx_shape_collider_quad_entity, QUAD_SHAPE_COMPONENT, Tests.add_quad_shape_component)
        sphere_shape_component = add_validated_component(physx_shape_collider_sphere_entity, SPHERE_SHAPE_COMPONENT, Tests.add_sphere_shape_component)

        # 6) PhysX Collider PhysicsAsset Mesh Support
        # --Crash--
        # proptree = physx_collider_physicsasset_component.get_property_tree(True)
        # proptree_count = proptree.get_container_count()
        # proptree_list = physx_collider_physicsasset_component.get_property_type_visibility()

        # --Crash--
        # test_entity = EditorEntity.create_editor_entity("Test")
        # test_component = test_entity.add_component("PhysX Collider")
        # has_component = test_entity.has_component("PhysX Collider")
        # print(test_component.get_property_type_visibility())

        # --Crash--
        # test_entity = EditorEntity.create_editor_entity("Test")
        # test_component = test_entity.add_component("PhysX Hinge Joint")
        # print(test_component.get_property_type_visibility())

        # --Crash--
        # test_entity = EditorEntity.create_editor_entity("Test")
        # test_component = add_validated_component(test_entity, "PhysX Shape Collider", ["Test Componenet Added", "Test Component Failed."])
        # print(test_component.get_property_type_visibility())
        # output = ["", "Proptree: " + str(test_component.get_property_type_visibility())]

        # --Works--
        # test_entity = EditorEntity.create_editor_entity("Test")
        # test_component = test_entity.add_component("PhysX Collider")
        # has_component = test_entity.has_component("PhysX Collider")
        # test_tree = test_component.get_property_tree()
        # output = ["", "Proptree: " + str(test_tree.build_paths_list())]
        # print(test_tree.build_paths_list())

        # --Works--
        # position = math.Vector3(0.0, 0.0, 0.0)
        # test_entity = hydra.Entity("test")
        # test_entity.create_entity(position, ["PhysX Collider"])
        # component = test_entity.components[0]
        # print(hydra.get_property_tree(component))

        # --Works--
        # test_entity = EditorEntity.create_editor_entity("Test")
        # test_component = test_entity.add_component("PhysX Collider")
        # has_component = test_entity.has_component("PhysX Collider")
        # test_tree = test_component.get_property_tree()
        # print(test_tree.build_paths_list())
        # print(has_component)

        #Report.critical_result(output, False)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PhysX_ColliderFamily_Component_CRUD)

