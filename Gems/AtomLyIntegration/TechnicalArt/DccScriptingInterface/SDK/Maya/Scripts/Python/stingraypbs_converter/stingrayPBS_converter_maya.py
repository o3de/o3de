# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
#
# -- This line is 75 characters -------------------------------------------

import os
import site
from unipath import Path
import maya.cmds as cmds

# -------------------------------------------------------------------------
def returnStubDir(stub, start_path):
    _DIRtoLastFile = None
    '''Take a file name (stub) and returns the directory of the file (stub)'''
    if _DIRtoLastFile is None:
        path = os.path.abspath(start_path)
        while 1:
            path, tail = os.path.split(path)
            if (os.path.isfile(os.path.join(path, stub))):
                break
            if (len(tail) == 0):
                path = ""
                if _DCCSI_GDEBUG:
                    print('~ Debug Message:  I was not able to find the '
                          'path to that file (stub) in a walk-up from currnet path')
                break
        _DIRtoLastFile = path

    return _DIRtoLastFile
# --------------------------------------------------------------------------
# Paths for the quick tests. These could throw a UI or a configuration on it.
_ASSET_PATH = Path(cmds.file(q=True, sn=True)).parent.resolve()
_DEV_ROOT = Path(returnStubDir('engineroot.txt', _ASSET_PATH)).resolve()  # hopefully safe
_REL_ROOT = Path(_DEV_ROOT, 'AtomTest').resolve()
_MAYA_SCRIPTS = Path(_REL_ROOT, 'Editor/Scripts/atom/maya').resolve()

site.addsitedir(_MAYA_SCRIPTS)
from atom_mat import AtomMaterial as atomMAT

_ATOM_MAT_TEMPLATE = atomMAT(Path(_MAYA_SCRIPTS,
                                  'StandardPBR_AllProperties.json').resolve())


import pymel.core as pm

class StingrayPBS(object):
    def __init__(self):
        # Append material from selected object(S)
        nodes = pm.ls(dag=1, o=1, s=1, sl=1)
        shade_eng = pm.listConnections(nodes, type=pm.nt.ShadingEngine)
        material = pm.ls(pm.listConnections(shade_eng), materials=1)
        self.mat = []
        for i in material:
            if i not in self.mat:
                self.mat.append(i)


# Make a StringrayPBS instance
strpbs = StingrayPBS()

# This is ugly but work.
# To to: remove all of duplications.
for i in range(len(strpbs.mat)):
    file_node_baseColor = pm.listConnections(strpbs.mat[i].TEX_color_map)
    if (file_node_baseColor and pm.objectType(file_node_baseColor[0]) == 'file'
            and pm.getAttr(strpbs.mat[i].use_color_map)):
        tex_baseColor = Path(pm.getAttr(file_node_baseColor[0].fileTextureName)).resolve()
        tex_baseColor = _REL_ROOT.rel_path_to(tex_baseColor)
        tex_baseColor = tex_baseColor.replace('\\', '/')
        _ATOM_MAT_TEMPLATE.setMap(_ATOM_MAT_TEMPLATE.texture_map['baseColor'], tex_baseColor)
    # else:
    #     _baseColor = pm.getAttr(strpbs.mat[i].base_color)
    #     _ATOM_MAT_TEMPLATE.setColor(_ATOM_MAT_TEMPLATE.texture_map['baseColor'], _baseColor)
    #     print(strpbs.mat[i])
    #     print(baseColor)

    file_node_normal = pm.listConnections(strpbs.mat[i].TEX_normal_map)
    if (file_node_normal and pm.objectType(file_node_normal[0]) == 'file'
            and pm.getAttr(strpbs.mat[i].use_normal_map)):
        tex_normal = Path(pm.getAttr(file_node_normal[0].fileTextureName)).resolve()
        tex_normal = _REL_ROOT.rel_path_to(tex_normal)
        tex_normal = tex_normal.replace('\\', '/')
        _ATOM_MAT_TEMPLATE.setMap(_ATOM_MAT_TEMPLATE.texture_map['normal'], tex_normal)

    file_node_metallic = pm.listConnections(strpbs.mat[i].TEX_metallic_map)
    if (file_node_metallic and pm.objectType(file_node_metallic[0]) == 'file'
            and pm.getAttr(strpbs.mat[i].use_metallic_map)):
        tex_metallic = Path(pm.getAttr(file_node_metallic[0].fileTextureName)).resolve()
        tex_metallic = _REL_ROOT.rel_path_to(tex_metallic)
        tex_metallic = tex_metallic.replace('\\', '/')
        _ATOM_MAT_TEMPLATE.setMap(_ATOM_MAT_TEMPLATE.texture_map['metallic'], tex_metallic)

    file_node_roughness = pm.listConnections(strpbs.mat[i].TEX_roughness_map)
    if (file_node_roughness and pm.objectType(file_node_roughness[0]) == 'file'
            and pm.getAttr(strpbs.mat[i].use_roughness_map)):
        file_node_roughness = pm.listConnections(strpbs.mat[i].TEX_roughness_map)
        tex_roughness = Path(pm.getAttr(file_node_roughness[0].fileTextureName)).resolve()
        tex_roughness = _REL_ROOT.rel_path_to(tex_roughness)
        tex_roughness = tex_roughness.replace('\\', '/')
        _ATOM_MAT_TEMPLATE.setMap(_ATOM_MAT_TEMPLATE.texture_map['roughness'], tex_roughness)

    file_node_ao = pm.listConnections(strpbs.mat[i].TEX_ao_map)
    if (file_node_ao and pm.objectType(file_node_ao[0]) == 'file'
            and pm.getAttr(strpbs.mat[i].use_ao_map)):
        tex_ao = Path(pm.getAttr(file_node_ao[0].fileTextureName)).resolve()
        tex_ao = _REL_ROOT.rel_path_to(tex_ao)
        tex_ao = tex_ao.replace('\\', '/')
        _ATOM_MAT_TEMPLATE.setMap(_ATOM_MAT_TEMPLATE.texture_map['ambientOcclusion'], tex_ao)

    _ATOM_MAT_TEMPLATE.write(Path(_ASSET_PATH, "{}.material".format(str(strpbs.mat[i]))).resolve())
