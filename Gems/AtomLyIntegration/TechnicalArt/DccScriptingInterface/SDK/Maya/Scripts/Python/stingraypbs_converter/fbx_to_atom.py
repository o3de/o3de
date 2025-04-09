# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
#
# -- This line is 75 characters -------------------------------------------

import simplejson as json
from box import Box
from atom_mat import AtomMaterial as atomMat

"""
The idea behind this script is to convert FBX to glTF in order to take advantage of glTF format
FBX2glTF CLT:
https://github.com/facebookincubator/FBX2glTF

ToDo:
Add FBX2glTF subprocess to the script. 
"""

atom_material = atomMat("C:\\atom\\dev\\AtomTest\\Editor\\Scripts\\atom\\maya\\StandardPBR_AllProperties.json")

""" All these path could be in configure file or bootstrapped """
fbx_root = 'C:\\atom\\dev\\Gems\\AtomContent\\AtomDemoContent\\Assets\\Objects\\Peccy\\'
rel_root = "C:\\atom\\dev\\Gems\\AtomContent\\AtomDemoContent\\Assets\\"
texture_root = 'Objects\\Peccy\\'

# -------------------------------------------------------------------------
class FBX:
    def __init__(self, fbx_file):
        self.fbx_file = fbx_file
        self.glTF_file = self.fbx_file.replace("fbx", "gltf")
        self.input_data = open(self.glTF_file, "r")
        self.glTF_properties = json.load(self.input_data)
        self.glTF_box = Box(self.glTF_properties)
        self.input_data.close()

    def get_material_names(self):
        return self.glTF_box.materials

    def get_textures(self, index):
        return self.glTF_box.images[index]

    def create_atom_material(self):
        for mat_index in range(len(fbx01.get_material_names())):
            baseColorTexture_index = self.get_material_names()[mat_index].pbrMetallicRoughness.baseColorTexture.index
            atom_material.setMap(atom_material.texture_map['baseColor'], texture_root +
                                 self.get_textures(baseColorTexture_index).uri)
            normalTexture_index = self.get_material_names()[mat_index].normalTexture.index
            atom_material.setMap(atom_material.texture_map['normal'], texture_root +
                                 self.get_textures(normalTexture_index).uri)
            roughnessTexture_index = self.get_material_names()[mat_index].\
                pbrMetallicRoughness.metallicRoughnessTexture.index
            atom_material.setMap(atom_material.texture_map['roughness'], texture_root +
                                 self.get_textures(roughnessTexture_index).uri)

            atom_material.write(rel_root + "\\Materials\\" + fbx01.get_material_names()[mat_index].name + ".material")
# -------------------------------------------------------------------------

if __name__ == "__main__":
    fbx01 = FBX(fbx_root + 'peccy_01.fbx')
    fbx01.create_atom_material()
