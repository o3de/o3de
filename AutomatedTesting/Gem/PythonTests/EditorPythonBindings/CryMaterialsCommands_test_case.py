"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# Tests the legacy Python API for CryMaterials while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.legacy.material as material
import azlmbr.math as math

materialName = 'materials/ter_layer_green'
print(f'Starting CryMaterial test case using material {materialName}')


def MaterialPropertyTest(property, value, doReset=True):
    try:
        # get old value and attempt to set new value
        oldValue = material.get_property(materialName, property)
        if oldValue == value:
            print(f'>>> `{property}` already set to {oldValue}')
            return
        material.set_property(materialName, property, value)
    
        # test that the set new value worked
        newValue = material.get_property(materialName, property)
        if oldValue != newValue:
            print(f"{property} updated correctly")
            
        # reset back to old value
        if doReset:
            material.set_property(materialName, property, oldValue)
    except:
        print(f'!!! hit an exception when setting `{property}` to {value}')


color = math.Color()
color.r = 255.0
color.g = 128.0
color.b = 64.0
color.a = 0.0

# Material Settings
# MaterialPropertyTest("Material Settings/Shader", "Geometrybeam") # Disabled, SPEC-3590
# MaterialPropertyTest("Material Settings/Surface Type", "grass") # Disabled, SPEC-3590

# Texture Maps
MaterialPropertyTest("Texture Maps/Diffuse/Tiling/IsTileU", False)
MaterialPropertyTest("Texture Maps/Diffuse/Tiling/IsTileV", False)
MaterialPropertyTest("Texture Maps/Diffuse/Tiling/TileU", 0.42)
MaterialPropertyTest("Texture Maps/Diffuse/Rotator/Type", 'Oscillated Rotation')
MaterialPropertyTest("Texture Maps/Diffuse/Rotator/Amplitude", 42.0)
MaterialPropertyTest("Texture Maps/Diffuse/Oscillator/TypeU", 'Fixed Moving')
MaterialPropertyTest("Texture Maps/Diffuse/Oscillator/AmplitudeU", 42.0)

# Vertex Deformation
MaterialPropertyTest("Vertex Deformation/Type", 'Sin Wave')
MaterialPropertyTest("Vertex Deformation/Wave Length X", 42.0)
MaterialPropertyTest("Vertex Deformation/Type", 'Perlin 3D')
MaterialPropertyTest("Vertex Deformation/Noise Scale", math.Vector3(1.1, 2.2, 3.3))

# Opacity Settings
MaterialPropertyTest("Opacity Settings/Opacity", 42)
MaterialPropertyTest("Opacity Settings/AlphaTest", 2)
MaterialPropertyTest("Opacity Settings/Additive", True)

# Lighting Settings
MaterialPropertyTest("Lighting Settings/Diffuse Color", color)
MaterialPropertyTest("Lighting Settings/Specular Color", color)
MaterialPropertyTest("Lighting Settings/Emissive Intensity", 42.0)
MaterialPropertyTest("Lighting Settings/Emissive Color", color)
MaterialPropertyTest("Lighting Settings/Specular Level", 2.0)

# Advanced
MaterialPropertyTest("Advanced/Allow layer activation", False)
MaterialPropertyTest("Advanced/2 Sided", True)
MaterialPropertyTest("Advanced/No Shadow", True)
MaterialPropertyTest("Advanced/Use Scattering", True)
MaterialPropertyTest("Advanced/Hide After Breaking", True)
MaterialPropertyTest("Advanced/Fog Volume Shading Quality High", True)
MaterialPropertyTest("Advanced/Blend Terrain Color", True)
MaterialPropertyTest("Advanced/Voxel Coverage", 0.42)
# --- MaterialPropertyTest("Advanced/Link to Material", "materials/ter_layer_blue") # Works, but clears on UI refresh
MaterialPropertyTest("Advanced/Propagate Opacity Settings", True)
MaterialPropertyTest("Advanced/Propagate Lighting Settings", True)
MaterialPropertyTest("Advanced/Propagate Advanced Settings", True)
MaterialPropertyTest("Advanced/Propagate Texture Maps", True)
MaterialPropertyTest("Advanced/Propagate Shader Params", True)
MaterialPropertyTest("Advanced/Propagate Shader Generation", True)
MaterialPropertyTest("Advanced/Propagate Vertex Deformation", True)

# Shader parameters vary with each Shader, just testing a couple of them...
# MaterialPropertyTest("Shader Params/Blend Factor", 7.0, False) # Disabled, SPEC-3590
# MaterialPropertyTest("Shader Params/Indirect bounce color", color, False) # Disabled, SPEC-3590

### These values are reset to False when set. Left them here commented for reference.
# MaterialPropertyTest("Shader Generation Params/Dust & Turbulence", True)
# MaterialPropertyTest("Shader Generation Params/Receive Shadows", True)
# MaterialPropertyTest("Shader Generation Params/UV Vignetting", True)

# Vertex Deformation
MaterialPropertyTest("Vertex Deformation/Type", "Sin Wave")
MaterialPropertyTest("Vertex Deformation/Wave Length X", 42.0)
MaterialPropertyTest("Vertex Deformation/Wave X/Level", 42.0)
MaterialPropertyTest("Vertex Deformation/Wave X/Amplitude", 42.0)
MaterialPropertyTest("Vertex Deformation/Wave X/Phase", 42.0)
MaterialPropertyTest("Vertex Deformation/Wave X/Frequency", 42.0)

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
