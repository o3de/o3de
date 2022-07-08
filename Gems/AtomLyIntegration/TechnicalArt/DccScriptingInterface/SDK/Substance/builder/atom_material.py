# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# -- This line is 75 characters -------------------------------------------
"""Empty Doc String"""  # To Do: add documentation
# -------------------------------------------------------------------------
#  built-ins
import os
import sys
#import simplejson as json
import json

# Lumberyard extensions
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import *

#  3rdparty (we provide)
from box import Box
from pathlib import Path
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# set up global space, logging etc.
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = __name__
if _PACKAGENAME is '__main__':
    _PACKAGENAME = 'DCCsi.SDK.substance.builder.atom_material'

import azpy
_LOGGER = azpy.initialize_logger(_PACKAGENAME)
_LOGGER.debug('Starting up:  {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# early attach WingIDE debugger (can refactor to include other IDEs later)
if _DCCSI_DEV_MODE:
    from azpy.test.entry_test import connect_wing
    foo = connect_wing()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# previous
class AtomPBR:
    def __init__(self, material_file):

        # loading .material File
        self.material_file = material_file
        self.input_data = open(self.material_file, "r")
        self.material = json.load(self.input_data)
        self.mat_box = Box(self.material)
        self.input_data.close()

        # List of texture slots
        self.tex = ['baseColor', 'metallic', 'roughness', 'normalMap', 'opacity']

        # Construct texture maps
        self.basecolor_tex = ""
        self.metallic_tex = ""
        self.roughness_tex = ""
        self.normalmap_tex = ""
        self.opacity_tex = ""

    def load(self, material_file):
        input_data = open(material_file, "r")
        self.material = json.load(input_data)
        self.mat_box = Box(self.material)
        input_data.close()

    def get_map(self, tex_slot):
        return self.mat_box.properties[tex_slot].parameters.textureMap

    def set_map(self, tex_slot, tex_map):
        self.mat_box.properties[tex_slot].parameters.textureMap = tex_map

    def write(self, material_out):
        output_data = open(material_out, "w+")
        output_data.write(json.dumps(self.mat_box, indent=4))
        output_data.close()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# new?
class AtomMaterial:
    def __init__(self, material_file):
        # loading .material File
        self.material_file = material_file
        self.input_data = open(self.material_file, "r")
        self.material = json.load(self.input_data)
        self.mat_box = Box(self.material)
        self.input_data.close()

        # List of texture slots
        # old tex maps
        # self.tex = ['DiffuseMap', 'NormalMap', 'SpecularMap', 'EnvironmentMap']
        self.tex = ['baseColor', 'metallic', 'roughness', 'specularF0', 'normal', 'opacity']

        self.texture_map = {'baseColor': 'baseColor',
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

    def get_material_type(self):
        return self.mat_box.materialType

    # old getMap function
    # def getMap(self, tex_slot):
    #     return self.mat_box.properties.general[tex_slot]

    def get_map(self, tex_slot):
        return self.mat_box.properties[tex_slot].textureMap

    def set_map(self, tex_slot, tex_map):
        self.mat_box.properties[tex_slot].textureMap = tex_map
        self.mat_box.properties[tex_slot].useTexture = True
        self.mat_box.properties[tex_slot].factor = 1.0

    def write(self, material_out):

        if not material_out.parent.exists():
            try:
                material_out.parent.mkdir(mode=0o777, parents=True, exist_ok=True)
                _LOGGER.info('mkdir: {}'.format(material_out.parent))
            except Exception as e:
                _LOGGER.error(e)
                raise(e)
        else:
            _LOGGER.info('exists: {}'.format(material_out.parent))
            
        material_out.touch()
        output_data = open(str(material_out), "w+")
        output_data.write(json.dumps(self.mat_box, indent=4))
        output_data.close()
        return material_out
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == "__main__":
    """Run this file as main"""

    _LOGGER.info("Test Run:: {0}.".format({_PACKAGENAME}))
    _LOGGER.info("{0} :: if __name__ == '__main__':".format(_PACKAGENAME))

    material_path = Path(Path(__file__).parent.parent, 'resources', 'atom')
    # material_01 = AtomPBR("atom_pbr.material", "awesome.material")
    # material_01 = AtomPBR("atom_pbr.material")
    material_01 = AtomMaterial(Path(material_path, "StandardPBR_AllProperties.material"))
    # material_01.load("atom_pbr.material")
    # material_01.map(material_01.tex[2]).textureMap = "materials/substance/amazing_xzzzx.tif"
    # # print(material_01.metallic)
    # material_01.write("awesome.material")
    # print(material_01.tex[2])
    # print(material_01.getMap(material_01.tex[3]))

    # material_01.baesColor_tex = "materials/substance/amazing_bc.tif"
    # material_01.setMap(material_01.tex[0], material_01.baesColor_tex)
    # material_01.write("awesome.material")

    # Test material parser for the new format.
    material_01.baseColor_tex = "Textures/Streaming/streaming99.dds"
    material_01.metallic_tex = "Textures/Streaming/streaming99.dds"
    material_01.set_map(material_01.tex[0], material_01.baseColor_tex)
    material_01.set_map(material_01.tex[1], material_01.metallic_tex)
    material_out = material_01.write(Path(material_path, "atom_variant00.material"))
    _LOGGER.info('materialType is:: {}'.format(material_01.get_material_type()))

    if material_out.exists():
        _LOGGER.info('Wrote material file: {}'.format(material_out))
        
    # remove the logger
    del _LOGGER
# ---- END ---------------------------------------------------------------
