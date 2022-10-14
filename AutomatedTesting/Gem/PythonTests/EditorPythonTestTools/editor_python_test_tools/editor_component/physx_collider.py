"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.physics as physics
import azlmbr.math as math

from editor_python_test_tools.editor_entity_utils import EditorEntity
from editor_python_test_tools.asset_utils import Asset

COMPONENT_NAME = "PhysX Collider"

class PhysxCollider:
    """
    PhysxCollider class is used to interact with the PhysXCollider Entity Componenet and its properties. This class
        makes it easy for the user to interact with the PhysxCollider in a natural and easy-to-write/maintain way.
    The PhysxCollider object stores a reference to an EditorComponenet and is created by passing an EditorEntity to the
    PhysxCollider constructor via PhysxCollider(EditorEntity).
    """
    def __init__(self, editor_entity: EditorEntity) -> None:
        self.component = editor_entity.add_component(COMPONENT_NAME)

    class Path:
        """
        A container class for the PropertyTree paths relevant to the Physx Collider. This tree can be returned by
            calling self.component.get_property_type_visibility() which will also return the expected data type
            and visibility.
        """
        class Box:
            BASE = 'Shape Configuration|Box'
            DIMENSIONS = 'Shape Configuration|Box|Dimensions'
        class Sphere:
            BASE = 'Shape Configuration|Sphere'
            RADIUS = 'Shape Configuration|Sphere|Radius'
        class Capsule:
            BASE = 'Shape Configuration|Capsule'
            HEIGHT = 'Shape Configuration|Capsule|Height'
            RADIUS = 'Shape Configuration|Capsule|Radius'
        class Cylinder:
            BASE = 'Shape Configuration|Cylinder'
            RADIUS = 'Shape Configuration|Cylinder|Radius'
            HEIGHT = 'Shape Configuration|Cylinder|Height'
            SUBDIVISION = 'Shape Configuration|Cylinder|Subdivision'
        class PhysicsAsset:
            BASE = 'Shape Configuration|Asset'
            PHYSX_MESH = 'Shape Configuration|Asset|PhysX Mesh'
            ASSET_SCALE = 'Shape Configuration|Asset|Configuration|Asset Scale'
            PHYSICS_MATERIALS_FROM_ASSET = 'Shape Configuration|Asset|Configuration|Physics materials from asset'

        COLLISION_LAYER = 'Collider Configuration|Collision Layer'
        COLLIDES_WITH = 'Collider Configuration|Collides With'
        TRIGGER = 'Collider Configuration|Trigger'
        SIMULATED = 'Collider Configuration|Simulated'
        IN_SCENE_QUERIES = 'Collider Configuration|In Scene Queries'
        OFFSET = 'Collider Configuration|Offset'
        ROTATION = 'Collider Configuration|Rotation'
        PHYSX_MATERIAL_ASSET = ']|'  # GH-12503 Needs a better property path
        TAG = 'Collider Configuration|Tag'
        RESET_OFFSET = 'Collider Configuration|Rest offset'
        CONTACT_OFFSET = 'Collider Configuration|Contact offset'
        DRAW_COLLIDER = 'Debug draw settings|Draw collider'
        SHAPE = 'Shape Configuration|Shape'

    # General Properties
    #TODO: GHI #12632 - Figure out how to set dropdowns for Collison Layer and Collides With

    def toggle_trigger(self) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to toggle the Physx Collider's Trigger Property.
        """
        self.component.toggle_property_switch(self.Path.TRIGGER)

    def toggle_simulated(self) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to toggle the Physx Collider's Simulated Property.
        """
        self.component.toggle_property_switch(self.Path.SIMULATED)

    def toggle_in_scene_queries(self) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to toggle the Physx Collider's In Scene Queries Property.
        """
        self.component.toggle_property_switch(self.Path.IN_SCENE_QUERIES)

    def set_offset(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to set the Physx Collider's Offset Property.
        """
        self.component.set_component_property_value(self.Path.OFFSET, math.Vector3(float(x), float(y), float(z)))

    def set_rotation(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Quaternion', 'Visible')

        Used to set the Physx Collider's Rotation Property.
        """
        # TODO: GHI #12633 - Figure out how to build a Quaternion
        assert NotImplementedError
        self.component.set_component_property_value(self.Path.ROTATION, math.Quaternion(float(x), float(y), float(z)))

    def set_tag(self, tag: chr) -> None:
        """
        Property Type, Default Visibility - ('AZStd::string', 'Visible')

        Used to set the Physx Collider's Tag Property.
        """
        # TODO: GHI #12634 - For some reason I can't get this to work. Says it takes an AZStd::string, but it won't take
        #  one or a character.
        assert NotImplementedError
        self.component.set_component_property_value(self.Path.TAG, tag)

    def set_rest_offset(self, offset: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the Physx Collider's Reset Offset Property.
        """
        self.component.set_component_property_value(self.Path.RESET_OFFSET, offset)

    def set_contact_offset(self, offset: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the Physx Collider's Contact Offset Property.
        """
        self.component.set_component_property_value(self.Path.CONTACT_OFFSET, offset)

    def toggle_draw_collider(self) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to toggle the Physx Collider's Draw Collider Property.
        """
        self.component.toggle_property_switch(self.Path.DRAW_COLLIDER)

    def set_physx_material_from_path(self, asset_product_path: str) -> None:
        """
        Property Type, Default Visibility - ('Asset<Physics::MaterialAsset>', 'Visible')

        Used to set the Physx Collider's PhysX Material from a user provided asset product path. This will return an
            error if a source path is provided.
        """
        # GHI #12503 PhysX Collider Component's Physic Material field(s) return unintuitive property tree paths.
        assert NotImplementedError
        assert self.component.is_property_visible(self.Path.PHYSX_MATERIAL_ASSET), \
            f"Failure: Cannot set Physx Material when property is not visible."

        px_material = Asset.find_asset_by_path(asset_product_path)
        self.component.set_component_property_value(self.Path.PHYSX_MATERIAL_ASSET, px_material.id)

    # Shape: Box
    def set_box_shape(self) -> None:
        """
        Property Type, Default Visibility - ('BoxShapeConfiguration', 'NotVisible')

        Used to set the Physx Collider's Shape property to Box.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Box)

    def set_box_dimensions(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to set the Physx Collider's Box Shape's dimensions.
        """
        assert self.component.is_property_visible(self.Path.Box.DIMENSIONS), \
            f"Failure: Cannot set box dimensions when property is not visible."

        self.component.set_component_property_value(self.Path.Box.DIMENSIONS,
                                                    math.Vector3(float(x), float(y), float(z)))

    # Shape: Capsule
    def set_capsule_shape(self) -> None:
        """
        Property Type, Default Visibility - ('CapsuleShapeConfiguration', 'NotVisible')

        Used to set the Physx Collider's Shape property to Capsule.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Capsule)

    def set_capsule_height(self, height: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the Physx Collider's Capsule Shape's height.
        """
        assert self.component.is_property_visible(self.Path.Capsule.HEIGHT), \
            f"Failure: Cannot set capsule height when property is not visible. Set the shape to capsule first."

        self.component.set_component_property_value(self.Path.Capsule.HEIGHT, float(height))

    def set_capsule_radius(self, radius: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the Physx Collider's Capsule Shape's radius.
        """
        assert self.component.is_property_visible(self.Path.Capsule.RADIUS), \
            f"Failure: Cannot set capsule radius when property is not visible. Set the shape to capsule first."

        self.component.set_component_property_value(self.Path.Capsule.HEIGHT, float(radius))

    # Shape: Cylinder
    def set_cylinder_shape(self) -> None:
        """
        Property Type, Default Visibility - ('EditorProxyCylinderShapeConfig', 'NotVisible')

        Used to set the Physx Collider's Shape property to Cylinder.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Cylinder)

    def set_cylinder_subdivision(self, subdivisions: int):
        """
        Property Type, Default Visibility - ('unsigned char', 'Visible')

        Used to set the Physx Collider's Cylinder Shape's subdivision. Subdivision supports int values [3-125].
        """
        assert self.component.is_property_visible(self.Path.Cylinder.SUBDIVISION), \
            f"Failure: Cannot set cylinder subdivisions when property is not visible. Set the shape to cylinder first."

        self.component.set_component_property_value(self.Path.Cylinder.SUBDIVISION, subdivisions)

    def set_cylinder_height(self, height: float):
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the Physx Collider's Cylinder Shape's height.
        """
        assert self.component.is_property_visible(self.Path.Cylinder.HEIGHT), \
            f"Failure: Cannot set cylinder height when property is not visible. Set the shape to cylinder first."

        self.component.set_component_property_value(self.Path.Cylinder.HEIGHT, float(height))

    def set_cylinder_radius(self, radius: float):
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the Physx Collider's Cylinder Shape's radius.
        """
        assert self.component.is_property_visible(self.Path.Cylinder.RADIUS), \
            f"Failure: Cannot set cylinder radius when property is not visible. Set the shape to cylinder first."

        self.component.set_component_property_value(self.Path.Cylinder.RADIUS, float(radius))

    # Shape: PhysicsAsset
    def set_physicsasset_shape(self) -> None:
        """
        Property Type, Default Visibility - ('PhysicsAssetShapeConfiguration', 'ShowChildrenOnly')

        Used to set the Physx Collider's Shape property to PhysicsAsset.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_PhysicsAsset)

    def set_physx_mesh_from_path(self, asset_product_path: str) -> None:
        """
        Property Type, Default Visibility - ('Asset<MeshAsset>', 'Visible')

        Used to set the Physx Collider's PhysicsAsset Shape's Physx Mesh from a user provided asset product path.
            This will return an error if a source path is provided.
        """
        assert self.component.is_property_visible(self.Path.PhysicsAsset.PHYSX_MESH), \
            f"Failure: Cannot set Physics Mesh when property is not visible."

        px_asset = Asset.find_asset_by_path(asset_product_path)
        self.component.set_component_property_value(self.Path.PhysicsAsset.PHYSX_MESH, px_asset.id)

    def set_physx_mesh_asset_scale(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to set the Physx Collider's Physx Mesh Asset Scale.
        """
        assert self.component.is_property_visible(self.Path.PhysicsAsset.ASSET_SCALE), \
            f"Failure: Cannot set Physics Mesh Asset Scale when property is not visible."

        self.component.set_component_property_value(self.Path.PhysicsAsset.ASSET_SCALE,
                                                    math.Vector3(float(x), float(y), float(z)))

    def toggle_physics_materials_from_asset(self) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')}

        Used to toggle the Physx Collider's Physics Materials From Asset. This allows the user to toggle between
            using the asset provided PhysX Materials, or to override them using the PhysX Collider's PhysX Materials
            property.
        """
        assert self.component.is_property_visible(self.Path.PhysicsAsset.PHYSICS_MATERIALS_FROM_ASSET), \
            f"Failure: Cannot toggle Physics Materials From Asset when property is not visible."

        self.component.toggle_property_switch(self.Path.PhysicsAsset.PHYSICS_MATERIALS_FROM_ASSET)

    # Shape: Sphere
    def set_sphere_shape(self) -> None:
        """
        Property Type, Default Visibility - ('SphereShapeConfiguration', 'NotVisible')

        Used to set the Physx Collider's Shape property to Sphere.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Sphere)

    def set_sphere_radius(self, radius) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the Physx Collider's Sphere radius property.
        """
        self.component.set_component_property_value(self.Path.Sphere.RADIUS, radius)
