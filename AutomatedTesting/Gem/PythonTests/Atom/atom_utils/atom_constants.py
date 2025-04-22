"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Holds constants used across both hydra and non-hydra scripts.
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

# Intensity mode options for the Light component.
INTENSITY_MODE = {
    'Candela': 0,
    'Lumen': 1,
    'Ev100': 3,
    'Nit': 5,
}

# Attenuation Radius Mode options for the Light component.
ATTENUATION_RADIUS_MODE = {
    'explicit': 0,
    'automatic': 1,
}

# Shadow Map Size options for the Light component.
SHADOWMAP_SIZE = {
    'Size256': 256,
    'Size512': 512,
    'Size1024': 1024,
    'Size2048': 2048,
}

# Shadow filter method options for the Light component.
SHADOW_FILTER_METHOD = {
    'PCF': 1,
    'ESM': 2,
    'PCF+ESM': 3,
    'None': 0,
}

# CubeMap capture type options for the Cubemap Capture Component
CUBEMAP_CAPTURE_TYPE = {
    'Specular IBL': 0,
    'Diffuse ILB': 1,
}

# Specular IBL property options for the Cubemap Capture Component
SPECULAR_IBL_QUALITY = {
    'Very Low': 0,
    'Low': 1,
    'Medium': 2,
    'High': 3,
    'Very High': 4,
}

# Qualiity Level settings for Diffuse Global Illumination level component
GLOBAL_ILLUMINATION_QUALITY = {
    'Low': 0,
    'Medium': 1,
    'High': 2,
}

# Mesh LOD type
MESH_LOD_TYPE = {
    'default': 0,
    'screen coverage': 1,
    'specific lod': 2,
}

# Display Mapper type
DISPLAY_MAPPER_OPERATION_TYPE = {
    'Aces': 0,
    'AcesLut': 1,
    'Passthrough': 2,
    'GammaSRGB': 3,
    'Reinhard': 4,
}

# Display Mapper presets
DISPLAY_MAPPER_PRESET = {
    '48Nits': 0,
    '1000Nits': 1,
    '2000Nits': 2,
    '4000Nits': 3,
}

# Control Type options for the Exposure Control component.
EXPOSURE_CONTROL_TYPE = {
    'manual': 0,
    'eye_adaptation': 1
}

#Reflection Probe Baked Cubemap Quality
BAKED_CUBEMAP_QUALITY = {
    'Very Low': 0,
    'Low': 1,
    'Medium': 2,
    'High': 3,
    'Very High': 4
}

#Diffuse Probe Grid number of rays to cast per probe from enum DiffuseProbeGridNumRaysPerProbe
NUM_RAYS_PER_PROBE = {
    'NumRaysPerProbe_144': 0,
    'NumRaysPerProbe_288': 1,
    'NumRaysPerProbe_432': 2,
    'NumRaysPerProbe_576': 3,
    'NumRaysPerProbe_720': 4,
    'NumRaysPerProbe_864': 5,
    'NumRaysPerProbe_1008': 6,
}

# LUT Resolution options for the HDR Color Grading component.
LUT_RESOLUTION = {
    '16x16x16': 16,
    '32x32x32': 32,
    '64x64x64': 64,
}

# Shaper Type options for the HDR Color Grading & Look Modification components.
SHAPER_TYPE = {
    'None': 0,
    'linear_custom': 1,
    '48_nits': 2,
    '1000_nits': 3,
    '2000_nits': 4,
    '4000_nits': 5,
    'log2_custom': 6,
    'pq': 7,
}

# Hair Lighting Model
HAIR_LIGHTING_MODEL = {
    'GGX': 0,
    'Marschner': 1,
    'Kajiya': 2,
}

# Physical Sky Intensity Mode
PHYSICAL_SKY_INTENSITY_MODE = {
    'Ev100': 4,
    'Nit': 3,
}

# PostFX Layer Category as defined in
# ./Gems/AtomLyIntegration/CommonFeatures/Assets/PostProcess/default.postfxlayercategories
POSTFX_LAYER_CATEGORY = {
    'FrontEnd': 1000000,
    'Cinematics': 2000000,
    'Gameplay': 3000000,
    'Camera': 4000000,
    'Volume': 5000000,
    'Level': 6000000,
    'Default': 2147483647,
}

# Directional Light Intensity Mode
DIRECTIONAL_LIGHT_INTENSITY_MODE = {
    'Ev100': 5,
    'Lux': 2,
}

# Directional Light Shadow Filter Method
DIRECTIONAL_LIGHT_SHADOW_FILTER_METHOD = {
    'None': 0,
    'PCF': 1,
    'ESM': 2,
    'PCF+ESM': 3,
}

#Origin type for Sky Atmosphere component
ATMOSPHERE_ORIGIN = {
    'GroundAtWorldOrigin': 0,
    'GroundAtLocalOrigin': 1,
    'PlanetCenterAtLocalOrigin': 2,
}

# Fog Mode
FOG_MODES = {
    'Linear': 0,
    'Exponential': 1,
    'ExponentialSquared': 2
}

# Level list used in Editor Level Load Test
# WARNING: "Sponza" level is sandboxed due to an intermittent failure.
LEVEL_LIST = ["hermanubis", "hermanubis_high", "macbeth_shaderballs", "PbrMaterialChart", "ShadowTest"]


class AtomComponentProperties:
    """
    Holds Atom component related constants
    """

    @staticmethod
    def actor(property: str = 'name') -> str:
        """
        Actor component properties.
          - 'Actor asset' Asset.id of the actor asset.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Actor',
            'Actor asset': 'Actor asset',
        }
        return properties[property]

    @staticmethod
    def bloom(property: str = 'name') -> str:
        """
        Bloom component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
          - 'Enable Bloom' Toggle active state of the component True/False
          - 'Enabled Override' Toggle use of overrides on the bloom component
          - 'Threshold Override' Override factor for Threshold (0-1)
          - 'Knee Override' Override factor for Knee (0-1)
          - 'Intensity Override' Override factor for Intensity (0-1)
          - 'BicubicEnabled Override' Override toggle for Bicubic Enabled
          - 'KernelSizeScale Override' Override factor for Kernel Size Scale (0-1)
          - 'KernelSizeStage0 Override' Override factor for Kernel Size stage 0 (0-1)
          - 'KernelSizeStage1 Override' Override factor for Kernel Size stage 1 (0-1)
          - 'KernelSizeStage2 Override' Override factor for Kernel Size stage 2 (0-1)
          - 'KernelSizeStage3 Override' Override factor for Kernel Size stage 3 (0-1)
          - 'KernelSizeStage4 Override' Override factor for Kernel Size stage 4 (0-1)
          - 'TintStage0 Override' Override factor for Tint stage 0 (0-1)
          - 'TintStage1 Override' Override factor for Tint stage 1 (0-1)
          - 'TintStage2 Override' Override factor for Tint stage 2 (0-1)
          - 'TintStage3 Override' Override factor for Tint stage 3 (0-1)
          - 'TintStage4 Override' Override factor for Tint stage 4 (0-1)
          - 'Threshold' Brighness of the light source to which bloom is applied (0-INF)
          - 'Knee' Soft knee to smoothen edges of threshold (0-1)
          - 'Intensity' Brightness of bloom (0-10000)
          - 'Enable Bicubic' Toggle to enable Bicubic Filtering for upsampling
          - 'Kernel Size Scale' Global scaling factor for Kernel size (0-2)
          - 'Kernel Size 0' Kernel Size for blur stage 0 in percent of screen width (0-1)
          - 'Kernel Size 1' Kernel Size for blur stage 1 in percent of screen width (0-1)
          - 'Kernel Size 2' Kernel Size for blur stage 2 in percent of screen width (0-1)
          - 'Kernel Size 3' Kernel Size for blur stage 3 in percent of screen width (0-1)
          - 'Kernel Size 4' Kernel Size for blur stage 4 in percent of screen width (0-1)
          - 'Tint 0' Tint for blur stage 0. RGB value set using azlmbr.math.Vector3 tuple
          - 'Tint 1' Tint for blur stage 1. RGB value set using azlmbr.math.Vector3 tuple
          - 'Tint 2' Tint for blur stage 2. RGB value set using azlmbr.math.Vector3 tuple
          - 'Tint 3' Tint for blur stage 3. RGB value set using azlmbr.math.Vector3 tuple
          - 'Tint 4' Tint for blur stage 4. RGB value set using azlmbr.math.Vector3 tuple
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Bloom',
            'requires': [AtomComponentProperties.postfx_layer()],
            'Enable Bloom': 'Controller|Configuration|Enable Bloom',
            'Enabled Override': 'Controller|Configuration|Overrides|Enabled Override',
            'Threshold Override': 'Controller|Configuration|Overrides|Threshold Override',
            'Knee Override': 'Controller|Configuration|Overrides|Knee Override',
            'Intensity Override': 'Controller|Configuration|Overrides|Intensity Override',
            'BicubicEnabled Override': 'Controller|Configuration|Overrides|BicubicEnabled Override',
            'KernelSizeScale Override': 'Controller|Configuration|Overrides|KernelSizeScale Override',
            'KernelSizeStage0 Override': 'Controller|Configuration|Overrides|KernelSizeStage0 Override',
            'KernelSizeStage1 Override': 'Controller|Configuration|Overrides|KernelSizeStage1 Override',
            'KernelSizeStage2 Override': 'Controller|Configuration|Overrides|KernelSizeStage2 Override',
            'KernelSizeStage3 Override': 'Controller|Configuration|Overrides|KernelSizeStage3 Override',
            'KernelSizeStage4 Override': 'Controller|Configuration|Overrides|KernelSizeStage4 Override',
            'TintStage0 Override': 'Controller|Configuration|Overrides|TintStage0 Override',
            'TintStage1 Override': 'Controller|Configuration|Overrides|TintStage1 Override',
            'TintStage2 Override': 'Controller|Configuration|Overrides|TintStage2 Override',
            'TintStage3 Override': 'Controller|Configuration|Overrides|TintStage3 Override',
            'TintStage4 Override': 'Controller|Configuration|Overrides|TintStage4 Override',
            'Threshold': 'Controller|Configuration|Threshold',
            'Knee': 'Controller|Configuration|Knee',
            'Intensity': 'Controller|Configuration|Intensity',
            'Enable Bicubic': 'Controller|Configuration|Enable Bicubic',
            'Kernel Size Scale': 'Controller|Configuration|Kernel Size|Kernel Size Scale',
            'Kernel Size 0': 'Controller|Configuration|Kernel Size|Kernel Size 0',
            'Kernel Size 1': 'Controller|Configuration|Kernel Size|Kernel Size 1',
            'Kernel Size 2': 'Controller|Configuration|Kernel Size|Kernel Size 2',
            'Kernel Size 3': 'Controller|Configuration|Kernel Size|Kernel Size 3',
            'Kernel Size 4': 'Controller|Configuration|Kernel Size|Kernel Size 4',
            'Tint 0': 'Controller|Configuration|Tint|Tint 0',
            'Tint 1': 'Controller|Configuration|Tint|Tint 1',
            'Tint 2': 'Controller|Configuration|Tint|Tint 2',
            'Tint 3': 'Controller|Configuration|Tint|Tint 3',
            'Tint 4': 'Controller|Configuration|Tint|Tint 4',
        }
        return properties[property]

    @staticmethod
    def camera(property: str = 'name') -> str:
        """
        Camera component properties.
          - 'Field of view': Sets the value for the camera's FOV (Field of View) in degrees, i.e. 60.0
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Camera',
            'Field of view': 'Controller|Configuration|Field of view'
        }
        return properties[property]

    @staticmethod
    def chromatic_aberration(property: str = 'name') -> str:
        """
        Chromatic Aberration component properties
          - 'Enable Chromatic Aberration' Toggle active state of the component (bool) default False
          - 'Strength' Strength of effect. 0.0 (default) to 1.0 (float)
          - 'Blend' Factor for additive blending with original image. 0.0 (default) to 1.0 (float)
          - 'Enabled Override'  Toggle use of overrides on the Chromatic Aberration component (bool) default False
          - 'Strength Override' Override for Strength factor. 0.0 (default) to 1.0 (float)
          - 'Blend Override' Override for Blend factor. 0.0 (default) to 1.0 (float)
        parameter property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Chromatic Aberration',
            'Enable Chromatic Aberration': 'Controller|Configuration|Enable Chromatic Aberration',
            'Strength': 'Controller|Configuration|Strength',
            'Blend': 'Controller|Configuration|Blend',
            'Enabled Override': 'Controller|Configuration|Overrides|Enabled Override',
            'Strength Override': 'Controller|Configuration|Overrides|Strength Override',
            'Blend Override': 'Controller|Configuration|Overrides|Blend Override',
        }
        return properties[property]

    @staticmethod
    def cube_map_capture(property: str = 'name') -> str:
        """
        CubeMap capture component properties.
          - 'Specular ILB' controls the quality of Specular IBL created
          - 'Capture Type': controls if CubeMap Capture component uses 'Diffuse ILB' or 'Specular ILB'
          - 'Exposure': Controls the exposure light in the image taken
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'CubeMap Capture',
            'Specular IBL CubeMap Quality': 'Controller|Configuration|Specular IBL CubeMap Quality',
            'Capture Type': 'Controller|Configuration|Capture Type',
            'Exposure': 'Controller|Configuration|Exposure',
        }
        return properties[property]

    @staticmethod
    def decal(property: str = 'name') -> str:
        """
        Decal component properties.
          - 'Attenuation Angle' controls how much the angle between geometry and decal impacts opacity. 0-1 Radians
          - 'Opacity' where one is opaque and zero is transparent
          - 'Normal Map Opacity' normal map set to one is opaque and zero is transparent
          - 'Sort Key' 0-255 stacking z-sort like key to define which decal is on top of another
          - 'Material' the material Asset.id of the decal.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Decal',
            'Attenuation Angle': 'Controller|Configuration|Attenuation Angle',
            'Opacity': 'Controller|Configuration|Opacity',
            'Normal Map Opacity': 'Controller|Configuration|Normal Map Opacity',
            'Sort Key': 'Controller|Configuration|Sort Key',
            'Material': 'Controller|Configuration|Material',
        }
        return properties[property]

    @staticmethod
    def deferred_fog(property: str = 'name') -> str:
        """
        Deferred Fog component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
          - 'Enable Deferred Fog' Toggle active state of the component (bool).
          - 'Fog Color' Sets the fog color. RGB value set using azlmbr.math.Vector3 tuple.
          - 'Fog Start Distance' Distance from the viewer where the fog starts (0.0, 5000.0).
          - 'Fog End Distance' Distance from the viewer where fog masks the background scene (0.0, 5000.0).
          - 'Fog Bottom Height' Height at which the fog layer starts (-5000.0, 5000.0).
          - 'Fog Max Height' Height of the fog layer top (-5000.0, 5000.0).
          - 'Noise Texture' The noise texture used for creating the fog turbulence (Asset.id).
            This property is not yet implemented for editing and will be fixed in a future sprint.
          - 'Noise Texture First Octave Scale' Scale of the first noise octave (INF, INF).
            Higher values indicates higher frequency. Values set using azlmbr.math.Vector2 tuple.
          - 'Noise Texture First Octave Velocity' Velocity of the first noise octave UV coordinates (INF, INF).
            Values set using azlmbr.math.Vector2 tuple.
          - 'Noise Texture Second Octave Scale' Scale of the second noise octave (INF, INF).
            Higher values indicates higher frequency. Values set using azlmbr.math.Vector2 tuple.
          - 'Noise Texture Second Octave Velocity' Velocity of the second noise octave UV coordinates (INF, INF).
            Values set using azlmbr.math.Vector2 tuple.
          - 'Octaves Blend Factor' Blend factor between the noise octaves (0.0, 1.0).
          - 'Enable Turbulence Properties' Enables Turbulence Properties (bool).
          - 'Enable Fog Layer' Enables the fog layer (bool).
          - 'Fog Mode' Set the fog formula
          (enum, Uses above dictionary FOG_MODES)
               Linear: f = (end - d)/(end - start)
               Exponential: f = 1/exp(d * density)
               ExponentialSquared: f = 1/exp((d * density)^2)
          - 'Fog Density': Density control for Exponential and ExponentialSquared modes.
          - 'Fog Density Clamp': The maximum density that the fog can reach. 
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Deferred Fog',
            'requires': [AtomComponentProperties.postfx_layer()],
            'Enable Deferred Fog': 'Controller|Configuration|Enable Deferred Fog',
            'Fog Color': 'Controller|Configuration|Fog Color',
            'Fog Start Distance': 'Controller|Configuration|Distance|Fog Start Distance',
            'Fog End Distance': 'Controller|Configuration|Distance|Fog End Distance',
            'Fog Bottom Height': 'Controller|Configuration|Fog Layer|Fog Bottom Height',
            'Fog Max Height': 'Controller|Configuration|Fog Layer|Fog Max Height',
            'Noise Texture': 'Controller|Configuration|Turbulence|Noise Texture',
            'Noise Texture First Octave Scale': 'Controller|Configuration|Turbulence|Noise Texture First Octave Scale',
            'Noise Texture First Octave Velocity': 'Controller|Configuration|Turbulence|Noise Texture First Octave Velocity',
            'Noise Texture Second Octave Scale': 'Controller|Configuration|Turbulence|Noise Texture Second Octave Scale',
            'Noise Texture Second Octave Velocity': 'Controller|Configuration|Turbulence|Noise Texture Second Octave Velocity',
            'Octaves Blend Factor': 'Controller|Configuration|Turbulence|Octaves Blend Factor',
            'Enable Turbulence Properties': 'Controller|Configuration|Enable Turbulence Properties',
            'Enable Fog Layer': 'Controller|Configuration|Enable Fog Layer',
            'Fog Density': 'Controller|Configuration|Density Control|Fog Density',
            'Fog Density Clamp': 'Controller|Configuration|Density Control|Fog Density Clamp',
            'Fog Mode': 'Controller|Configuration|Fog Mode',
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
          - 'CameraEntityId Override' Override enable for CameraEntityId, (bool, default true).
          - 'Enabled Override' Override enable for Enabled, (bool, default true).
          - 'QualityLevel Override' Override enable for QualityLevel, (bool, default true).
          - 'ApertureF Override' Override factor for ApertureF, (float, 0.0 to default 1.0).
          - 'FocusDistance Override' Override factor for Focus Distance, (float, 0.0 to default 1.0).
          - 'EnableAutoFocus Override' Override enable for EnableAutoFocus, (bool, default true).
          - 'AutoFocusScreenPosition Override' Override enable for AutoFocusScreenPosition, (float, 0.0 to default 1.0).
          - 'AutoFocusSensitivity Override' Override enable for AutoFocusSensitivity, (float, 0.0 to default 1.0).
          - 'AutoFocusSpeed Override' Override enable for AutoFocusSpeed, (float, 0.0 to default 1.0).
          - 'AutoFocusDelay Override' Override enable for AutoFocusDelay, (float, 0.0 to default 1.0).
          - 'EnableDebugColoring Override' Override enable for EnableDebugColoring, (bool, default true).
          - 'Enable Depth of Field' Enables or disables depth of field, (bool, default false).
          - 'Quality Level' 0 or 1, 0 is standard Bokeh blur, 1 is high quality Bokeh blur (int, 0 or 1, default is 1).
          - 'Aperture F' The higher the value the larger the aperture opening, (float, 0.0 to default 0.5).
          - 'F Number' The ratio of the system's focal length to the diameter of the aperture.
          - 'Focus Distance' The distance from the camera to the focused object (float, 0.0 to default 100.0).
          - 'Enable Auto Focus' Enables or disables auto focus (bool, default true).
          - 'Focus Screen Position' XY value of the focus position on screen for autofocus (math.Vector2(float x, float
           y) where ranges are 0.0 to 1.0).\n
          - 'Auto Focus Sensitivity' Higher value is more responsive, lower needs greater distance depth to refocus,
             range 0.0 to 1.0.
          - 'Auto Focus Speed' Distance that focus moves per second, normalizing the distance from view near to view far
            at the value of 1, range 0.0 to 2.0.
          - 'Auto Focus Delay' Specifies a delay time for focus to shift from one target to another, range 0.0 to 1.0.
          - 'Enable Debug Color' Enables or disables debug color overlay, (bool).
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Depth Of Field',
            'requires': [AtomComponentProperties.postfx_layer()],
            'Camera Entity': 'Controller|Configuration|Camera Entity',
            'CameraEntityId Override': 'Controller|Configuration|Overrides|CameraEntityId Override',
            'Enabled Override': 'Controller|Configuration|Overrides|Enabled Override',
            'QualityLevel Override': 'Controller|Configuration|Overrides|QualityLevel Override',
            'ApertureF Override': 'Controller|Configuration|Aperture F',
            'FocusDistance Override': 'Controller|Configuration|Overrides|FocusDistance Override',
            'EnableAutoFocus Override': 'Controller|Configuration|Overrides|EnableAutoFocus Override',
            'AutoFocusScreenPosition Override': 'Controller|Configuration|Overrides|AutoFocusScreenPosition Override',
            'AutoFocusSensitivity Override': 'Controller|Configuration|Overrides|AutoFocusSensitivity Override',
            'AutoFocusSpeed Override': 'Controller|Configuration|Overrides|AutoFocusSpeed Override',
            'AutoFocusDelay Override': 'Controller|Configuration|Overrides|AutoFocusDelay Override',
            'EnableDebugColoring Override': 'Controller|Configuration|Overrides|EnableDebugColoring Override',
            'Enable Depth of Field': 'Controller|Configuration|Enable Depth of Field',
            'Quality Level': 'Controller|Configuration|Quality Level',
            'Aperture F': 'Controller|Configuration|Aperture F',
            'F Number': 'Controller|Configuration|F Number',
            'Focus Distance': 'Controller|Configuration|Focus Distance',
            'Enable Auto Focus': 'Controller|Configuration|Auto Focus|Enable Auto Focus',
            'Focus Screen Position': 'Controller|Configuration|Auto Focus|Focus Screen Position',
            'Auto Focus Sensitivity': 'Controller|Configuration|Auto Focus|Auto Focus Sensitivity',
            'Auto Focus Speed': 'Controller|Configuration|Auto Focus|Auto Focus Speed',
            'Auto Focus Delay': 'Controller|Configuration|Auto Focus|Auto Focus Delay',
            'Enable Debug Color': 'Controller|Configuration|Debugging|Enable Debug Color'
        }
        return properties[property]

    @staticmethod
    def diffuse_global_illumination(property: str = 'name') -> str:
        """
        Diffuse Global Illumination level component properties.
        Controls global settings for Diffuse Probe Grid components.
          - 'Quality Level' from atom_constants.py GLOBAL_ILLUMINATION_QUALITY
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Diffuse Global Illumination',
            'Quality Level': 'Controller|Configuration|Quality Level'
        }
        return properties[property]

    @staticmethod
    def diffuse_probe_grid(property: str = 'name') -> str:
        """
        Diffuse Probe Grid component properties. Requires one of 'shapes'.
          - 'shapes' a list of supported shapes as component names.
          - 'Scrolling' Toggle the translation of probes with the entity (bool)
          - 'Show Inactive Probes' Toggle the visualization of inactive probes (bool)
          - 'Show Visualization' Toggles the probe grid visualization (bool)
          - 'Visualization Sphere Radius' Sets the radius of probe visualization spheres (float 0.1 to inf)
          - 'Normal Bias' Adjusts normal bias (float 0.0 to 1.0)
          - 'Ambient Multiplier' adjusts multiplier for irradiance intensity (float 0.0 to 10.0)
          - 'View Bias'Adjusts view bias (float 0.0 to 1.0)
          - 'Number of Rays Per Probe' Number of rays to cast per probe from atom_constants.py NUM_RAYS_PER_PROBE
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Diffuse Probe Grid',
            'shapes': ['Axis Aligned Box Shape', 'Box Shape'],
            'Scrolling': 'Grid Settings|Scrolling',
            'Show Inactive Probes': 'Visualization|Show Inactive Probes',
            'Show Visualization': 'Visualization|Show Visualization',
            'Visualization Sphere Radius': 'Visualization|Visualization Sphere Radius',
            'Normal Bias': 'Grid Settings|Normal Bias',
            'Ambient Multiplier': 'Grid Settings|Ambient Multiplier',
            'View Bias': 'Grid Settings|View Bias',
            'Number of Rays Per Probe': 'Grid Settings|Number of Rays Per Probe',
        }
        return properties[property]

    @staticmethod
    def directional_light(property: str = 'name') -> str:
        """
        Directional Light component properties.
          - 'Camera' an EditorEntity.id reference to the Camera component that controls cascaded shadow view frustum.
               Must be a different entity than the one which hosts Directional Light component.
          - 'Color' Color of the Light. (Color imported from azlmbr.math (0.0, 0.0, 0.0, 0.0))
          - 'Intensity mode' Allows specifying light values in Lux or Ev100. (enum, Lux, Ev100)
               (Uses above dictionary DIRECTIONAL_LIGHT_INTENSITY_MODE)
          - 'Intensity' Intensity of the light in the set photometric unit. (float, default 4.0, range -4.0 to 16.0)
          - 'Angular diameter' Angular diameter of the directional light in degrees. The sun is about 0.5.
               (float, Default 0.5, range 0.0 - 1.0)
          - 'Camera' Entity of the camera for cascaded shadowmap view frustum.
          - 'Shadow far clip' Shadow specific far clip distance. (default 100.0, range -INF to INF)
          - 'Shadowmap size' Width/Height of shadowmap. (4 enum values selectable from dropdown: 256, 512, 1024, 2048))
          - 'Cascade count' Number of cascades. (integer default 4, range 0 to 4)
          - 'Automatic splitting' Switch splitting of shadowmap frustum to cascades automatically or not. (bool)
          - 'Split ratio' Ratio to lerp between the two types of frustrum splitting scheme.
               (float, default 0.9, range 0.0 - 1.0)
               0 = Uniform scheme which will split the frustum evenly across all cascades.
               1 = Logarithmic scheme which is designed to split the frustrum in a logarithmic fashion in order to
                   enable us
              to produce a more optimal perspective aliasing across the frustrum.
          - 'Far depth cascade' Far depth of each cascade. The value of the index greater than or equal to cascade
               count is ignored.
               (Vector4 imported from azlmbr.math (0.0, 0.0, 0.0, 0.0) each value should be greater than the previous,
               range 0 to current value of Shadow far clip.)
          - 'Ground height' Height of the ground.  Used to correct position of cascades.
               (float, default 0.0, range -INF - INF)
          - 'Cascade correction' Enable position correction of cascades to optimize the appearance for certain
               camera positions. (bool)
          - 'Debug coloring' Enable coloring to see how cascades places 0:red, 1:green, 2:blue, 3:yellow.
          - 'Shadow filter method' Filtering method of edge-softening of shadows.
               (enum, Uses above dictionary DIRECTIONAL_LIGHT_SHADOW_FILTER_METHOD)
               None: no Filtering
               PCF: Percentage-closer filtering
               ESM: Exponential shadow maps
               ESM+PCF: ESM with a PCF fallback
               For BehaviorContext (or TrackView), None=0, PCF=1, ESM=2, ESM=PCF=3
          - 'Filtering sample count' This is used only when the pixel is predicted to be on the boundary.
               (integer, default 32, range 4 to 64)
          - 'Shadow Receiver Plan Bias Enable' This reduces shadow acne when using large pcf kernels. (bool)
          - 'Shadow Bias' Reduces acne by applying a fixed bias along z in shadow-space.
               If this is 0, no biasing is applied. (float, default 0.002, range 0.002 to 0.2)
          - 'Normal Shadow Bias' Reducing acne by biasing the shadowmap lookup along the geometric normal.
               If this is 0, no biasing is applied. (float, default 2.5, range 0.0 to 10.0)
          - 'Blend between cascades' Enable smooth blending between shadowmap cascades. (bool)
          - 'Fullscreen Blur' Enables fullscreen blur on fullscreen sunlight shadows. (bool)
          - 'Fullscreen Blur Strength' Affects how strong the fullscreen is. (float, default is 0.667 range 0 to 0.95)
          - 'Fullscreen Blur Sharpness' Affect how sharp the fullscreen shadow blur appears around edges.
               (float, default 50.0 range 0.0 to 400.0)
          - 'Affects GI' Controls whether this light affects diffuse global illumination. (bool)
          - 'Factor' Multiplier on the amount of contribution to diffuse global illumination.
                (float, default 1.0 range 0.0 to 2.0)
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Directional Light',
            'Color': 'Controller|Configuration|Color',
            'Intensity mode': 'Controller|Configuration|Intensity mode',
            'Intensity': 'Controller|Configuration|Intensity',
            'Angular diameter': 'Controller|Configuration|Angular diameter',
            'Camera': 'Controller|Configuration|Shadow|Camera',
            'Shadow far clip': 'Controller|Configuration|Shadow|Shadow far clip',
            'Shadowmap size': 'Controller|Configuration|Shadow|Shadowmap size',
            'Cascade count': 'Controller|Configuration|Shadow|Cascade count',
            'Automatic splitting': 'Controller|Configuration|Shadow|Automatic splitting',
            'Split ratio': 'Controller|Configuration|Shadow|Split ratio',
            'Far depth cascade': 'Controller|Configuration|Shadow|Far depth cascade',
            'Ground height': 'Controller|Configuration|Shadow|Ground height',
            'Cascade correction': 'Controller|Configuration|Shadow|Cascade correction',
            'Debug coloring': 'Controller|Configuration|Shadow|Debug coloring',
            'Shadow filter method': 'Controller|Configuration|Shadow|Shadow filter method',
            'Filtering sample count': 'Controller|Configuration|Shadow|Filtering sample count',
            'Shadow Receiver Plane Bias Enable': 'Controller|Configuration|Shadow|Shadow Receiver Plane Bias Enable',
            'Shadow Bias': 'Controller|Configuration|Shadow|Shadow Bias',
            'Normal Shadow Bias': 'Controller|Configuration|Shadow|Normal Shadow Bias',
            'Blend between cascades': 'Controller|Configuration|Shadow|Blend between cascades',
            'Fullscreen Blur': 'Controller|Configuration|Shadow|Fullscreen Blur',
            'Fullscreen Blur Strength': 'Controller|Configuration|Shadow|Fullscreen Blur Strength',
            'Fullscreen Blur Sharpness': 'Controller|Configuration|Shadow|Fullscreen Blur Sharpness',
            'Affects GI': 'Controller|Configuration|Global Illumination|Affects GI',
            'Factor': 'Controller|Configuration|Global Illumination|Factor',
        }
        return properties[property]

    @staticmethod
    def display_mapper(property: str = 'name') -> str:
        """
        Display Mapper level component properties.
          - 'Type' specifies the Display Mapper type from atom_constants.py DISPLAY_MAPPER_OPERATION_TYPE
          - 'Enable LDR color grading LUT' toggles the use of LDR color grading LUT
          - 'LDR color Grading LUT' is the Low Definition Range (LDR) color grading for Look-up Textures (LUT) which is
            an Asset.id value corresponding to a LUT asset file.
          - 'Override Defaults' toggle enables parameter overrides for ACES settings (bool)
          - 'Alter Surround' toggle applies gamma adjustments for dim surround (bool)
          - 'Alter Desaturation' toggle applies desaturation adjustment for luminance differences (bool)
          - 'Alter CAT D60 to D65' toggles conversion referencing black luminance level constant (bool)
          - 'Preset Selection' select from a list of presets from atom_constants.py DISPLAY_MAPPER_PRESET
          - 'Cinema Limit (black)' reference black
          - 'Cinema Limit (white)' reference white
          - 'Min Point (luminance)' linear extension below this value
          - 'Mid Point (luminance)' middle gray value
          - 'Max Point (luminance)' linear extension above this value
          - 'Surround Gamma' applied to compensate for the condition of the viewing environment
          - 'Gamma' value applied as the basic Gamma curve Opto-Electrical Transfer Function (OETF)
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Display Mapper',
            'Type': 'Controller|Configuration|Type',
            'Enable LDR color grading LUT': 'Controller|Configuration|Enable LDR color grading LUT',
            'LDR color Grading LUT': 'Controller|Configuration|LDR color Grading LUT',
            'Override Defaults': 'Controller|Configuration|ACES Parameters|Override Defaults',
            'Alter Surround': 'Controller|Configuration|ACES Parameters|Alter Surround',
            'Alter Desaturation': 'Controller|Configuration|ACES Parameters|Alter Desaturation',
            'Alter CAT D60 to D65': 'Controller|Configuration|ACES Parameters|Alter CAT D60 to D65',
            'Preset Selection': 'Controller|Configuration|ACES Parameters|Load Preset|Preset Selection',
            'Cinema Limit (black)': 'Controller|Configuration|ACES Parameters|Cinema Limit (black)',
            'Cinema Limit (white)': 'Controller|Configuration|ACES Parameters|Cinema Limit (white)',
            'Min Point (luminance)': 'Controller|Configuration|ACES Parameters|Min Point (luminance)',
            'Mid Point (luminance)': 'Controller|Configuration|ACES Parameters|Mid Point (luminance)',
            'Max Point (luminance)': 'Controller|Configuration|ACES Parameters|Max Point (luminance)',
            'Surround Gamma': 'Controller|Configuration|ACES Parameters|Surround Gamma',
            'Gamma': 'Controller|Configuration|ACES Parameters|Gamma',
        }
        return properties[property]

    @staticmethod
    def entity_reference(property: str = 'name') -> str:
        """
        Entity Reference component properties.
          - 'EntityIdReferences' component container of entityId references. Initially empty.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Entity Reference',
            'EntityIdReferences': 'Controller|Configuration|EntityIdReferences',
        }
        return properties[property]

    @staticmethod
    def exposure_control(property: str = 'name') -> str:
        """
        Exposure Control component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
          - 'Enable' Toggle active state of Exposure Control (bool).
          - 'Enabled Override' Toggle active state of Exposure Control Overrides (bool).
          - 'ExposureControlType Override' Toggle enable for Exposure Control Type (bool).
          - 'ManualCompensation Override' Override Factor for Manual Compensation (0.0, 1.0).
          - 'EyeAdaptationExposureMin Override' Override Factor for Minimum Exposure (0.0, 1.0).
          - 'EyeAdaptationExposureMax Override' Override Factor for Maximum Exposure (0.0, 1.0).
          - 'EyeAdaptationSpeedUp Override' Override Factor for Speed Up (0.0, 1.0).
          - 'EyeAdaptationSpeedDown Override' Override Factor for Speed Down (0.0, 1.0).
          - 'HeatmapEnabled Override' Toggle enable for Enable Heatmap (bool).
          - 'Control Type' specifies manual or Eye Adaptation control from atom_constants.py CONTROL_TYPE.
          - 'Manual Compensation' Manual exposure compensation value (-16.0, 16.0).
          - 'Minimum Exposure' Exposure compensation for Eye Adaptation minimum exposure (-16.0, 16.0).
          - 'Maximum Exposure' Exposure compensation for Eye Adaptation maximum exposure (-16.0, 16.0).
          - 'Speed Up' Speed for Auto Exposure to adapt to bright scenes (0.01, 10.0).
          - 'Speed Down' Speed for Auto Exposure to adapt to dark scenes (0.01, 10.0).
          - 'Enable Heatmap' Toggle enable for Heatmap (bool).
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Exposure Control',
            'requires': [AtomComponentProperties.postfx_layer()],
            'Enable': 'Controller|Configuration|Enable',
            'Enabled Override': 'Controller|Configuration|Overrides|Enabled Override',
            'ExposureControlType Override': 'Controller|Configuration|Overrides|ExposureControlType Override',
            'ManualCompensation Override': 'Controller|Configuration|Overrides|ManualCompensation Override',
            'EyeAdaptationExposureMin Override': 'Controller|Configuration|Overrides|EyeAdaptationExposureMin Override',
            'EyeAdaptationExposureMax Override': 'Controller|Configuration|Overrides|EyeAdaptationExposureMax Override',
            'EyeAdaptationSpeedUp Override': 'Controller|Configuration|Overrides|EyeAdaptationSpeedUp Override',
            'EyeAdaptationSpeedDown Override': 'Controller|Configuration|Overrides|EyeAdaptationSpeedDown Override',
            'HeatmapEnabled Override': 'Controller|Configuration|Overrides|HeatmapEnabled Override',
            'Control Type': 'Controller|Configuration|Control Type',
            'Manual Compensation': 'Controller|Configuration|Manual Compensation',
            'Minimum Exposure': 'Controller|Configuration|Eye Adaptation|Minimum Exposure',
            'Maximum Exposure': 'Controller|Configuration|Eye Adaptation|Maximum Exposure',
            'Speed Up': 'Controller|Configuration|Eye Adaptation|Speed Up',
            'Speed Down': 'Controller|Configuration|Eye Adaptation|Speed Down',
            'Enable Heatmap': 'Controller|Configuration|Eye Adaptation|Enable Heatmap',
        }
        return properties[property]

    @staticmethod
    def global_skylight(property: str = 'name') -> str:
        """
        Global Skylight (IBL) component properties.
          - 'Diffuse Image' Asset.id for the cubemap image for determining diffuse lighting.
          - 'Specular Image' Asset.id for the cubemap image for determining specular lighting.
          - 'Exposure' Exposure setting for Global Skylight, value range is -5 to 5
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Global Skylight (IBL)',
            'Diffuse Image': 'Diffuse Image',
            'Specular Image': 'Specular Image',
            'Exposure': 'Exposure',
        }
        return properties[property]

    @staticmethod
    def grid(property: str = 'name') -> str:
        """
        Grid component properties.
          - 'Grid Size': The size of the grid, default value is 32
          - 'Axis Color': Sets color of the grid axis using azlmbr.math.Color tuple, default value is 0,0,255 (blue)
          - 'Primary Grid Spacing': Amount of space between grid lines, default value is 1.0
          - 'Primary Color': Sets color of the primary grid lines using azlmbr.math.Color tuple,
             default value is 64,64,64 (dark grey)
          - 'Secondary Grid Spacing': Amount of space between sub-grid lines, default value is 0.25
          - 'Secondary Color': Sets color of the secondary grid lines using azlmbr.math.Color tuple,
             default value is 128,128,128 (light grey)
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Grid',
            'Grid Size': 'Controller|Configuration|Grid Size',
            'Axis Color': 'Controller|Configuration|Axis Color',
            'Primary Grid Spacing': 'Controller|Configuration|Primary Grid Spacing',
            'Primary Color': 'Controller|Configuration|Primary Color',
            'Secondary Grid Spacing': 'Controller|Configuration|Secondary Grid Spacing',
            'Secondary Color': 'Controller|Configuration|Secondary Color',
        }
        return properties[property]

    @staticmethod
    def hair(property: str = 'name') -> str:
        """
        Atom Hair component properties. Requires Actor component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.
          - 'Hair Asset' Asset.id of the hair TressFX asset.
          - 'Enable Area Lights' (bool default True)
          - 'Enable Azimuth' Azimuth Contribution (bool default True)
          - 'Enable Directional Lights' (bool default True)
          - 'Enable IBL' imaged-based lighting for hair (bool default True)
          - 'Enable Longitude' Longitude Contribution (bool default True)
          - 'Enable Marschner R' (bool default True)
          - 'Enable Marschner TRT' (bool default True)
          - 'Enable Marschner TT' (bool default True)
          - 'Enable Punctual Lights' (bool default True)
          - 'Enable Shadows' (bool default True)
          - 'Hair Lighting Model' simulation algorithm selected from atom_constants.py HAIR_LIGHTING_MODEL (default 'Marschner')
          - 'Base Albedo Asset' Asset.id of the base albedo texture asset (streamingimage or supported texture format)
          - 'Base Color' base color of the hair (math.Color RGBA, default 255,255,255,161)
          - 'Enable Hair LOD' Level of Detail usage for the hair (bool default False)
          - 'Enable Hair LOD(Shadow)' Level of Detail usage for the shadow of hair (bool default False)
          - 'Enable Strand Tangent' (bool default False)
          - 'Enable Strand UV' usage of Strand Albedo (bool default False)
          - 'Enable Thin Tip' end of the hair will narrow or be squared off (bool default True)
          - 'Fiber Radius' Diameter of the fiber (float 0.0 to 0.01, default 0.002)
          - 'Fiber Spacing' spacing between the fibers (float 0.0.to 1.0, default 0.4)
          - 'Fiber ratio' extent to which the hair strand will taper (float 0.01 to 1.0, default 0.06)
          - 'Hair Cuticle Angle' determins how the light refraction behaves (float radians 0.05 to 0.15, default 0.08)
          - 'Hair Ex1' Specular power to use for the calculated specular root value (float 0.0 to 100.0, default 14.4)
          - 'Hair Ex2' Specular power to use for the calculated specular tip value (float 0.0 to 100.0, default 11.8)
          - 'Hair Kdiffuse' Diffuse coefficient, think of it as a gain value (float 0.0 to 1.0, deafult 0.22)
          - 'Hair Ks1' Primary specular reflection coefficient (float 0.0 to 1.0, default 0.001)
          - 'Hair Ks2' Secondary specular reflection coefficient (float 0.0 to 1.0, default 0.136)
          - 'Hair Roughness' (float 0.4 to 0.9, default 0.65)
          - 'Hair Shadow Alpha' attenuate hair shadows based on depth into strands (float 0.0 to 1.0, default 0.35)
          - 'LOD End Distance' Distance in centimeters where LOD will be its maximum reduction/multiplier (float 0.0 to inf, default 5.0)
          - 'LOD Start Distance' Distance to begin LOD in centimeters camera to hair. (float 0.0 to inf, default 1.0)
          - 'Mat Tip Color' blend from root to tip (math.Color RGBA, default 255,255,255,161)
          - 'Max LOD Reduction' Maximum amount of reduction as a percentage of the original (float 0.0 to 1.0, default 0.5)
          - 'Max LOD Strand Width Multiplier' Maximum amount the strand width would be multiplied by (float 0.0 to 10.0, default 2.0)
          - 'Max Shadow Fibers' shadow attenuation calculation cutoff (int 0 to 100, default 50)
          - 'Shadow LOD End Distance' Distance where shadow LOD should be at its maximum (float 0.0 to inf, default 5.0)
          - 'Shadow LOD Start Distance' Distance to begin shadow LOD (float 0.0 to inf, default 1.0)
          - 'Shadow Max LOD Reduction' max reduction as a percentage of the original (float 0.0 to 1.0, default 0.5)
          - 'Shadow Max LOD Strand Width Multiplier' max amount the shadow width cast by the strand would be multiplied by (float 0.0 to 10.0, default 2.0)
          - 'Strand Albedo Asset' Asset.id of the texture asset used for strands (streamingimage)
          - 'Strand UVTiling Factor' Amount of tiling to use (float 0.0 to 10.0, default 1.0)
          - 'Tip Percentage' amount of lerp blend between Base Scalp Albedo and Mat Tip Color (float 0.0 default to 1.0)
          - 'Clamp Velocity' limits the displacement of hair segments per frame (float 1.0 to 24.0, default 20.0)
          - 'Damping' smooths out the motion of the hair (float 0.0 to 1.0, default 0.08)
          - 'Global Constraint Range' global shape stiffness (float 0.0 to 1.0, default 0.308)
          - 'Global Constraint Stiffness' stiffness of a strand (float 0.0 to 1.0, default 0.408)
          - 'Gravity Magnitude' gravitational pseudo value approximating force on strands (float 0.0 to 1.0 default 0.19)
          - 'Length Constraint Iterations' simulation time (iterations) toward keeping the global hair shape (int 1 to 10, default 3)
          - 'Local Constraint Iterations' more simulation time (iterations) toward keeping the local hair shape (int 1 to 10, default 3)
          - 'Local Constraint Stiffness' Controls the stiffness of a strand (float 0.0 to 1.0, default 0.908)
          - 'Tip Separation' Forces the tips of the strands away from each other (float 0.0 to 1.0, default 0.1)
          - 'Vsp Accel Threshold' Velocity Shock Propagation acceleration threshold (float 0.0 to 10.0, default 1.208)
          - 'Vsp Coeffs' Velocity Shock Propagation (float 0.0 to 1.0, default 0.758)
          - 'Wind Angle Radians' (float radians 0.0 to 1.0, default 0.698)
          - 'Wind Direction' (math.Vector3 XYZ world space default 0.0, 1.0, 0.0)
          - 'Wind Magnitude' wind multiplier (float 0.0 default to 1.0)
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Atom Hair',
            'requires': [AtomComponentProperties.actor()],
            'Hair Asset': 'Controller|Configuration|Hair Asset',
            'Enable Area Lights': 'Controller|Configuration|Hair Global Settings|Enable Area Lights',
            'Enable Azimuth': 'Controller|Configuration|Hair Global Settings|Enable Azimuth',
            'Enable Directional Lights': 'Controller|Configuration|Hair Global Settings|Enable Directional Lights',
            'Enable IBL': 'Controller|Configuration|Hair Global Settings|Enable IBL',
            'Enable Longitude': 'Controller|Configuration|Hair Global Settings|Enable Longitude',
            'Enable Marschner R': 'Controller|Configuration|Hair Global Settings|Enable Marschner R',
            'Enable Marschner TRT': 'Controller|Configuration|Hair Global Settings|Enable Marschner TRT',
            'Enable Marschner TT': 'Controller|Configuration|Hair Global Settings|Enable Marschner TT',
            'Enable Punctual Lights': 'Controller|Configuration|Hair Global Settings|Enable Punctual Lights',
            'Enable Shadows': 'Controller|Configuration|Hair Global Settings|Enable Shadows',
            'Hair Lighting Model': 'Controller|Configuration|Hair Global Settings|Hair Lighting Model',
            'Base Albedo Asset': 'Controller|Configuration|TressFX Render Settings|Base Albedo Asset',
            'Base Color': 'Controller|Configuration|TressFX Render Settings|Base Color',
            'Enable Hair LOD': 'Controller|Configuration|TressFX Render Settings|Enable Hair LOD',
            'Enable Hair LOD(Shadow)': 'Controller|Configuration|TressFX Render Settings|Enable Hair LOD(Shadow)',
            'Enable Strand Tangent': 'Controller|Configuration|TressFX Render Settings|Enable Strand Tangent',
            'Enable Strand UV': 'Controller|Configuration|TressFX Render Settings|Enable Strand UV',
            'Enable Thin Tip': 'Controller|Configuration|TressFX Render Settings|Enable Thin Tip',
            'Fiber Radius': 'Controller|Configuration|TressFX Render Settings|Fiber Radius',
            'Fiber Spacing': 'Controller|Configuration|TressFX Render Settings|Fiber Spacing',
            'Fiber ratio': 'Controller|Configuration|TressFX Render Settings|Fiber ratio',
            'Hair Cuticle Angle': 'Controller|Configuration|TressFX Render Settings|Hair Cuticle Angle',
            'Hair Ex1': 'Controller|Configuration|TressFX Render Settings|Hair Ex1',
            'Hair Ex2': 'Controller|Configuration|TressFX Render Settings|Hair Ex2',
            'Hair Kdiffuse': 'Controller|Configuration|TressFX Render Settings|Hair Kdiffuse',
            'Hair Ks1': 'Controller|Configuration|TressFX Render Settings|Hair Ks1',
            'Hair Ks2': 'Controller|Configuration|TressFX Render Settings|Hair Ks2',
            'Hair Roughness': 'Controller|Configuration|TressFX Render Settings|Hair Roughness',
            'Hair Shadow Alpha': 'Controller|Configuration|TressFX Render Settings|Hair Shadow Alpha',
            'LOD End Distance': 'Controller|Configuration|TressFX Render Settings|LOD End Distance',
            'LOD Start Distance': 'Controller|Configuration|TressFX Render Settings|LOD Start Distance',
            'Mat Tip Color': 'Controller|Configuration|TressFX Render Settings|Mat Tip Color',
            'Max LOD Reduction': 'Controller|Configuration|TressFX Render Settings|Max LOD Reduction',
            'Max LOD Strand Width Multiplier': 'Controller|Configuration|TressFX Render Settings|Max LOD Strand Width Multiplier',
            'Max Shadow Fibers': 'Controller|Configuration|TressFX Render Settings|Max Shadow Fibers',
            'Shadow LOD End Distance': 'Controller|Configuration|TressFX Render Settings|Shadow LOD End Distance',
            'Shadow LOD Start Distance': 'Controller|Configuration|TressFX Render Settings|Shadow LOD Start Distance',
            'Shadow Max LOD Reduction': 'Controller|Configuration|TressFX Render Settings|Shadow Max LOD Reduction',
            'Shadow Max LOD Strand Width Multiplier': 'Controller|Configuration|TressFX Render Settings|Shadow Max LOD Strand Width Multiplier',
            'Strand Albedo Asset': 'Controller|Configuration|TressFX Render Settings|Strand Albedo Asset',
            'Strand UVTiling Factor': 'Controller|Configuration|TressFX Render Settings|Strand UVTiling Factor',
            'Tip Percentage': 'Controller|Configuration|TressFX Render Settings|Tip Percentage',
            'Clamp Velocity': 'Controller|Configuration|TressFX Sim Settings|Clamp Velocity',
            'Damping': 'Controller|Configuration|TressFX Sim Settings|Damping',
            'Global Constraint Range': 'Controller|Configuration|TressFX Sim Settings|Global Constraint Range',
            'Global Constraint Stiffness': 'Controller|Configuration|TressFX Sim Settings|Global Constraint Stiffness',
            'Gravity Magnitude': 'Controller|Configuration|TressFX Sim Settings|Gravity Magnitude',
            'Length Constraint Iterations': 'Controller|Configuration|TressFX Sim Settings|Length Constraint Iterations',
            'Local Constraint Iterations': 'Controller|Configuration|TressFX Sim Settings|Local Constraint Iterations',
            'Local Constraint Stiffness': 'Controller|Configuration|TressFX Sim Settings|Local Constraint Stiffness',
            'Tip Separation': 'Controller|Configuration|TressFX Sim Settings|Tip Separation',
            'Vsp Accel Threshold': 'Controller|Configuration|TressFX Sim Settings|Vsp Accel Threshold',
            'Vsp Coeffs': 'Controller|Configuration|TressFX Sim Settings|Vsp Coeffs',
            'Wind Angle Radians': 'Controller|Configuration|TressFX Sim Settings|Wind Angle Radians',
            'Wind Direction': 'Controller|Configuration|TressFX Sim Settings|Wind Direction',
            'Wind Magnitude': 'Controller|Configuration|TressFX Sim Settings|Wind Magnitude',
        }
        return properties[property]

    @staticmethod
    def hdr_color_grading(property: str = 'name') -> str:
        """
        HDR Color Grading component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
          - 'Enable HDR color grading' Toggle active state of the component. (bool)
          - 'Color Adjustment Weight' Level of influence for Color Adjustment parameters. (0, 1)
          - 'Exposure' Brightness/Darkness of the scene. (-INF, INF)
          - 'Contrast' Lowers/Enhances the difference in color of the scene. (-100, 100)
          - 'Pre Saturation' Controls base color saturation before further modification. (-100, 100)
          - 'Filter Intensity' Brightness of the color filter. (-INF, INF)
          - 'Filter Multiply'  Enables and controls strength of the color filter. (0, 1)
          - 'Filter Swatch' Determines color of the color filter applied to the scene. (Vector3)
          - 'White Balance Weight' Level of influence for White Balance parameters. (0, 1)
          - 'Temperature' Color temperature in Kelvin. (1000, 40000)
          - 'Tint' Changes the tint of the scene from Neutral (0) to Magenta (-100) or Green (100). (-100, 100)
          - 'Luminance Preservation' Maintains the relative brightness of the scene
            before applying Color Grading. (0, 1)
          - 'Split Toning Weight' Level of influence for Split Toning parameters. (0, 1)
          - 'Balance' Determines the level of light interpreted as Shadow  or Highlight. (-1, 1)
          - 'Split Toning Shadows Color' Shadows are toned to this color. (Vector3)
          - 'Split Toning Highlights Color' Highlights are toned to this color.
          - 'SMH Weight'  Level of influence for Shadow Midtones Highlights parameters. (0, 1)
          - 'Shadows Start' Minimum brightness to interpret as Shadow. (0, 16)
          - 'Shadows End' Maximum brightness to interpret as Shadow. (0, 16)
          - 'Highlights Start' Minimum brightness to interpret as Highlight. (0, 16)
          - 'Highlights End' Maximum brightness to interpret as Shadow. (0, 16)
          - 'SMH Shadows Color' Shadow interpreted areas set to this color. (Vector3)
          - 'SMH Midtones Color' Midtone interpreted areas set to this color. (Vector3)
          - 'SMH Highlights Color' Highlight interpreted areas set to this color. (Vector3)
          - 'Channel Mixing Red' Color Channels interpreted as Red. (Vector3)
          - 'Channel Mixing Green' Color Channels interpreted as Green. (Vector3)
          - 'Channel Mixing Blue' Color channels interpreted as Blue. (Vector3)
          - 'Final Adjustment Weight' Level of influence for Final Adjustment parameters parameters. (0, 1)
          - 'Post Saturation' Controls color saturation after modification. (-100, 100)
          - 'Hue Shift' Shifts all color by 1% of a rotation in the color wheel per 0.01. (0.0, 1.0)
          - 'LUT Resolution' Resolution of generated LUT from atom_constants.py LUT_RESOLUTION.
          - 'Shaper Type' Shaper type used for the generated LUT from atom_constants.py SHAPER_TYPE.
          - 'Generated LUT Path' absolute path to the generated look up table file (read-only)
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'HDR Color Grading',
            'requires': [AtomComponentProperties.postfx_layer()],
            'Enable HDR color grading': 'Controller|Configuration|Enable HDR color grading',
            'Color Adjustment Weight': 'Controller|Configuration|Color Adjustment|Weight',
            'Exposure': 'Controller|Configuration|Color Adjustment|Exposure',
            'Contrast': 'Controller|Configuration|Color Adjustment|Contrast',
            'Pre Saturation': 'Controller|Configuration|Color Adjustment|Pre Saturation',
            'Filter Intensity': 'Controller|Configuration|Color Adjustment|Filter Intensity',
            'Filter Multiply': 'Controller|Configuration|Color Adjustment|Filter Multiply',
            'Filter Swatch': 'Controller|Configuration|Color Adjustment|Filter Swatch',
            'White Balance Weight': 'Controller|Configuration|White Balance|Weight',
            'Temperature': 'Controller|Configuration|White Balance|Temperature',
            'Tint': 'Controller|Configuration|White Balance|Tint',
            'Luminance Preservation': 'Controller|Configuration|White Balance|Luminance Preservation',
            'Split Toning Weight': 'Controller|Configuration|Split Toning|Weight',
            'Balance': 'Controller|Configuration|Split Toning|Balance',
            'Split Toning Shadows Color': 'Controller|Configuration|Split Toning|Shadows Color',
            'Split Toning Highlights Color': 'Controller|Configuration|Split Toning|Highlights Color',
            'SMH Weight': 'Controller|Configuration|Shadow Midtones Highlights|Weight',
            'Shadows Start': 'Controller|Configuration|Shadow Midtones Highlights|Shadows Start',
            'Shadows End': 'Controller|Configuration|Shadow Midtones Highlights|Shadows End',
            'Highlights Start': 'Controller|Configuration|Shadow Midtones Highlights|Highlights Start',
            'Highlights End': 'Controller|Configuration|Shadow Midtones Highlights|Highlights End',
            'SMH Shadows Color': 'Controller|Configuration|Shadow Midtones Highlights|Shadows Color',
            'SMH Midtones Color': 'Controller|Configuration|Shadow Midtones Highlights|Midtones Color',
            'SMH Highlights Color': 'Controller|Configuration|Shadow Midtones Highlights|Highlights Color',
            'Channel Mixing Red': 'Controller|Configuration|Channel Mixing|Channel Mixing Red',
            'Channel Mixing Green': 'Controller|Configuration|Channel Mixing|Channel Mixing Green',
            'Channel Mixing Blue': 'Controller|Configuration|Channel Mixing|Channel Mixing Blue',
            'Final Adjustment Weight': 'Controller|Configuration|Final Adjustment|Weight',
            'Post Saturation': 'Controller|Configuration|Final Adjustment|Post Saturation',
            'Hue Shift': 'Controller|Configuration|Final Adjustment|Hue Shift',
            'LUT Resolution': 'Controller|Configuration|LUT Generation|LUT Resolution',
            'Shaper Type': 'Controller|Configuration|LUT Generation|Shaper Type',
            'Generated LUT Path': 'LUT Generation|Generated LUT Path',
        }
        return properties[property]

    @staticmethod
    def hdri_skybox(property: str = 'name') -> str:
        """
        HDRi Skybox component properties.
          - 'Cubemap Texture': Asset.id for the texture used in cubemap rendering (File Type *.exr.streamingimage).
          - 'Exposure': Light exposure value for HDRi Skybox projection ('float', range -5.0 - 5.0, default 0.0).
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'HDRi Skybox',
            'Cubemap Texture': 'Controller|Configuration|Cubemap Texture',
            'Exposure': 'Controller|Configuration|Exposure',
        }
        return properties[property]

    @staticmethod
    def light(property: str = 'name') -> str:
        """
        Light component properties.
          - 'Light type' from atom_constants.py LIGHT_TYPES
          - 'Color' the RGBA value to set for the color of the light using azlmbr.math.Color tuple.
          - 'Intensity mode' from atom_constants.py INTENSITY_MODE
          - 'Intensity' the intensity of the light in the set photometric unit (float with no ceiling).
          - 'Attenuation radius Mode' from atom_constants.py ATTENUATION_RADIUS_MODE
          - 'Attenuation radius Radius' sets the distance at which the light no longer has effect.
          - 'Enable shadow' toggle for enabling shadows for the light.
          - 'Shadows Bias' how deep in shadow a surface must be before being affected by it (Range 0-100).
          - 'Normal Shadow Bias' reduces shadow acne (Range 0-10).
          - 'Shadowmap size' from atom_constants.py SHADOWMAP_SIZE
          - 'Shadow filter method' from atom_constants.py SHADOW_FILTER_METHOD
          - 'Filtering sample count' smooths/hardens shadow edges - specific to PCF & ESM+PCF (Range 4-64).
          - 'ESM exponent' smooths/hardens shadow edges - specific to ESM & ESM+PCF (Range 50-5000).
          - 'Enable shutters' toggle for enabling shutters for the light.
          - 'Inner angle' inner angle value for the shutters (in degrees) (Range 0.5-90).
          - 'Outer angle' outer angle value for the shutters (in degrees) (Range 0.5-90).
          - 'Both directions' Enables/Disables light emitting from both sides of a surface.
          - 'Fast approximation' Enables/Disables fast approximation of linear transformed cosines.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Light',
            'Light type': 'Controller|Configuration|Light type',
            'Color': 'Controller|Configuration|Color',
            'Intensity mode': 'Controller|Configuration|Intensity mode',
            'Intensity': 'Controller|Configuration|Intensity',
            'Attenuation radius Mode': 'Controller|Configuration|Attenuation radius|Mode',
            'Attenuation radius Radius': 'Controller|Configuration|Attenuation radius|Radius',
            'Enable shadow': 'Controller|Configuration|Shadows|Enable shadow',
            'Shadows Bias': 'Controller|Configuration|Shadows|Bias',
            'Normal shadow bias': 'Controller|Configuration|Shadows|Normal shadow bias',
            'Shadowmap size': 'Controller|Configuration|Shadows|Shadowmap size',
            'Shadow filter method': 'Controller|Configuration|Shadows|Shadow filter method',
            'Filtering sample count': 'Controller|Configuration|Shadows|Filtering sample count',
            'ESM exponent': 'Controller|Configuration|Shadows|ESM exponent',
            'Enable shutters': 'Controller|Configuration|Shutters|Enable shutters',
            'Inner angle': 'Controller|Configuration|Shutters|Inner angle',
            'Outer angle': 'Controller|Configuration|Shutters|Outer angle',
            'Both directions': 'Controller|Configuration|Both directions',
            'Fast approximation': 'Controller|Configuration|Fast approximation',

        }
        return properties[property]

    @staticmethod
    def look_modification(property: str = 'name') -> str:
        """
        Look Modification component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
          - 'Enable look modification' Toggle active state of the component True/False
          - 'Color Grading LUT' Asset.id for the LUT used for affecting level look.
          - 'Shaper Type' Shaper type used for scene look modification from atom_constants.py SHAPER_TYPE.
          - 'LUT Intensity' Overall influence of the LUT on the scene. (0.0, 1.0)
          - 'LUT Override' Blend intensity of the LUT (for use with multiple Look Modification entities). (0.0, 1.0)
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Look Modification',
            'requires': [AtomComponentProperties.postfx_layer()],
            'Enable look modification': 'Controller|Configuration|Enable look modification',
            'Color Grading LUT': 'Controller|Configuration|Color Grading LUT',
            'Shaper Type': 'Controller|Configuration|Shaper Type',
            'LUT Intensity': 'Controller|Configuration|LUT Intensity',
            'LUT Override': 'Controller|Configuration|LUT Override',
        }
        return properties[property]

    @staticmethod
    def material(property: str = 'name') -> str:
        """
        Material component properties. Requires one of Actor OR Mesh component.
          - 'requires' a list of component names as strings required by this component.
            Only one of these is required at a time for this component.\n
          - 'Material Asset': the default material Asset.id. Overrides all Model and LOD materials.
          - 'Enable LOD Materials' toggles the use of LOD Materials.
          - 'LOD Materials' container property for specified LOD materials. (list of EditorMaterialComponentSlot)
          - 'Model Materials' container property of materials included with model. (EditorMaterialComponentSlot)
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Material',
            'requires': [AtomComponentProperties.actor(), AtomComponentProperties.mesh()],
            'Material Asset': 'Default Material|Material Asset',
            'Enable LOD Materials': 'Enable LOD Materials',
            'LOD Materials': 'LOD Materials',
            'Model Materials': 'Model Materials',
        }
        return properties[property]

    @staticmethod
    def mesh(property: str = 'name') -> str:
        """
        Mesh component properties.
          - 'Model Asset' Asset.id of the mesh model.
          - 'Mesh Asset' Mesh Asset is deprecated in favor of Model Asset, but this property will continue
             to exist and map to the new Model Asset property to keep older tests working.
          - 'Sort Key' dis-ambiguates which mesh renders in front of others (signed int 64)
          - 'Use ray tracing' Toggles interaction with ray tracing (bool)
          - 'Lod Type' options: default, screen coverage, specific lod
          - 'Exclude from reflection cubemaps' Toggles inclusion in generated cubemaps (bool)
          - 'Use Forward Pass IBL Specular' Toggles Forward Pass IBL Specular (bool)
          - 'Minimum Screen Coverage' portion of the screen at which the mesh is culled; 0 (never culled) to 1
          - 'Quality Decay Rate' rate at which the mesh degrades; 0 (never) to 1 (lowest quality imediately)
          - 'Lod Override' which specific LOD to always use; default or other named LOD
          - 'Vertex Count LOD0' the vertices in LOD 0 of the model
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        :rtype: str
        """
        properties = {
            'name': 'Mesh',
            'Model Asset': 'Controller|Configuration|Model Asset',
            'Mesh Asset': 'Controller|Configuration|Model Asset',
            'Sort Key': 'Controller|Configuration|Sort Key',
            'Use ray tracing': 'Controller|Configuration|Use ray tracing',
            'Lod Type': 'Controller|Configuration|Lod Type',
            'Exclude from reflection cubemaps': 'Controller|Configuration|Exclude from reflection cubemaps',
            'Use Forward Pass IBL Specular': 'Controller|Configuration|Use Forward Pass IBL Specular',
            'Minimum Screen Coverage': 'Controller|Configuration|Lod Configuration|Minimum Screen Coverage',
            'Quality Decay Rate': 'Controller|Configuration|Lod Configuration|Quality Decay Rate',
            'Lod Override': 'Controller|Configuration|Lod Configuration|Lod Override',
            'Vertex Count LOD0': 'Model Stats|Mesh Stats|LOD 0|Vert Count'
        }
        return properties[property]

    @staticmethod
    def occlusion_culling_plane(property: str = 'name') -> str:
        """
        Occlusion Culling Plane component properties.
          - 'Show Visualization' Toggles the visual display of the Occlusion Culling Plane in edit and game mode (bool)
          - 'Transparent Visualization': Toggles the transparency of the Occlusion Culling Plane when visible (bool)
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Occlusion Culling Plane',
            'Show Visualization': 'Controller|Configuration|Settings|Show Visualization',
            'Transparent Visualization': 'Controller|Configuration|Settings|Transparent Visualization',
        }
        return properties[property]

    @staticmethod
    def physical_sky(property: str = 'name') -> str:
        """
        Physical Sky component properties.
        - 'Intensity Mode' Specifying the light unit type (emum, Ev100, Nit, default Ev100).
        - 'Sky Intensity' Brightness of the sky (float, range -4.0 - 11.0, default 4.0).
        - 'Sun Intensity' Brightness of the sun (float, range -4.0 - 11.0, default 8.0).
        - 'Turbidity' A measure of the aerosol content in the air (int, range 1-10, default of 1).
        - 'Sun Radius Factor' A factor for Physical sun radius in millions of km. 1 unit is 695,508 km
         (float, range 0.1 - 2, default 1.0). /n
        - 'Enable Fog' Toggle fog on or off (bool, default False).
        - 'Fog Color' Color of the fog (math.Color(float x, float y, float z, float a) where ranges are 0 to 255).
        - 'Fog Top Height' Height of the fog upwards from the horizon (float, range 0.0 - 0.5 default 0.01).
        - 'Fog Bottom Height' Height of the fog downwards from the horizon (float, range 0.0 - 0.3 default 0.0).
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Physical Sky',
            'Intensity Mode': 'Controller|Configuration|Intensity Mode',
            'Sky Intensity': 'Controller|Configuration|Sky Intensity',
            'Sun Intensity': 'Controller|Configuration|Sun Intensity',
            'Turbidity': 'Controller|Configuration|Turbidity',
            'Sun Radius Factor': 'Controller|Configuration|Sun Radius Factor',
            'Enable Fog': 'Controller|Configuration|Fog|Enable Fog',
            'Fog Color': 'Controller|Configuration|Fog|Fog Color',
            'Fog Top Height': 'Controller|Configuration|Fog|Fog Top Height',
            'Fog Bottom Height': 'Controller|Configuration|Fog|Fog Bottom Height',
        }
        return properties[property]

    @staticmethod
    def postfx_layer(property: str = 'name') -> str:
        """
        PostFX Layer component properties.
          - 'Layer Category' frequency at which the settings will be applied from atom_constants.py POSTFX_LAYER_CATEGORY
          - 'Priority' this will take over other settings with the same frequency. lower takes precedence (int)
          - 'Weight' how much these settings override previous settings. (float 0.0 to default 1.0)
          - 'Select Camera Tags Only' property container list of tags.
            Only cameras with these tags will include this effect.
          - 'Excluded Camera Tags' property container list of tags.
            Cameras with these tags will not be included in the effect.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'PostFX Layer',
            'Layer Category': 'Controller|Configuration|Layer Category',
            'Priority': 'Controller|Configuration|Priority',
            'Weight': 'Controller|Configuration|Weight',
            'Select Camera Tags Only': 'Controller|Configuration|Select Camera Tags Only',
            'Excluded Camera Tags': 'Controller|Configuration|Excluded Camera Tags',
        }
        return properties[property]

    @staticmethod
    def postfx_gradient(property: str = 'name') -> str:
        """
        PostFX Gradient Weight Modifier component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.
          - 'Gradient Entity Id' a separate entity id containing a gradient component.
          - 'Opacity' factor multiplied by the current gradient before mixing. (float 0.0 to 1.0)
          - 'Invert Input' swap the gradient input order black/white behave oppositely (bool)
          - 'Enable Levels' toggle the application of input/output levels (bool)
          - 'Input Max' adjustment to the white point for the input
            treating more of the gradient as max value. (float 0.0 to default 1.0)
          - 'Input Min' adjustment to the black point for the input
            treating more of the gradient as min value. (float 0.0 default to 1.0)
          - 'Input Mid' adjustment to the midtone point for the input
            effecting all values of the gradient to be more toward min or max. (float 0.0 to 10.0, default 1.0)
          - 'Output Max' adjusts the output white point of the effective gradient after input levels are applied
            (float 0.0 to default 1.0)
          - 'Output Min' adjusts the output black point of the effective gradient after input levels are applied
            (float 0.0 default to 1.0)
          - 'Enable Transform' toggle the ability to apply transform to the gradient input (bool)
          - 'Scale' adjusts the gradient size (Vector3 default 1.0,1.0,1.0)
          - 'Rotate' rotates the gradient (Vector3 rotation degrees; default 0.0,0.0,0.0)
          - 'Translate' moves the gradient position (Vector3 default 0.0,0.0,0.0)
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'PostFX Gradient Weight Modifier',
            'requires': [AtomComponentProperties.postfx_layer()],
            'Gradient Entity Id': 'Controller|Configuration|Gradient Sampler|Gradient Entity Id',
            'Opacity': 'Controller|Configuration|Gradient Sampler|Opacity',
            'Invert Input': 'Controller|Configuration|Gradient Sampler|Advanced|Invert Input',
            'Enable Levels': 'Controller|Configuration|Gradient Sampler|Enable Levels',
            'Input Max': 'Controller|Configuration|Gradient Sampler|Enable Levels|Input Max',
            'Input Min': 'Controller|Configuration|Gradient Sampler|Enable Levels|Input Min',
            'Input Mid': 'Controller|Configuration|Gradient Sampler|Enable Levels|Input Mid',
            'Output Max': 'Controller|Configuration|Gradient Sampler|Enable Levels|Output Max',
            'Output Min': 'Controller|Configuration|Gradient Sampler|Enable Levels|Output Min',
            'Enable Transform': 'Controller|Configuration|Gradient Sampler|Enable Transform',
            'Scale': 'Controller|Configuration|Gradient Sampler|Enable Transform|Scale',
            'Rotate': 'Controller|Configuration|Gradient Sampler|Enable Transform|Rotate',
            'Translate': 'Controller|Configuration|Gradient Sampler|Enable Transform|Translate',
        }
        return properties[property]

    @staticmethod
    def postfx_radius(property: str = 'name') -> str:
        """
        PostFX Radius Weight Modifier component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.
          - 'Radius' Radius of the PostFX modification (float deafult 0.0 to infinity)
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'PostFX Radius Weight Modifier',
            'requires': [AtomComponentProperties.postfx_layer()],
            'Radius': 'Controller|Configuration|Radius',
        }
        return properties[property]

    @staticmethod
    def postfx_shape(property: str = 'name') -> str:
        """
        PostFX Shape Weight Modifier component properties. Requires PostFX Layer and one of 'shapes' listed.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.
          - 'shapes' a list of supported shapes as component names. 'Tube Shape' is also supported but requires 'Spline'.
          - 'Fall-off Distance' Distance from the shape to smoothly transition the PostFX.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'PostFX Shape Weight Modifier',
            'requires': [AtomComponentProperties.postfx_layer()],
            'shapes': ['Axis Aligned Box Shape', 'Box Shape', 'Capsule Shape', 'Compound Shape', 'Cylinder Shape',
                       'Disk Shape', 'Polygon Prism Shape', 'Quad Shape', 'Sphere Shape', 'Shape Reference'],
            'Fall-off Distance': 'Controller|Configuration|Fall-off Distance',
        }
        return properties[property]

    @staticmethod
    def reflection_probe(property: str = 'name') -> str:
        """
        Reflection Probe component properties. Requires one of 'shapes' listed.
          - 'shapes' a list of supported shapes as component names.
          - 'Bake Exposure' Used when baking the cubemap. (float -20.0 to 20.0)
          - 'Parallax Correction' Toggles between preauthored cubemap and one that captures at the location. (bool)
          - 'Show Visualization' Toggles the reflection probe visualization sphere. (bool)
          - 'Settings Exposure' Used when rendering meshes with the cubemap. (float -20.0 to 20.0)
          - 'Use Baked Cubemap' Toggles between preauthored cubemap and one that captures at the location. (bool)
          - 'Baked Cubemap Quality' resolution of the baked cubemap from atom_constants.py BAKED_CUBEMAP_QUALITY.
          - 'Height' Inner extents constrained by the shape dimensions attached to the entity.
          - 'Length' Inner extents constrained by the shape dimensions attached to the entity.
          - 'Width' Inner extents constrained by the shape dimensions attached to the entity.
          - 'Baked Cubemap Path' Read-only Path of baked cubemap image generated by calling 'BakeReflectionProbe' ebus.
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Reflection Probe',
            'shapes': ['Axis Aligned Box Shape', 'Box Shape'],
            'Bake Exposure': 'Cubemap Bake|Bake Exposure',
            'Parallax Correction': 'Controller|Configuration|Settings|Parallax Correction',
            'Show Visualization': 'Controller|Configuration|Settings|Show Visualization',
            'Settings Exposure': 'Controller|Configuration|Settings|Exposure',
            'Use Baked Cubemap': 'Cubemap|Use Baked Cubemap',
            'Baked Cubemap Quality': 'Cubemap|Baked Cubemap Quality',
            'Height': 'Controller|Configuration|Inner Extents|Height',
            'Length': 'Controller|Configuration|Inner Extents|Length',
            'Width': 'Controller|Configuration|Inner Extents|Width',
            'Baked Cubemap Path': 'Cubemap|Baked Cubemap Path',
        }
        return properties[property]

    @staticmethod
    def sky_atmosphere(property: str = 'name') -> str:
        """
        Sky Atmosphere component properties
          - 'Ground albedo' Additional light from the surface of the ground (Vector3 float) default (0.0,0.0,0.0)
          - 'Ground radius' Kilometers 0.0 to 100000.0 (float) default 6360.0
          - 'Origin' The origin to use for the atmosphere (int)
          - 'Atmosphere height' Kilometers 0.0 to 10000.0 (float) default 100.0
          - 'Illuminance factor' An additional factor to brighten or darken the overall atmosphere (Vector3 float) default (1.0,1.0,1.0)
          - 'Mie absorption scale' 0.0 to 1.0 (float) default 0.004
          - 'Mie absorption' (Vector3 float) default (1.0,1.0,1.0)
          - 'Mie exponential distribution' Altitude in kilometers at which Mie scattering is reduced to roughly 40%. 0.0 to 400.0 (float) default 1.2
          - 'Mie scattering scale' 0.0 to 1.0 (float) default 0.004
          - 'Mie scattering' Mie scattering coefficients from aerosole molecules at surface of the planet. (Vector3 float) default (1.0,1.0,1.0)
          - 'Ozone absorption scale' Ozone molecule absorption scale 0.0 to 1.0 (float) default 0.001881
          - 'Ozone absorption' Absorption coefficients from ozone molecules (Vector3 float) default (1.0,1.0,1.0)
          - 'Rayleigh exponential distribution' Altitude in kilometers at which Rayleigh scattering is reduced to roughly 40%. 0.0 to 400.0 (float) default 8.0
          - 'Rayleigh scattering scale' 0.0 to 1.0 (float) default 0.033100f
          - 'Rayleigh scattering' Raleigh scattering coefficients from air molecules at surface of the planet. (Vector3 float) default (1.0,1.0,1.0)
          - 'Show sun' display a sun (bool) default True
          - 'Sun color' (azlmbr.math.Color RGBA) default (255.0,255.0,255.0,255.0)
          - 'Sun falloff factor' 0.0 to 200.0 (float) default 1.0
          - 'Sun limb color' for adjusting outer edge color of sun. (azlmbr.math.Color RGBA) default (255.0,255.0,255.0,255.0)
          - 'Sun luminance factor' 0.0 to 100000.0 (float) default 0.05
          - 'Sun orientation' Optional sun entity to use for orientation (EntityId)
          - 'Sun radius factor' 0.0 to 100.0 (float) default 1.0
          - 'Enable shadows' (bool) default False
          - 'Fast sky' (bool) default True
          - 'Max samples' 1 to 64 (unsigned int) default 14
          - 'Min samples' 1 to 64 (unsigned int) default 4
          - 'Near clip' 0.0 to inf (float) default 0.0
          - 'Near fade distance' 0.0 to inf (float) default 0.0
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Sky Atmosphere',
            'Ground albedo': 'Controller|Configuration|Planet|Ground albedo',
            'Ground radius': 'Controller|Configuration|Planet|Ground radius',
            'Origin': 'Controller|Configuration|Planet|Origin',
            'Atmosphere height': 'Controller|Configuration|Atmosphere|Atmosphere height',
            'Illuminance factor': 'Controller|Configuration|Atmosphere|Illuminance factor',
            'Mie absorption scale': 'Controller|Configuration|Atmosphere|Mie absorption scale',
            'Mie absorption': 'Controller|Configuration|Atmosphere|Mie absorption',
            'Mie exponential distribution': 'Controller|Configuration|Atmosphere|Mie exponential distribution',
            'Mie scattering scale': 'Controller|Configuration|Atmosphere|Mie scattering scale',
            'Mie scattering': 'Controller|Configuration|Atmosphere|Mie scattering',
            'Ozone absorption scale': 'Controller|Configuration|Atmosphere|Ozone absorption scale',
            'Ozone absorption': 'Controller|Configuration|Atmosphere|Ozone absorption',
            'Rayleigh exponential distribution': 'Controller|Configuration|Atmosphere|Rayleigh exponential distribution',
            'Rayleigh scattering scale': 'Controller|Configuration|Atmosphere|Rayleigh scattering scale',
            'Rayleigh scattering': 'Controller|Configuration|Atmosphere|Rayleigh scattering',
            'Show sun': 'Controller|Configuration|Sun|Show sun',
            'Sun color': 'Controller|Configuration|Sun|Sun color',
            'Sun falloff factor': 'Controller|Configuration|Sun|Sun falloff factor',
            'Sun limb color': 'Controller|Configuration|Sun|Sun limb color',
            'Sun luminance factor': 'Controller|Configuration|Sun|Sun luminance factor',
            'Sun orientation': 'Controller|Configuration|Sun|Sun orientation',
            'Sun radius factor': 'Controller|Configuration|Sun|Sun radius factor',
            'Enable shadows': 'Controller|Configuration|Advanced|Enable shadows',
            'Fast sky': 'Controller|Configuration|Advanced|Fast sky',
            'Max samples': 'Controller|Configuration|Advanced|Max samples',
            'Min samples': 'Controller|Configuration|Advanced|Min samples',
            'Near clip': 'Controller|Configuration|Advanced|Near clip',
            'Near fade distance': 'Controller|Configuration|Advanced|Near fade distance',
        }
        return properties[property]

    @staticmethod
    def ssao(property: str = 'name') -> str:
        """
        SSAO component properties. Requires PostFX Layer component.
          - 'requires' a list of component names as strings required by this component.
            Use editor_entity_utils EditorEntity.add_components(list) to add this list of requirements.\n
          - 'Enable SSAO' toggles the overall function of Screen Space Ambient Occlusion (bool)
          - 'SSAO Strength' multiplier for SSAO strenght (float 0.0 to 2.0, default 1.0)
          - 'Sampling Radius' (float 0.0 to 0.25, default 0.05)
          - 'Enable Blur' toggles the blur feature of SSAO (bool)
          - 'Blur Strength' (float 0.0 to 1.0 default 0.85)
          - 'Blur Edge Threshold' (float default 0.0 to 1.0)
          - 'Blur Sharpness' (float 0.0 to 400.0, default 200.0)
          - 'Enable Downsample' toggles downsampling before SSAO; trades quality for speed (bool)
          - 'Enabled Override' toggles a collection of override values (bool)
          - 'Strength Override' (float 0.0 to default 1.0)
          - 'SamplingRadius Override' (float 0.0 to default 1.0)
          - 'EnableBlur Override' toggles blur overrides (bool)
          - 'BlurConstFalloff Override' (float 0.0 to default 1.0)
          - 'BlurDepthFalloffThreshold Override' (float 0.0 to default 1.0)
          - 'BlurDepthFalloffStrength Override' (float 0.0 to default 1.0)
          - 'EnableDownsample Override' toggles override for enable downsampling (bool)
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'SSAO',
            'requires': [AtomComponentProperties.postfx_layer()],
            'Enable SSAO': 'Controller|Configuration|Enable SSAO',
            'SSAO Strength': 'Controller|Configuration|SSAO Strength',
            'Sampling Radius': 'Controller|Configuration|Sampling Radius',
            'Enable Blur': 'Controller|Configuration|Enable Blur',
            'Blur Strength': 'Controller|Configuration|Blur Strength',
            'Blur Edge Threshold': 'Controller|Configuration|Blur Edge Threshold',
            'Blur Sharpness': 'Controller|Configuration|Blur Sharpness',
            'Enable Downsample': 'Controller|Configuration|Enable Downsample',
            'Enabled Override': 'Controller|Configuration|Overrides|Enabled Override',
            'Strength Override': 'Controller|Configuration|Overrides|Strength Override',
            'SamplingRadius Override': 'Controller|Configuration|Overrides|SamplingRadius Override',
            'EnableBlur Override': 'Controller|Configuration|Overrides|EnableBlur Override',
            'BlurConstFalloff Override': 'Controller|Configuration|Overrides|BlurConstFalloff Override',
            'BlurDepthFalloffStrength Override': 'Controller|Configuration|Overrides|BlurDepthFalloffStrength Override',
            'BlurDepthFalloffThreshold Override': 'Controller|Configuration|Overrides|BlurDepthFalloffThreshold Override',
            'EnableDownsample Override': 'Controller|Configuration|Overrides|EnableDownsample Override',
        }
        return properties[property]

    @staticmethod
    def stars(property: str = 'name') -> str:
        """
        Stars component properties
          - 'Stars Asset' Asset.id of the star asset file (default.star)
          - 'Exposure' controls how bright the stars are. 0.0 to 32.0 default 1.0 (float)
          - 'Twinkle rate' how frequently stars twinkle. 0.0 to 10.0 default 0.5 (float)
          - 'Radius factor' star radius multiplier. 0.0 to 64.0 default 7.0 (float)
        :param property: From the last element of the property tree path. Default 'name' for component name string.
        :return: Full property path OR component name if no property specified.
        """
        properties = {
            'name': 'Stars',
            'Stars Asset': 'Controller|Configuration|Stars Asset',
            'Exposure': 'Controller|Configuration|Exposure',
            'Twinkle rate': 'Controller|Configuration|Twinkle rate',
            'Radius factor': 'Controller|Configuration|Radius factor',
        }
        return properties[property]


class AtomToolsDocumentRequestBusEvents(object):
    """
    Used to store string constants representing the bus options for azlmbr.bus.AtomToolsDocumentRequestBus
    """
    GET_ABSOLUTE_PATH = "GetAbsolutePath"
    OPEN = "Open"
    REOPEN = "Reopen"
    CLOSE = "Close"
    SAVE = "Save"
    SAVE_AS_CHILD = "SaveAsChild"
    SAVE_AS_COPY = "SaveAsCopy"
    IS_OPEN = "IsOpen"
    IS_MODIFIED = "IsModified"
    CAN_SAVE_AS_CHILD = "CanSaveAsChild"
    CAN_UNDO = "CanUndo"
    CAN_REDO = "CanRedo"
    UNDO = "Undo"
    REDO = "Redo"
    BEGIN_EDIT = "BeginEdit"
    END_EDIT = "EndEdit"


class AtomToolsDocumentSystemRequestBusEvents(object):
    """
    Used to store string constants representing the bus options for azlmbr.bus.AtomToolsDocumentSystemRequestBus
    """
    CREATE_DOCUMENT_FROM_TYPE_NAME = "CreateDocumentFromTypeName"
    CREATE_DOCUMENT_FROM_FILE_TYPE = "CreateDocumentFromFileType"
    CREATE_DOCUMENT_FROM_FILE_PATH = "CreateDocumentFromFilePath"
    DESTROY_DOCUMENT = "DestroyDocument"
    OPEN_DOCUMENT = "OpenDocument"
    CLOSE_DOCUMENT = "CloseDocument"
    CLOSE_ALL_DOCUMENTS = "CloseAllDocuments"
    CLOSE_ALL_DOCUMENTS_EXCEPT = "CloseAllDocumentsExcept"
    SAVE_DOCUMENT = "SaveDocument"
    SAVE_DOCUMENT_AS_COPY = "SaveDocumentAsCopy"
    SAVE_DOCUMENT_AS_CHILD = "SaveDocumentAsChild"
    SAVE_ALL_DOCUMENTS = "SaveAllDocuments"
    SAVE_ALL_MODIFIED_DOCUMENTS = "SaveAllModifiedDocuments"
    QUEUE_REOPEN_MODIFIED_DOCUMENTS = "QueueReopenModifiedDocuments"
    REOPEN_MODIFIED_DOCUMENTS = "ReopenModifiedDocuments"
    GET_DOCUMENT_COUNT = "GetDocumentCount"
    IS_DOCUMENT_OPEN = "IsDocumentOpen"
    ADD_RECENT_FILE_PATH = "AddRecentFilePath"
    CLEAR_RECENT_FILE_PATHS = "ClearRecentFilePaths"
    SET_RECENT_FILE_PATHS = "SetRecentFilePaths"
    GET_RECENT_FILE_PATHS = "GetRecentFilePaths"


class AtomToolsMainWindowRequestBusEvents(object):
    """
    Used to store string constants representing the bus options for azlmbr.bus.AtomToolsMainWindowRequestBus
    """
    ACTIVATE_WINDOW = "ActivateWindow"
    SET_DOCK_WIDGET_VISIBLE = "SetDockWidgetVisible"
    IS_DOCK_WIDGET_VISIBLE = "IsDockWidgetVisible"
    GET_DOCK_WIDGET_NAMES = "GetDockWidgetNames"
    QUEUE_UPDATE_MENUS = "QueueUpdateMenus"
    SET_STATUS_MESSAGE = "SetStatusMessage"
    SET_STATUS_WARNING = "SetStatusWarning"
    SET_STATUS_ERROR = "SetStatusError"
    RESIZE_VIEWPORT_RENDER_TARGET = "ResizeViewportRenderTarget"
    LOCK_VIEWPORT_RENDER_TARGET_SIZE = "LockViewportRenderTargetSize"
    UNLOCK_VIEWPORT_RENDER_TARGET_SIZE = "UnlockViewportRenderTargetSize"


class EntityPreviewViewportSettingsRequestBusEvents(object):
    """
    Used to store string constants representing the bus options for azlmbr.bus.EntityPreviewViewportSettingsRequestBus
    """
    SET_LIGHTING_PRESET = "SetLightingPreset"
    GET_LIGHTING_PRESET = "GetLightingPreset"
    GET_LAST_LIGHTING_PRESET_ASSET_ID = "GetLastLightingPresetAssetId"
    SAVE_LIGHTING_PRESET = "SaveLightingPreset"
    LOAD_LIGHTING_PRESET = "LoadLightingPreset"
    LOAD_LIGHTING_PRESET_BY_ASSET_ID = "LoadLightingPresetByAssetId"
    GET_LAST_LIGHTING_PRESET_PATH = "GetLastLightingPresetPath"
    GET_LAST_LIGHTING_PRESET_PATH_WITHOUT_ALIAS = "GetLastLightingPresetPathWithoutAlias"
    REGISTER_LIGHTING_PRESET_PATH = "RegisterLightingPresetPath"
    UNREGISTER_LIGHTING_PRESET_PATH = "UnregisterLightingPresetPath"
    GET_REGISTERED_LIGHTING_PRESET_PATHS = "GetRegisteredLightingPresetPaths"
    SET_MODEL_PRESET = "SetModelPreset"
    GET_MODEL_PRESET = "GetModelPreset"
    GET_LAST_MODEL_PRESET_ASSET_ID = "GetLastModelPresetAssetId"
    SAVE_MODEL_PRESET = "SaveModelPreset"
    LOAD_MODEL_PRESET = "LoadModelPreset"
    LOAD_MODEL_PRESET_BY_ASSET_ID = "LoadModelPresetByAssetId"
    GET_LAST_MODEL_PRESET_PATH = "GetLastModelPresetPath"
    GET_LAST_MODEL_PRESET_PATH_WITHOUT_ALIAS = "GetLastModelPresetPathWithoutAlias"
    REGISTER_MODEL_PRESET_PATH = "RegisterModelPresetPath"
    UNREGISTER_MODEL_PRESET_PATH = "UnregisterModelPresetPath"
    GET_REGISTERED_MODEL_PRESET_PATHS = "GetRegisteredModelPresetPaths"
    LOAD_RENDER_PIPELINE = "LoadRenderPipeline"
    LOAD_RENDER_PIPELINE_BY_ASSET_ID = "LoadRenderPipelineByAssetId"
    GET_LAST_RENDER_PIPELINE_PATh = "GetLastRenderPipelinePath"
    GET_LAST_RENDER_PIPELINE_PATH_WITHOUT_ALIAS = "GetLastRenderPipelinePathWithoutAlias"
    REGISTER_RENDER_PIPELINE_PATH = "RegisterRenderPipelinePath"
    UNREGISTER_RENDER_PIPELINE_PATH = "UnregisterRenderPipelinePath"
    GET_REGISTERED_RENDER_PIPELINE_PATHS = "GetRegisteredRenderPipelinePaths"
    SET_SHADOW_CATCHER_ENABLED = "SetShadowCatcherEnabled"
    GET_SHADOW_CATCHER_ENABLED = "GetShadowCatcherEnabled"
    SET_GRID_ENABLED = "SetGridEnabled"
    GET_GRID_ENABLED = "GetGridEnabled"
    SET_ALTERNATE_SKYBOX_ENABLED = "SetAlternateSkyboxEnabled"
    GET_ALTERNATE_SKYBOX_ENABLED = "GetAlternateSkyboxEnabled"
    SET_FIELD_OF_VIEW = "SetFieldOfView"
    GET_FIELD_OF_VIEW = "GetFieldOfView"


class DynamicNodeManagerRequestBusEvents(object):
    """
    Used to store string constants representing the bus options for azlmbr.atomtools.DynamicNodeManagerRequestBus
    """
    LOAD_CONFIG_FILES = "LoadConfigFiles"
    REGISTER_CONFIG = "RegisterConfig"
    GET_CONFIG_BY_ID = "GetConfigById"
    CLEAR = "Clear"
    CREATE_NODE_BY_ID = "CreateNodeById"
    CREATE_NODE_BY_NAME = "CreateNodeByName"


class GraphDocumentRequestBusEvents(object):
    """
    Used to store string constants representing the bus options for azlmbr.atomtools.GraphDocumentRequestBus
    """
    GET_GRAPH = "GetGraph"
    GET_GRAPH_ID = "GetGraphId"
    GET_GRAPH_NAME = "GetGraphName"
    SET_GENERATED_FILE_PATHS = "SetGeneratedFilePaths"
    GET_GENERATED_FILE_PATHS = "GetGeneratedFilePaths"
    COMPILE_GRAPH = "CompileGraph"
    QUEUE_COMPILE_GRAPH = "QueueCompileGraph"
    IS_COMPILE_GRAPH_QUEUED = "IsCompileGraphQueued"


class GraphControllerRequestBusEvents(object):
    """
    Used to store string constants representing the bus options for azlmbr.editor.graph.GraphControllerRequestBus
    """
    ADD_NODE = "AddNode"
    REMOVE_NODE = "RemoveNode"
    GET_POSITION = "GetPosition"
    WRAP_NODE = "WrapNode"
    WRAP_NODE_ORDERED = "WrapNodeOrdered"
    UNWRAP_NODE = "UnwrapNode"
    IS_NODE_WRAPPED = "IsNodeWrapped"
    SET_WRAPPER_NODE_ACTION_STRING = "SetWrapperNodeActionString"
    ADD_CONNECTION = "AddConnection"
    ADD_CONNECTION_BY_SLOT_ID = "AddConnectionBySlotId"
    ARE_SLOTS_CONNECTED = "AreSlotsConnected"
    REMOVE_CONNECTION = "RemoveConnection"
    EXTEND_SLOT = "ExtendSlot"
    GET_NODE_BY_ID = "GetNodeById"
    GET_NODES_FROM_GRAPH_NODE_IDS = "GetNodesFromGraphNodeIds"
    GET_NODE_IDS_BY_NODE = "GetNodeIdByNode"
    GET_SLOT_ID_BY_SLOT = "GetSlotIdBySlot"
    GET_NODES = "GetNodes"
    GET_SELECTED_NODES = "GetSelectedNodes"
    SET_SELECTED = "SetSelected"
    CLEAR_SELECTION = "ClearSelection"
    ENABLE_NODE = "EnableNode"
    DISABLE_NODE = "DisableNode"
    CENTER_ON_NODES = "CenterOnNodes"
    GET_MAJOR_PITCH = "GetMajorPitch"
