"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.physics as physics
import azlmbr.math as math

from editor_python_test_tools.editor_entity_utils import EditorEntity
from editor_python_test_tools.editor_component.editor_component_validation import \
    (_validate_xyz_is_float, _validate_property_visibility, validate_property_switch_toggle)
from editor_python_test_tools.asset_utils import Asset

from consts.physics import PHYSX_COLLIDER
from consts.general import ComponentPropertyVisibilityStates as PropertyVisibility


class EditorPhysxCollider:
    """
    PhysxCollider class is used to interact with the PhysX Collider Entity Component and its properties. This class
        makes it easy for the user to interact with the PhysX Collider in a natural and easy-to-write/maintain way.
    The PhysxCollider object stores a reference to an EditorComponent and is created by passing an EditorEntity to the
    PhysxCollider object constructor PhysxCollider(EditorEntity).
    """
    def __init__(self, editor_entity: EditorEntity) -> None:
        self.component = editor_entity.add_component(PHYSX_COLLIDER)

    class Path:
        """
        A container class for the PropertyTree paths relevant to the PhysX Collider. This tree can be returned by
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
        PHYSX_MATERIAL_ASSET = ']|'  # o3de/o3de#12503 Needs a better property path
        TAG = 'Collider Configuration|Tag'
        RESET_OFFSET = 'Collider Configuration|Rest offset'
        CONTACT_OFFSET = 'Collider Configuration|Contact offset'
        DRAW_COLLIDER = 'Debug draw settings|Draw collider'
        SHAPE = 'Shape Configuration|Shape'

    # General Properties
    # o3de/o3de#12632 - Figure out how to set dropdowns for Collison Layer and Collides With

    def toggle_is_trigger(self) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to toggle the PhysX Collider's Trigger Property.
        """
        validate_property_switch_toggle(self.component, self.Path.TRIGGER)

    def toggle_is_simulated(self) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to toggle the PhysX Collider's Simulated Property.
        """
        validate_property_switch_toggle(self.component, self.Path.SIMULATED)

    def toggle_in_scene_queries(self) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to toggle the PhysX Collider's In Scene Queries Property.
        """
        validate_property_switch_toggle(self.component, self.Path.IN_SCENE_QUERIES)

    def set_offset(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to set the PhysX Collider's Offset Property.
        """
        _validate_xyz_is_float(x, y, z, f"One of the offset inputs are not a float - X: {x}, Y: {y}, Z {z}")

        self.component.set_component_property_value(self.Path.OFFSET, math.Vector3(x, y, z))

    def set_rotation(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Quaternion', 'Visible')

        Used to set the PhysX Collider's Rotation Property.
        """
        _validate_xyz_is_float(x, y, z, f"One of the rotation inputs are not a float - X: {x}, Y: {y}, Z {z}")

        rotation = math.Quaternion()
        rotation.SetFromEulerDegrees(math.Vector3(x, y, z))
        self.component.set_component_property_value(self.Path.ROTATION, rotation)

    def set_tag(self, tag: chr) -> None:
        """
        Property Type, Default Visibility - ('AZStd::string', 'Visible')

        Used to set the PhysX Collider's Tag Property.
        """
        # o3de/o3de#12634 - For some reason I can't get this to work. Says it takes an AZStd::string, but it won't take
        #  one or a character.
        assert NotImplementedError
        self.component.set_component_property_value(self.Path.TAG, tag)

    def set_rest_offset(self, rest_offset: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Collider's Reset Offset Property.
        """
        assert isinstance(rest_offset), f"The value passed to Rest Offset \"{rest_offset}\" is not a float."

        self.component.set_component_property_value(self.Path.RESET_OFFSET, rest_offset)

    def set_contact_offset(self, contact_offset: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Collider's Contact Offset Property.
        """
        assert isinstance(contact_offset), f"The value passed to Contact Offset \"{contact_offset}\" is not a float."

        self.component.set_component_property_value(self.Path.CONTACT_OFFSET, contact_offset)

    def toggle_draw_collider(self) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to toggle the PhysX Collider's Draw Collider Property.
        """
        validate_property_switch_toggle(self.component, self.Path.DRAW_COLLIDER)

    def set_physx_material_from_path(self, asset_product_path: str) -> None:
        """
        Property Type, Default Visibility - ('Asset<Physics::MaterialAsset>', 'Visible')

        Used to set the PhysX Collider's PhysX Material from a user provided asset product path. This will return an
            error if a source path is provided.
        """
        # o3de/o3de#12503 PhysX Collider Component's Physic Material field(s) return unintuitive property tree paths.
        assert NotImplementedError
        _validate_property_visibility(self.component, self.Path.PHYSX_MATERIAL_ASSET, PropertyVisibility.VISIBLE)

        px_material = Asset.find_asset_by_path(asset_product_path)
        self.component.set_component_property_value(self.Path.PHYSX_MATERIAL_ASSET, px_material.id)

    # Shape: Box
    def set_box_shape(self) -> None:
        """
        Property Type, Default Visibility - ('BoxShapeConfiguration', 'NotVisible')

        Used to set the PhysX Collider's Shape property to Box.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Box)

    def set_box_dimensions(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to set the PhysX Collider's Box Shape's dimensions.
        """
        _validate_xyz_is_float(x, y, z, f"One of the Box Dimensions are not a float - X: {x}, Y: {y}, Z {z}")
        _validate_property_visibility(self.component, self.Path.Box.DIMENSIONS, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Box.DIMENSIONS, math.Vector3(x, y, z))

    # Shape: Capsule
    def set_capsule_shape(self) -> None:
        """
        Property Type, Default Visibility - ('CapsuleShapeConfiguration', 'NotVisible')

        Used to set the PhysX Collider's Shape property to Capsule.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Capsule)

    def set_capsule_height(self, height: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Collider's Capsule Shape's height.
        """
        assert isinstance(height), f"The value passed to Capsule Height \"{height}\" is not a float."
        _validate_property_visibility(self.component, self.Path.Capsule.HEIGHT, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Capsule.HEIGHT, float(height))

    def set_capsule_radius(self, radius: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Collider's Capsule Shape's radius.
        """
        assert isinstance(radius), f"The value passed to Capsule Radius \"{radius}\" is not a float."
        _validate_property_visibility(self.component, self.Path.Capsule.RADIUS, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Capsule.RADIUS, float(radius))

    # Shape: Cylinder
    def set_cylinder_shape(self) -> None:
        """
        Property Type, Default Visibility - ('EditorProxyCylinderShapeConfig', 'NotVisible')

        Used to set the PhysX Collider's Shape property to Cylinder.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Cylinder)

    def set_cylinder_subdivision(self, subdivisions: int) -> None:
        """
        Property Type, Default Visibility - ('unsigned char', 'Visible')

        Used to set the PhysX Collider's Cylinder Shape's subdivision. Subdivision supports int values [3-125].
        """
        _validate_property_visibility(self.component, self.Path.Cylinder.SUBDIVISION, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Cylinder.SUBDIVISION, subdivisions)

    def set_cylinder_height(self, height: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Collider's Cylinder Shape's height.
        """
        assert isinstance(height), f"The value passed to Cylinder Height \"{height}\" is not a float."
        _validate_property_visibility(self.component, self.Path.Cylinder.HEIGHT, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Cylinder.HEIGHT, float(height))

    def set_cylinder_radius(self, radius: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Collider's Cylinder Shape's radius.
        """
        assert isinstance(radius), f"The value passed to Cylinder Radius \"{radius}\" is not a float."
        _validate_property_visibility(self.component, self.Path.Cylinder.RADIUS, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Cylinder.RADIUS, float(radius))

    # Shape: PhysicsAsset
    def set_physicsasset_shape(self) -> None:
        """
        Property Type, Default Visibility - ('PhysicsAssetShapeConfiguration', 'ShowChildrenOnly')

        Used to set the PhysX Collider's Shape property to PhysicsAsset.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_PhysicsAsset)

    def set_physx_mesh_from_path(self, asset_product_path: str) -> None:
        """
        Property Type, Default Visibility - ('Asset<MeshAsset>', 'Visible')

        Used to set the PhysX Collider's PhysicsAsset Shape's PhysX Mesh from a user provided asset product path.
            This will return an error if a source path is provided.
        """
        _validate_property_visibility(self.component, self.Path.PhysicsAsset.PHYSX_MESH, PropertyVisibility.VISIBLE)

        px_asset = Asset.find_asset_by_path(asset_product_path)
        self.component.set_component_property_value(self.Path.PhysicsAsset.PHYSX_MESH, px_asset.id)

    def set_physx_mesh_asset_scale(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to set the PhysX Collider's PhysX Mesh Asset Scale.
        """
        _validate_xyz_is_float(x, y, z,
                               f"One of the Physx Mesh Asset Scale inputs are not a float - X: {x}, Y: {y}, Z {z}")
        _validate_property_visibility(self.component, self.Path.PhysicsAsset.ASSET_SCALE, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.PhysicsAsset.ASSET_SCALE,
                                                    math.Vector3(float(x), float(y), float(z)))

    def toggle_use_physics_materials_from_asset(self) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')}

        Used to toggle the PhysX Collider's Physics Materials From Asset. This allows the user to toggle between
            using the asset provided PhysX Materials, or to override them using the PhysX Collider's PhysX Materials
            property.
        """
        _validate_property_visibility(self.component, self.Path.PhysicsAsset.PHYSICS_MATERIALS_FROM_ASSET,
                                      PropertyVisibility.VISIBLE)

        validate_property_switch_toggle(self.component, self.Path.PhysicsAsset.PHYSICS_MATERIALS_FROM_ASSET)

    # Shape: Sphere
    def set_sphere_shape(self) -> None:
        """
        Property Type, Default Visibility - ('SphereShapeConfiguration', 'NotVisible')

        Used to set the PhysX Collider's Shape property to Sphere.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Sphere)

    def set_sphere_radius(self, radius: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Collider's Sphere radius property.
        """
        assert isinstance(radius), f"The value passed to Sphere Radius \"{radius}\" is not a float."
        _validate_property_visibility(self.component, self.Path.Sphere.RADIUS, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Sphere.RADIUS, radius)
