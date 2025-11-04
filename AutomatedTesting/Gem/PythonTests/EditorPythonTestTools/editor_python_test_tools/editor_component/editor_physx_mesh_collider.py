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

from consts.physics import PHYSX_MESH_COLLIDER
from consts.general import ComponentPropertyVisibilityStates as PropertyVisibility


class EditorPhysxMeshCollider:
    """
    EditorPhysxMeshCollider class is used to interact with the PhysX Mesh Collider component and its properties. This class
        makes it easy for the user to interact with the PhysX Mesh Collider in a natural and easy-to-write/maintain way.
    The EditorPhysxMeshCollider object stores a reference to an EditorComponent and is created by passing an EditorEntity to the
    EditorPhysxMeshCollider object constructor EditorPhysxMeshCollider(EditorEntity).
    """
    def __init__(self, editor_entity: EditorEntity) -> None:
        self.component = editor_entity.add_component(PHYSX_MESH_COLLIDER)

    class Path:
        """
        A container class for the PropertyTree paths relevant to the PhysX Mesh Collider. This tree can be returned by
            calling self.component.get_property_type_visibility() which will also return the expected data type
            and visibility.
        """
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

    # General Properties
    # o3de/o3de#12632 - Figure out how to set dropdowns for Collison Layer and Collides With

    def set_is_trigger(self, value: bool) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to set the PhysX Mesh Collider's Is Trigger Property.
        """
        self.component.set_component_property_value(self.Path.TRIGGER, value)

    def get_is_trigger(self) -> bool:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to get the PhysX Mesh Collider's Trigger Property.
        """
        return self.component.get_component_property_value(self.Path.TRIGGER)

    def set_is_simulated(self, value: bool) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to set the PhysX Mesh Collider's Simulated Property value.
        """
        self.component.set_component_property_value(self.Path.SIMULATED, value)

    def get_is_simulated(self) -> bool:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to get the PhysX Mesh Collider's Simulated Property value.
        """
        return self.component.get_component_property_value(self.Path.SIMULATED)

    def set_in_scene_queries(self, value: bool) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to set the PhysX Mesh Collider's In Scene Queries Property value.
        """
        self.component.set_component_property_value(self.Path.IN_SCENE_QUERIES, value)

    def get_in_scene_queries(self) -> bool:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to get the PhysX Mesh Collider's In Scene Queries Property value.
        """
        return self.component.get_component_property_value(self.Path.IN_SCENE_QUERIES)

    def set_offset(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to set the PhysX Mesh Collider's Offset Property.
        """
        _validate_xyz_is_float(x, y, z, f"One of the offset inputs are not a float - X: {x}, Y: {y}, Z {z}")

        self.component.set_component_property_value(self.Path.OFFSET, math.Vector3(x, y, z))

    def get_offset(self) -> math.Vector3:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to get the PhysX Mesh Collider's Offset Property value.
        """
        return self.component.get_component_property_value(self.Path.OFFSET)

    def set_rotation(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Quaternion', 'Visible')

        Used to set the PhysX Mesh Collider's Rotation Property.
        """
        _validate_xyz_is_float(x, y, z, f"One of the rotation inputs are not a float - X: {x}, Y: {y}, Z {z}")

        rotation = math.Quaternion()
        rotation.SetFromEulerDegrees(math.Vector3(x, y, z))
        self.component.set_component_property_value(self.Path.ROTATION, rotation)

    def get_rotation(self) -> math.Quaternion:
        """
        Property Type, Default Visibility - ('Quaternion', 'Visible')

        Used to get the PhysX Mesh Collider's Rotation Property value.
        """
        return self.component.get_component_property_value(self.Path.ROTATION)

    def set_tag(self, tag: chr) -> None:
        """
        Property Type, Default Visibility - ('AZStd::string', 'Visible')

        Used to set the PhysX Mesh Collider's Tag Property.
        """
        # o3de/o3de#12634 - For some reason I can't get this to work. Says it takes an AZStd::string, but it won't take
        #  one or a character.
        assert NotImplementedError
        self.component.set_component_property_value(self.Path.TAG, tag)

    def get_tag(self) -> chr:
        """
        Property Type, Default Visibility - ('AZStd::string', 'Visible')

        Used to get the PhysX Mesh Collider's Tag Property value.
        """
        return self.component.get_component_property_value(self.Path.TAG)

    def set_rest_offset(self, rest_offset: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Mesh Collider's Reset Offset Property.

        NOTE: Rest Offset Must be LESS-THAN Contact Offset.
        """
        assert isinstance(rest_offset, float), f"The value passed to Rest Offset \"{rest_offset}\" is not a float."

        self.component.set_component_property_value(self.Path.RESET_OFFSET, rest_offset)

    def get_rest_offset(self) -> float:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to get the PhysX Mesh Collider's Reset Offset Property value.
        """
        return self.component.get_component_property_value(self.Path.RESET_OFFSET)

    def set_contact_offset(self, contact_offset: float) -> None:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to set the PhysX Mesh Collider's Contact Offset Property.

        NOTE: Contact Offset Must be GREATER-THAN Rest Offset.
        """
        assert isinstance(contact_offset, float), f"The value passed to Contact Offset \"{contact_offset}\" is not a float."

        self.component.set_component_property_value(self.Path.CONTACT_OFFSET, contact_offset)

    def get_contact_offset(self) -> float:
        """
        Property Type, Default Visibility - ('float', 'Visible')

        Used to get the PhysX Mesh Collider's Contact Offset Property value.
        """
        return self.component.get_component_property_value(self.Path.CONTACT_OFFSET)

    def set_draw_collider(self, value: bool) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to set the PhysX Mesh Collider's Draw Collider Property value.
        """
        self.component.set_component_property_value(self.Path.DRAW_COLLIDER, value)

    def get_draw_collider(self) -> bool:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to get the PhysX Mesh Collider's Draw Collider Property value.
        """
        return self.component.get_component_property_value(self.Path.DRAW_COLLIDER)

    def set_physx_material_from_path(self, asset_product_path: str) -> None:
        """
        Property Type, Default Visibility - ('Asset<Physics::MaterialAsset>', 'Visible')

        Used to set the PhysX Mesh Collider's PhysX Material from a user provided asset product path. This will return an
            error if a source path is provided.
        """
        # o3de/o3de#12503 PhysX Mesh Collider Component's Physic Material field(s) return unintuitive property tree paths.
        assert NotImplementedError
        _validate_property_visibility(self.component, self.Path.PHYSX_MATERIAL_ASSET, PropertyVisibility.VISIBLE)

        px_material = Asset.find_asset_by_path(asset_product_path)
        self.component.set_component_property_value(self.Path.PHYSX_MATERIAL_ASSET, px_material.id)

    def get_physx_material(self) -> Asset:
        """
        Property Type, Default Visibility - ('Asset<Physics::MaterialAsset>', 'Visible')

        Used to get the PhysX Mesh Collider's PhysX Material
        """
        # o3de/o3de#12503 PhysX Mesh Collider Component's Physic Material field(s) return unintuitive property tree paths.
        assert NotImplementedError

        return self.component.get_component_property_value(self.Path.PHYSX_MATERIAL_ASSET)

    def set_physx_mesh(self, asset_product_path: str) -> None:
        """
        Property Type, Default Visibility - ('Asset<MeshAsset>', 'Visible')

        Used to set the PhysX Mesh Collider's PhysicsAsset Shape's PhysX Mesh from a user provided asset product path.
        """
        _validate_property_visibility(self.component, self.Path.PhysicsAsset.PHYSX_MESH, PropertyVisibility.VISIBLE)

        px_asset = Asset.find_asset_by_path(asset_product_path)
        self.component.set_component_property_value(self.Path.PhysicsAsset.PHYSX_MESH, px_asset.id)

    def get_physx_mesh(self) -> azasset.AssetId:
        """
        Property Type, Default Visibility - ('Asset<MeshAsset>', 'Visible')

        Used to get the currently set PhysX Mesh Collider's PhysicsAsset Shape's PhysX Mesh.

        return: This will return the Asset ID and will need to use Asset(id) to be manipulate the asset.
        """
        _validate_property_visibility(self.component, self.Path.PhysicsAsset.PHYSX_MESH, PropertyVisibility.VISIBLE)
        return self.component.get_component_property_value(self.Path.PhysicsAsset.PHYSX_MESH)

    def set_physx_mesh_asset_scale(self, x: float, y: float, z: float) -> None:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to set the PhysX Mesh Collider's PhysX Mesh Asset Scale.
        """
        _validate_xyz_is_float(x, y, z,
                               f"One of the Physx Mesh Asset Scale inputs are not a float - X: {x}, Y: {y}, Z {z}")
        _validate_property_visibility(self.component, self.Path.PhysicsAsset.ASSET_SCALE, PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.PhysicsAsset.ASSET_SCALE,
                                                    math.Vector3(float(x), float(y), float(z)))

    def get_physx_mesh_asset_scale(self) -> None:
        """
        Property Type, Default Visibility - ('Vector3', 'Visible')

        Used to get the PhysX Mesh Collider's PhysX Mesh Asset Scale.
        """
        _validate_property_visibility(self.component, self.Path.PhysicsAsset.ASSET_SCALE, PropertyVisibility.VISIBLE)
        return self.component.get_component_property_value(self.Path.PhysicsAsset.ASSET_SCALE)

    def set_use_physics_materials_from_asset(self, value: bool) -> None:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to set the PhysX Mesh Collider's Physics Materials From Asset. This allows the user to toggle between
            using the asset provided PhysX Materials, or to override them using the PhysX Mesh Collider's PhysX Materials
            property.
        """
        _validate_property_visibility(self.component, self.Path.PhysicsAsset.PHYSICS_MATERIALS_FROM_ASSET,
                                      PropertyVisibility.VISIBLE)

        self.component.set_component_property_value(self.Path.PhysicsAsset.PHYSICS_MATERIALS_FROM_ASSET, value)

    def get_use_physics_materials_from_asset(self) -> bool:
        """
        Property Type, Default Visibility - ('bool', 'Visible')

        Used to get the PhysX Mesh Collider's Draw Collider Property value.
        """
        _validate_property_visibility(self.component, self.Path.PhysicsAsset.PHYSICS_MATERIALS_FROM_ASSET,
                                      PropertyVisibility.VISIBLE)

        return self.component.get_component_property_value(self.Path.PhysicsAsset.PHYSICS_MATERIALS_FROM_ASSET)
