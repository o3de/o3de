"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.physics as physics
import azlmbr.math as math
import azlmbr.asset as azasset

from editor_python_test_tools.editor_entity_utils import EditorEntity
from editor_python_test_tools.editor_component.editor_component_validation import \
    (_validate_xyz_is_float, _validate_property_visibility)
from editor_python_test_tools.asset_utils import Asset

from consts.physics import PHYSX_PRIMITIVE_COLLIDER
from consts.general import ComponentPropertyVisibilityStates as PropertyVisibility


class EditorPhysxPrimitiveCollider:
    """
    EditorPhysxPrimitiveCollider class is used to interact with the PhysX Primitive Collider component and its properties. This class
        makes it easy for the user to interact with the PhysX Primitive Collider in a natural and easy-to-write/maintain way.
    The EditorPhysxPrimitiveCollider object stores a reference to an EditorComponent and is created by passing an EditorEntity to the
    EditorPhysxPrimitiveCollider object constructor EditorPhysxPrimitiveCollider(EditorEntity).
    """
    def __init__(self, editor_entity: EditorEntity) -> None:
        self.component = editor_entity.add_component(PHYSX_PRIMITIVE_COLLIDER)

    class Path:
        """
        A container class for the PropertyTree paths relevant to the PhysX Primitive Collider. This tree can be returned by
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

    def set_is_trigger(self, value: bool) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to set the PhysX Primitive Collider's Is Trigger Property.
        """
        self.component.set_component_property_value(self.Path.TRIGGER, value)

    def get_is_trigger(self) -> bool:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to get the PhysX Primitive Collider's Trigger Property.
        """
        return self.component.get_component_property_value(self.Path.TRIGGER)

    def set_is_simulated(self, value: bool) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to set the PhysX Primitive Collider's Simulated Property value.
        """
        self.component.set_component_property_value(self.Path.SIMULATED, value)

    def get_is_simulated(self) -> bool:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to get the PhysX Primitive Collider's Simulated Property value.
        """
        return self.component.get_component_property_value(self.Path.SIMULATED)

    def set_in_scene_queries(self, value: bool) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to set the PhysX Primitive Collider's In Scene Queries Property value.
        """
        self.component.set_component_property_value(self.Path.IN_SCENE_QUERIES, value)

    def get_in_scene_queries(self) -> bool:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to get the PhysX Primitive Collider's In Scene Queries Property value.
        """
        return self.component.get_component_property_value(self.Path.IN_SCENE_QUERIES)

    def set_offset(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to set the PhysX Primitive Collider's Offset Property.
        """
        _validate_xyz_is_float(x, y, z, f"One of the offset inputs are not a float - X: {x}, Y: {y}, Z {z}")

        self.component.set_component_property_value(self.Path.OFFSET, math.Vector3(x, y, z))

    def get_offset(self) -> math.Vector3:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to get the PhysX Primitive Collider's Offset Property value.
        """
        return self.component.get_component_property_value(self.Path.OFFSET)

    def set_rotation(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Quaternion', 'Visible')

        Used to set the PhysX Primitive Collider's Rotation Property.
        """
        _validate_xyz_is_float(x, y, z, f"One of the rotation inputs are not a float - X: {x}, Y: {y}, Z {z}")

        rotation = math.Quaternion()
        rotation.SetFromEulerDegrees(math.Vector3(x, y, z))
        self.component.set_component_property_value(self.Path.ROTATION, rotation)

    def get_rotation(self) -> math.Quaternion:
        """
        Property Type, Default Visibility - ('Quaternion', 'Visible')

        Used to get the PhysX Primitive Collider's Rotation Property value.
        """
        return self.component.get_component_property_value(self.Path.ROTATION)

    def set_tag(self, tag: chr) -> None:
        """
        Property Type, Default Visibility - ('AZStd::string', 'Visible')

        Used to set the PhysX Primitive Collider's Tag Property.
        """
        # o3de/o3de#12634 - For some reason I can't get this to work. Says it takes an AZStd::string, but it won't take
        #  one or a character.
        assert NotImplementedError
        self.component.set_component_property_value(self.Path.TAG, tag)

    def get_tag(self) -> chr:
        """
        Property Type, Default Visibility - ('AZStd::string', 'Visible')

        Used to get the PhysX Primitive Collider's Tag Property value.
        """
        return self.component.get_component_property_value(self.Path.TAG)

    def set_rest_offset(self, rest_offset: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Primitive Collider's Reset Offset Property.

        NOTE: Rest Offset Must be LESS-THAN Contact Offset.
        """
        assert isinstance(rest_offset, float), f"The value passed to Rest Offset \"{rest_offset}\" is not a float."

        self.component.set_component_property_value(self.Path.RESET_OFFSET, rest_offset)

    def get_rest_offset(self) -> float:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to get the PhysX Primitive Collider's Reset Offset Property value.
        """
        return self.component.get_component_property_value(self.Path.RESET_OFFSET)

    def set_contact_offset(self, contact_offset: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Primitive Collider's Contact Offset Property.

        NOTE: Contact Offset Must be GREATER-THAN Rest Offset.
        """
        assert isinstance(contact_offset, float), f"The value passed to Contact Offset \"{contact_offset}\" is not a float."

        self.component.set_component_property_value(self.Path.CONTACT_OFFSET, contact_offset)

    def get_contact_offset(self) -> float:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to get the PhysX Primitive Collider's Contact Offset Property value.
        """
        return self.component.get_component_property_value(self.Path.CONTACT_OFFSET)

    def set_draw_collider(self, value: bool) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to set the PhysX Primitive Collider's Draw Collider Property value.
        """
        self.component.set_component_property_value(self.Path.DRAW_COLLIDER, value)

    def get_draw_collider(self) -> bool:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to get the PhysX Primitive Collider's Draw Collider Property value.
        """
        return self.component.get_component_property_value(self.Path.DRAW_COLLIDER)

    def set_physx_material_from_path(self, asset_product_path: str) -> None:
        """
        Property Type, Default Visibility - ('Asset<Physics::MaterialAsset>', 'Visible')

        Used to set the PhysX Primitive Collider's PhysX Material from a user provided asset product path. This will return an
            error if a source path is provided.
        """
        # o3de/o3de#12503 PhysX Primitive Collider Component's Physic Material field(s) return unintuitive property tree paths.
        assert NotImplementedError
        _validate_property_visibility(self.component, self.Path.PHYSX_MATERIAL_ASSET, PropertyVisibility.VISIBLE)

        px_material = Asset.find_asset_by_path(asset_product_path)
        self.component.set_component_property_value(self.Path.PHYSX_MATERIAL_ASSET, px_material.id)

    def get_physx_material(self) -> Asset:
        """
        Property Type, Default Visibility - ('Asset<Physics::MaterialAsset>', 'Visible')

        Used to get the PhysX Primitive Collider's PhysX Material
        """
        # o3de/o3de#12503 PhysX Primitive Collider Component's Physic Material field(s) return unintuitive property tree paths.
        assert NotImplementedError

        return self.component.get_component_property_value(self.Path.PHYSX_MATERIAL_ASSET)

    def get_shape(self) -> str:
        """
        Property Type, Default Visibility - ('BoxShapeConfiguration', 'NotVisible')

        Used to set the PhysX Primitive Collider's Shape property to Box.
        """
        return str(self.component.get_component_property_value(self.Path.SHAPE))

    # Shape: Box
    def set_box_shape(self) -> None:
        """
        Property Type, Default Visibility - ('BoxShapeConfiguration', 'NotVisible')

        Used to set the PhysX Primitive Collider's Shape property to Box.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Box)

    def set_box_dimensions(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to set the PhysX Primitive Collider's Box Shape's dimensions.
        """
        _validate_xyz_is_float(x, y, z, f"One of the Box Dimensions are not a float - X: {x}, Y: {y}, Z {z}")
        _validate_property_visibility(self.component, self.Path.Box.DIMENSIONS, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Box.DIMENSIONS, math.Vector3(x, y, z))

    def get_box_dimensions(self) -> math.Vector3:
        """
        Property Type, Default Visibility - ('BoxShapeConfiguration', 'NotVisible')

        Used to get the PhysX Primitive Collider's Box Shape's dimensions.
        """
        _validate_property_visibility(self.component, self.Path.Box.DIMENSIONS, PropertyVisibility.VISIBLE)
        return self.component.get_component_property_value(self.Path.Box.DIMENSIONS)

    # Shape: Capsule
    def set_capsule_shape(self) -> None:
        """
        Property Type, Default Visibility - ('CapsuleShapeConfiguration', 'NotVisible')

        Used to set the PhysX Primitive Collider's Shape property to Capsule.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Capsule)

    def set_capsule_height(self, height: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Primitive Collider's Capsule Shape's height.
        """
        assert isinstance(height, float), f"The value passed to Capsule Height \"{height}\" is not a float."
        _validate_property_visibility(self.component, self.Path.Capsule.HEIGHT, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Capsule.HEIGHT, height)

    def get_capsule_height(self) -> float:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to get the PhysX Primitive Collider's Capsule Shape's height.
        """
        _validate_property_visibility(self.component, self.Path.Capsule.HEIGHT, PropertyVisibility.VISIBLE)
        return self.component.get_component_property_value(self.Path.Capsule.HEIGHT)

    def set_capsule_radius(self, radius: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Primitive Collider's Capsule Shape's radius.
        """
        assert isinstance(radius, float), f"The value passed to Capsule Radius \"{radius}\" is not a float."
        _validate_property_visibility(self.component, self.Path.Capsule.RADIUS, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Capsule.RADIUS, radius)

    def get_capsule_radius(self) -> float:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to get the PhysX Primitive Collider's Capsule Shape's radius.
        """
        _validate_property_visibility(self.component, self.Path.Capsule.RADIUS, PropertyVisibility.VISIBLE)
        return self.component.get_component_property_value(self.Path.Capsule.RADIUS)

    # Shape: Cylinder
    def set_cylinder_shape(self) -> None:
        """
        Property Type, Default Visibility - ('EditorProxyCylinderShapeConfig', 'NotVisible')

        Used to set the PhysX Primitive Collider's Shape property to Cylinder.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Cylinder)

    def set_cylinder_subdivision(self, subdivisions: int) -> None:
        """
        Property Type, Default Visibility - ('unsigned char', 'Visible')

        Used to set the PhysX Primitive Collider's Cylinder Shape's subdivision. Subdivision supports int values [3-125].
        """
        _validate_property_visibility(self.component, self.Path.Cylinder.SUBDIVISION, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Cylinder.SUBDIVISION, subdivisions)

    def get_cylinder_subdivision(self) -> int:
        """
        Property Type, Default Visibility - ('unsigned char', 'Visible')

        Used to get the PhysX Primitive Collider's Cylinder Shape's subdivision. Subdivision supports int values [3-125].
        """
        _validate_property_visibility(self.component, self.Path.Cylinder.SUBDIVISION, PropertyVisibility.VISIBLE)
        return self.component.get_component_property_value(self.Path.Cylinder.SUBDIVISION)

    def set_cylinder_height(self, height: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Primitive Collider's Cylinder Shape's height.
        """
        assert isinstance(height, float), f"The value passed to Cylinder Height \"{height}\" is not a float."
        _validate_property_visibility(self.component, self.Path.Cylinder.HEIGHT, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Cylinder.HEIGHT, height)

    def get_cylinder_height(self) -> float:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to get the PhysX Primitive Collider's Cylinder Shape's height.
        """
        _validate_property_visibility(self.component, self.Path.Cylinder.HEIGHT, PropertyVisibility.VISIBLE)
        return self.component.get_component_property_value(self.Path.Cylinder.HEIGHT)

    def set_cylinder_radius(self, radius: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Primitive Collider's Cylinder Shape's radius.
        """
        assert isinstance(radius, float), f"The value passed to Cylinder Radius \"{radius}\" is not a float."
        _validate_property_visibility(self.component, self.Path.Cylinder.RADIUS, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Cylinder.RADIUS, radius)

    def get_cylinder_radius(self) -> float:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to get the PhysX Primitive Collider's Cylinder Shape's radius.
        """
        _validate_property_visibility(self.component, self.Path.Cylinder.RADIUS, PropertyVisibility.VISIBLE)
        return self.component.get_component_property_value(self.Path.Cylinder.RADIUS)

    # Shape: Sphere
    def set_sphere_shape(self) -> None:
        """
        Property Type, Default Visibility - ('SphereShapeConfiguration', 'NotVisible')

        Used to set the PhysX Primitive Collider's Shape property to Sphere.
        """
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Sphere)

    def set_sphere_radius(self, radius: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Primitive Collider's Sphere radius property.
        """
        assert isinstance(radius, float), f"The value passed to Sphere Radius \"{radius}\" is not a float."
        _validate_property_visibility(self.component, self.Path.Sphere.RADIUS, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.Sphere.RADIUS, radius)

    def get_sphere_radius(self) -> float:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to get the PhysX Primitive Collider's Sphere radius property.
        """
        _validate_property_visibility(self.component, self.Path.Sphere.RADIUS, PropertyVisibility.VISIBLE)
        return self.component.get_component_property_value(self.Path.Sphere.RADIUS)
