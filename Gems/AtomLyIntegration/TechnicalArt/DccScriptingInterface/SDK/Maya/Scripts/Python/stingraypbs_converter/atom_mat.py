# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
#
# -- This line is 75 characters -------------------------------------------

"""
Module Documentation: To Do
"""
# -------------------------------------------------------------------------
#  built-ins
import json
# 3rdParty
from box import Box
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
class AtomMaterial:
    def __init__(self, material_file):
        '''To Do: document'''
        # loading .material Files
        self.material_file = material_file
        self.input_data = open(self.material_file, "r")
        self.material = json.load(self.input_data)
        self.mat_box = Box(self.material)
        self.input_data.close()
        # Texture map dictionary:
        self.texture_map = {'ambientOcclusion': 'ambientOcclusion',
                            'baseColor': 'baseColor',
                            'emissive': 'emissive',
                            'metallic': 'metallic',
                            'roughness': 'roughness',
                            'specularF0': 'specular',
                            'normal': 'normal',
                            'opacity': 'opacity'
                            }

    def load(self, material_file):
        input_data = open(material_file, "r")
        self.material = json.load(input_data)
        self.mat_box = Box(self.material)
        input_data.close()

    def getBaseMaterial(self):
        return self.mat_box.material.baseMaterial


    def getMap(self, tex_slot):
        return self.mat_box.properties[tex_slot].textureMap

    def setMap(self, tex_slot, tex_map):
        self.mat_box.properties[tex_slot].textureMap = tex_map
        self.mat_box.properties[tex_slot].useTexture = True
        self.mat_box.properties[tex_slot].factor = 1.0

    def write(self, material_out):
        output_data = open(material_out, "w+")
        output_data.write(json.dumps(self.mat_box, indent=4))
        output_data.close()
# -------------------------------------------------------------------------
