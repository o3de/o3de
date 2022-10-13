"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
PhysX Collider Property Tree

'Component Mode': ('ComponentModeDelegate', 'ShowChildrenOnly'), 
'Collider Configuration': ('ColliderConfiguration', 'ShowChildrenOnly'),
'Collider Configuration|Collision Layer': ('CollisionLayer', 'Visible'),
'Collider Configuration|Collides With': ('Id', 'Visible'), 
'Collider Configuration|Trigger': ('bool', 'Visible'),
'Collider Configuration|Simulated': ('bool', 'Visible'),
'Collider Configuration|In Scene Queries': ('bool', 'Visible'), 
'Collider Configuration|Offset': ('Vector3', 'Visible'), 
'Collider Configuration|Rotation': ('Quaternion', 'Visible'), 

'Collider Configuration|': ('Physics::MaterialSlots', 'ShowChildrenOnly'), 
']': ('Physics::MaterialSlots::MaterialSlot', 'ShowChildrenOnly'), 
']|': ('Asset<Physics::MaterialAsset>', 'Visible'), 

'Collider Configuration|Tag': ('AZStd::string', 'Visible'), 
'Collider Configuration|Rest offset': ('float', 'Visible'), 
'Collider Configuration|Contact offset': ('float', 'Visible'),
'Debug draw settings': ('Collider', 'ShowChildrenOnly'), 
'Debug draw settings|Draw collider': ('bool', 'Visible'), 
 
'Shape Configuration': ('EditorProxyShapeConfig', 'ShowChildrenOnly'), 
'Shape Configuration|Subdivision level': ('unsigned char', 'NotVisible'),
'Shape Configuration|Shape': ('unsigned char', 'Visible'),

--- Asset
'Shape Configuration|Asset': ('EditorProxyAssetShapeConfig', 'Visible'),
'Shape Configuration|Asset|Configuration': ('PhysicsAssetShapeConfiguration', 'ShowChildrenOnly'),  
'Shape Configuration|Asset|PhysX Mesh': ('Asset<MeshAsset>', 'Visible'), 
'Shape Configuration|Asset|Configuration|Asset Scale': ('Vector3', 'Visible'),
'Shape Configuration|Asset|Configuration|Physics materials from asset': ('bool', 'Visible')}

---Capsule
'Shape Configuration|Capsule': ('CapsuleShapeConfiguration', 'NotVisible'),  
'Shape Configuration|Capsule|Height': ('float', 'Visible'),
'Shape Configuration|Capsule|Radius': ('float', 'Visible'), 
 
---Box
'Shape Configuration|Box': ('BoxShapeConfiguration', 'NotVisible'),
'Shape Configuration|Box|Dimensions': ('Vector3', 'Visible'), 

---Sphere
'Shape Configuration|Sphere': ('SphereShapeConfiguration', 'NotVisible'), 
'Shape Configuration|Sphere|Radius': ('float', 'Visible'), 

---Cylinder
'Shape Configuration|Cylinder': ('EditorProxyCylinderShapeConfig', 'NotVisible'), 
'Shape Configuration|Cylinder|Configuration': ('CookedMeshShapeConfiguration', 'ShowChildrenOnly'),  
'Shape Configuration|Cylinder|Radius': ('float', 'Visible'), 
'Shape Configuration|Cylinder|Height': ('float', 'Visible'), 
'Shape Configuration|Cylinder|Subdivision': ('unsigned char', 'Visible'), 


"""
import azlmbr.physics as physics
import azlmbr.math as math
import azlmbr.legacy.general as general

from editor_python_test_tools.editor_entity_utils import EditorEntity
from editor_python_test_tools.asset_utils import Asset
from editor_python_test_tools.utils import TestHelper as helper

COMPONENT_NAME = "PhysX Collider"

class PhysxCollider:
    def __init__(self, editor_entity: EditorEntity) -> None:
        self.component = editor_entity.add_component(COMPONENT_NAME)

    class Path:
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
        DRAW_COLLIDER = 'Debug draw settings'
        SHAPE = 'Shape Configuration|Shape'

    # General Properties
    #TODO: Need to figure out how to set dropdowns for Collison Layer and Collides With

    def toggle_trigger(self):
        self.component.toggle_property_switch(self.Path.TRIGGER)

    def toggle_simulated(self):
        self.component.toggle_property_switch(self.Path.SIMULATED)

    def toggle_in_scene_queries(self):
        self.component.toggle_property_switch(self.Path.IN_SCENE_QUERIES)

    def set_offset(self, x: float, y: float, z: float):
        self.component.set_component_property_value(self.Path.OFFSET, math.Vector3(x, y, z))

    def set_rotation(self, x: float, y: float, z: float):
        self.component.set_component_property_value(self.Path.ROTATION, math.Vector3(x, y, z))

    def set_tag(self, tag: str):
        self.component.set_component_property_value(self.Path.TAG, tag)

    def set_rest_offset(self, offset: float):
        self.component.set_component_property_value(self.Path.RESET_OFFSET, offset)

    def set_contact_offset(self, offset: float):
        self.component.set_component_property_value(self.Path.CONTACT_OFFSET, offset)

    def toggle_draw_collider(self):
        self.component.toggle_property_switch(self.Path.DRAW_COLLIDER)

    def set_physx_material_from_path(self, asset_product_path: str) -> None:
        assert self.component.is_property_visible(self.Path.PHYSX_MATERIAL_ASSET), \
            f"Failure: Cannot set box dimensions when property is not visible."

        px_material = Asset.find_asset_by_path(asset_product_path)
        self.component.set_component_property_value(self.Path.PHYSX_MATERIAL_ASSET, px_material.id)

    # Shape: Box
    def set_box_shape(self) -> None:
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Box)

    def set_box_dimensions(self, x: float, y: float, z: float) -> None:
        assert self.component.is_property_visible(self.Path.Box.DIMENSIONS), \
            f"Failure: Cannot set box dimensions when property is not visible."

        self.component.set_component_property_value(self.Path.Box.DIMENSIONS, math.Vector3(x, y, z))

    # Shape: Capsule
    def set_capsule_shape(self) -> None:
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Capsule)

    def set_capsule_height(self, height: str) -> None:
        assert self.component.is_property_visible(self.Path.Capsule.HEIGHT), \
            f"Failure: Cannot set capsule height when property is not visible. Set the shape to capsule first."

        self.component.set_component_property_value(self.Path.Capsule.HEIGHT, height)

    def set_capsule_radius(self, radius: float) -> None:
        assert self.component.is_property_visible(self.Path.Capsule.RADIUS), \
            f"Failure: Cannot set capsule radius when property is not visible. Set the shape to capsule first."

        self.component.set_component_property_value(self.Path.Capsule.HEIGHT, radius)

    # Shape: Cylinder
    def set_cylinder_shape(self) -> None:
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Cylinder)

    def set_cylinder_subdivision(self, subdivisions: int):
        assert self.component.is_property_visible(self.Path.Cylinder.SUBDIVISION), \
            f"Failure: Cannot set cylinder subdivisions when property is not visible. Set the shape to cylinder first."

        self.component.set_component_property_value(self.Path.Cylinder.SUBDIVISION, subdivisions)

    def set_cylinder_height(self, height: float):
        assert self.component.is_property_visible(self.Path.Cylinder.HEIGHT), \
            f"Failure: Cannot set cylinder height when property is not visible. Set the shape to cylinder first."

        self.component.set_component_property_value(self.Path.Cylinder.HEIGHT, height)

    def set_cylinder_radius(self, radius: float):
        assert self.component.is_property_visible(self.Path.Cylinder.RADIUS), \
            f"Failure: Cannot set cylinder radius when property is not visible. Set the shape to cylinder first."

        self.component.set_component_property_value(self.Path.Cylinder.RADIUS, radius)

    # Shape: PhysicsAsset
    def set_physicsasset_shape(self) -> None:
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_PhysicsAsset)

    def set_physx_mesh_from_path(self, asset_product_path: str) -> None:
        assert self.component.is_property_visible(self.Path.PhysicsAsset.PHYSX_MESH), \
            f"Failure: Cannot set Physics Mesh when property is not visible."

        px_asset = Asset.find_asset_by_path(asset_product_path)
        self.component.set_component_property_value(self.Path.PhysicsAsset.PHYSX_MESH, px_asset.id)

    def set_physx_mesh_asset_scale(self, x: float, y: float, z: float) -> None:
        assert self.component.is_property_visible(self.Path.PhysicsAsset.ASSET_SCALE), \
            f"Failure: Cannot set Physics Mesh Asset Scale when property is not visible."

        self.component.set_component_property_value(self.Path.PhysicsAsset.ASSET_SCALE, math.Vector3(x, y, z))

    def toggle_physics_materials_from_asset(self) -> None:
        assert self.component.is_property_visible(self.Path.PhysicsAsset.PHYSICS_MATERIALS_FROM_ASSET), \
            f"Failure: Cannot toggle Physics Materials From Asset when property is not visible."

        self.component.toggle_property_switch(self.Path.PhysicsAsset.PHYSICS_MATERIALS_FROM_ASSET)

    # Shape: Sphere
    def set_sphere_shape(self) -> None:
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Sphere)

    def set_sphere_radius(self, radius) -> None:
        self.component.set_component_property_value(self.Path.Sphere.RADIUS, radius)
