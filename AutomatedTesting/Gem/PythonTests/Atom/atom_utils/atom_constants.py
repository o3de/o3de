"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Hold constants used across both hydra and non-hydra scripts.
"""

# Light type options for the Light component.
LIGHT_TYPES = {
    'unknown': 0,
    'sphere': 1,
    'spot_disk': 2,
    'capsule': 3,
    'quad': 4,
    'polygon': 5,
    'simple_point': 6,
    'simple_spot': 7,
}


class AtomComponentProperties:
    """
    Holds Atom component related constants
    """

    @staticmethod
    def actor(property: str = 'name') -> str:
        """
        Actor component properties.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Actor',
        }
        return properties[property]

    @staticmethod
    def bloom(property: str = 'name') -> str:
        """
        Bloom component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Bloom',
            'requires': [AtomComponentProperties.postfx_layer()],
        }
        return properties[property]

    @staticmethod
    def camera(property: str = 'name') -> str:
        """
        Camera component properties.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Camera',
        }
        return properties[property]

    @staticmethod
    def decal(property: str = 'name') -> str:
        """
        Decal component properties.
          - 'Material' the material Asset.id of the decal.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Decal',
            'Material': 'Controller|Configuration|Material',
        }
        return properties[property]

    @staticmethod
    def deferred_fog(property: str = 'name') -> str:
        """
        Deferred Fog component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Deferred Fog',
            'requires': [AtomComponentProperties.postfx_layer()],
        }
        return properties[property]

    @staticmethod
    def depth_of_field(property: str = 'name') -> str:
        """
        Depth of Field component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
          - 'Camera Entity' an EditorEntity.id reference to the Camera component required for this effect.
            Must be a different entity than the one which hosts Depth of Field component.\n
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'DepthOfField',
            'requires': [AtomComponentProperties.postfx_layer()],
            'Camera Entity': 'Controller|Configuration|Camera Entity',
        }
        return properties[property]

    @staticmethod
    def diffuse_probe(property: str = 'name') -> str:
        """
        Diffuse Probe Grid component properties. Requires one of 'shapes'.
          - 'shapes' a list of supported shapes as component names.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Diffuse Probe Grid',
            'shapes': ['Axis Aligned Box Shape', 'Box Shape']
        }
        return properties[property]

    @staticmethod
    def directional_light(property: str = 'name') -> str:
        """
        Directional Light component properties.
          - 'Camera' an EditorEntity.id reference to the Camera component that controls cascaded shadow view frustum.
            Must be a different entity than the one which hosts Directional Light component.\n
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Directional Light',
            'Camera': 'Controller|Configuration|Shadow|Camera',
        }
        return properties[property]

    @staticmethod
    def display_mapper(property: str = 'name') -> str:
        """
        Display Mapper component properties.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Display Mapper',
        }
        return properties[property]

    @staticmethod
    def entity_reference(property: str = 'name') -> str:
        """
        Entity Reference component properties.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Entity Reference',
        }
        return properties[property]

    @staticmethod
    def exposure_control(property: str = 'name') -> str:
        """
        Exposure Control component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Exposure Control',
            'requires': [AtomComponentProperties.postfx_layer()],
        }
        return properties[property]

    @staticmethod
    def global_skylight(property: str = 'name') -> str:
        """
        Global Skylight (IBL) component properties.
          - 'Diffuse Image' Asset.id for the cubemap image for determining diffuse lighting.
          - 'Specular Image' Asset.id for the cubemap image for determining specular lighting.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Global Skylight (IBL)',
            'Diffuse Image': 'Controller|Configuration|Diffuse Image',
            'Specular Image': 'Controller|Configuration|Specular Image',
        }
        return properties[property]

    @staticmethod
    def grid(property: str = 'name') -> str:
        """
        Grid component properties.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Grid',
        }
        return properties[property]

    @staticmethod
    def hdr_color_grading(property: str = 'name') -> str:
        """
        HDR Color Grading component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'HDR Color Grading',
            'requires': [AtomComponentProperties.postfx_layer()],
        }
        return properties[property]

    @staticmethod
    def hdri_skybox(property: str = 'name') -> str:
        """
        HDRi Skybox component properties.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'HDRi Skybox',
        }
        return properties[property]

    @staticmethod
    def light(property: str = 'name') -> str:
        """
        Light component properties.
          - 'Light type' from atom_constants.py LIGHT_TYPES
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Light',
            'Light type': 'Controller|Configuration|Light type',
        }
        return properties[property]

    @staticmethod
    def look_modification(property: str = 'name') -> str:
        """
        Look Modification component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Look Modification',
            'requires': [AtomComponentProperties.postfx_layer()],
        }
        return properties[property]

    @staticmethod
    def material(property: str = 'name') -> str:
        """
        Material component properties. Requires one of Actor OR Mesh component.
          - 'requires' a list of component names as strings required by this component.
            Only one of these is required at a time for this component.\n
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Material',
            'requires': [AtomComponentProperties.actor(), AtomComponentProperties.mesh()],
        }
        return properties[property]

    @staticmethod
    def mesh(property: str = 'name') -> str:
        """
        Mesh component properties.
          - 'Mesh Asset' Asset.id of the mesh model.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        :rtype: str
        """
        properties = {
            'name': 'Mesh',
            'Mesh Asset': 'Controller|Configuration|Mesh Asset',
        }
        return properties[property]

    @staticmethod
    def occlusion_culling_plane(property: str = 'name') -> str:
        """
        Occlusion Culling Plane component properties.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Occlusion Culling Plane',
        }
        return properties[property]

    @staticmethod
    def physical_sky(property: str = 'name') -> str:
        """
        Physical Sky component properties.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Physical Sky',
        }
        return properties[property]

    @staticmethod
    def postfx_layer(property: str = 'name') -> str:
        """
        PostFX Layer component properties.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'PostFX Layer',
        }
        return properties[property]

    @staticmethod
    def postfx_gradient(property: str = 'name') -> str:
        """
        PostFX Gradient Weight Modifier component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'PostFX Gradient Weight Modifier',
            'requires': [AtomComponentProperties.postfx_layer()],
        }
        return properties[property]

    @staticmethod
    def postfx_radius(property: str = 'name') -> str:
        """
        PostFX Radius Weight Modifier component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'PostFX Radius Weight Modifier',
            'requires': [AtomComponentProperties.postfx_layer()],
        }
        return properties[property]

    @staticmethod
    def postfx_shape(property: str = 'name') -> str:
        """
        PostFX Shape Weight Modifier component properties. Requires PostFX Layer and one of 'shapes' listed.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
          - 'shapes' a list of supported shapes as component names. 'Tube Shape' is also supported but requires 'Spline'.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'PostFX Shape Weight Modifier',
            'requires': [AtomComponentProperties.postfx_layer()],
            'shapes': ['Axis Aligned Box Shape', 'Box Shape', 'Capsule Shape', 'Compound Shape', 'Cylinder Shape',
                       'Disk Shape', 'Polygon Prism Shape', 'Quad Shape', 'Sphere Shape', 'Vegetation Reference Shape'],
        }
        return properties[property]

    @staticmethod
    def reflection_probe(property: str = 'name') -> str:
        """
        Reflection Probe component properties. Requires one of 'shapes' listed.
          - 'shapes' a list of supported shapes as component names.
          - 'Baked Cubemap Path' Asset.id of the baked cubemap image generated by a call to 'BakeReflectionProbe' ebus.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Reflection Probe',
            'shapes': ['Axis Aligned Box Shape', 'Box Shape'],
            'Baked Cubemap Path': 'Cubemap|Baked Cubemap Path',
        }
        return properties[property]

    @staticmethod
    def ssao(property: str = 'name') -> str:
        """
        SSAO component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'SSAO',
            'requires': [AtomComponentProperties.postfx_layer()],
        }
        return properties[property]
