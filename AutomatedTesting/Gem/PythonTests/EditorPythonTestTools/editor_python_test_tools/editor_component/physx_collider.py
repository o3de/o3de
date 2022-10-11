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

from editor_python_test_tools.editor_entity_utils import EditorComponent, EditorEntity
from editor_python_test_tools.asset_utils import Asset
from editor_python_test_tools.utils import TestHelper as helper
from Physics.utils.physics_constants import (PHYSX_COLLIDER)


class PhysxCollider:
    def __init__(self, editor_entity: EditorEntity) -> None:
        self.component = editor_entity.add_component(PHYSX_COLLIDER)

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

    # Shape: Box
    def set_box_shape(self) -> None:
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Box)

    def set_box_dimensions(self, x: float, y: float, z: float) -> None:
        self.component.set_vector3_component_property(self.Path.Box.DIMENSIONS, math.Vector3(x, y, z))

    # Shape: PhysicsAsset
    # Once we can set and check for PhysicsAsset, error checking can be performed.
    def set_physicsasset_shape(self) -> None:
        raise NotImplementedError("GHI#<INSERT_HERE> - PhysicsAsset needs to be exposed to Python.")

    def set_physx_mesh_from_path(self, asset_product_path: str) -> None:
        px_asset = Asset.find_asset_by_path(asset_product_path)
        self.component.set_component_property_value(self.Path.PhysicsAsset.PHYSX_MESH, px_asset.id)

    def set_physx_mesh_asset_scale(self, x: float, y: float, z: float) -> None:
        position = math.Vector3(x, y, z)
        self.component.set_component_property_value(self.path.PhysicsAsset.ASSET_SCALE, position)

    def toggle_physics_materials_from_asset(self) -> None:
        self.component.toggle_component_switch(self.path.PhysicsAsset.PHYSICS_MATERIALS_FROM_ASSET)

    def set_capsule_shape(self) -> None:
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Capsule)

    def set_cylinder_shape(self) -> None:
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Cylinder)

    def set_sphere_shape(self) -> None:
        self.component.set_component_property_value(self.Path.SHAPE, physics.ShapeType_Sphere)










