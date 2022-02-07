# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import site
from unipath import Path
import click
import glob

# -------------------------------------------------------------------------
def returnStubDir(stub):
    _DIRtoLastFile = None
    '''Take a file name (stub) and returns the directory of the file (stub)'''
    if _DIRtoLastFile is None:
        path = os.path.abspath(__file__)
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
_DEV_ROOT = Path(returnStubDir('engineroot.txt')).resolve() #hopefully safe
_REL_ROOT = Path(_DEV_ROOT, 'AtomTest').resolve()
_MAYA_SCRIPTS = Path(_REL_ROOT, 'Editor/Scripts/atom/maya').resolve()

site.addsitedir(_MAYA_SCRIPTS)
from atom_mat import AtomMaterial as atomMAT

_ATOM_MAT_TEMPLATE = atomMAT(Path(_MAYA_SCRIPTS,
                                  'StandardPBR_AllProperties.material').resolve())

_model_asset_dir = Path(_REL_ROOT, 'Objects/Characters/Peccy').resolve()
_atom_mat_path = Path(_REL_ROOT, 'Objects/Characters/Peccy').resolve()

import pymel.core as pm

def converter(_model_asset_dir, _atom_mat_path):
    _maya_file = glob.glob(_model_asset_dir + '*.ma')
    print(_maya_file)
    for maya in range(len(_maya_file)):
        pm.openFile(_maya_file[maya], f=True, prompt=False)
        set_stingray_properties()
        

# This is ugly but work.
# To to: remove all of duplications.
def set_stingray_properties():
    nodes = pm.ls(dag=1, o=1, s=1)
    shade_eng = pm.listConnections(nodes, type=pm.nt.ShadingEngine)
    materials = pm.ls(pm.listConnections(shade_eng), materials=1)
    mat = []
    for i in materials:
        if i not in mat:
            mat.append(i)

    print(mat)
    for j in range(len(mat)):
        file_node_baseColor = pm.listConnections(mat[j].TEX_color_map)
        if (file_node_baseColor and pm.objectType(file_node_baseColor[0]) == 'file'
                and pm.getAttr(mat[j].use_color_map)):
            tex_baseColor = Path(pm.getAttr(file_node_baseColor[0].fileTextureName)).norm().replace(_REL_ROOT, "")
            _ATOM_MAT_TEMPLATE.setMap(_ATOM_MAT_TEMPLATE.texture_map['baseColor'], tex_baseColor)
        # else:
        #     _baseColor = pm.getAttr(mat[j].base_color)
        #     _ATOM_MAT_TEMPLATE.setColor(_ATOM_MAT_TEMPLATE.texture_map['baseColor'], _baseColor)
        #     print(mat[j])
        #     print(baseColor)
    
        file_node_normal = pm.listConnections(mat[j].TEX_normal_map)
        if (file_node_normal and pm.objectType(file_node_normal[0]) == 'file'
                and pm.getAttr(mat[j].use_normal_map)):
            tex_normal = Path(pm.getAttr(file_node_normal[0].fileTextureName)).norm().replace(_REL_ROOT, "")
            _ATOM_MAT_TEMPLATE.setMap(_ATOM_MAT_TEMPLATE.texture_map['normal'], tex_normal)
    
        file_node_metallic = pm.listConnections(mat[j].TEX_metallic_map)
        if (file_node_metallic and pm.objectType(file_node_metallic[0]) == 'file'
                and pm.getAttr(mat[j].use_metallic_map)):
            tex_metallic = Path(pm.getAttr(file_node_metallic[0].fileTextureName)).norm().replace(_REL_ROOT, "")
            _ATOM_MAT_TEMPLATE.setMap(_ATOM_MAT_TEMPLATE.texture_map['metallic'], tex_metallic)
    
        file_node_roughness = pm.listConnections(mat[j].TEX_roughness_map)
        if (file_node_roughness and pm.objectType(file_node_roughness[0]) == 'file'
                and pm.getAttr(mat[j].use_roughness_map)):
            file_node_roughness = pm.listConnections(mat[j].TEX_roughness_map)
            tex_roughness = Path(pm.getAttr(file_node_roughness[0].fileTextureName)).norm().replace(_REL_ROOT, "")
            _ATOM_MAT_TEMPLATE.setMap(_ATOM_MAT_TEMPLATE.texture_map['roughness'], tex_roughness)
    
        file_node_ao = pm.listConnections(mat[j].TEX_ao_map)
        if (file_node_ao and pm.objectType(file_node_ao[0]) == 'file'
                and pm.getAttr(mat[j].use_ao_map)):
            tex_ao = Path(pm.getAttr(file_node_ao[0].fileTextureName)).norm().replace(_REL_ROOT, "")
            _ATOM_MAT_TEMPLATE.setMap(_ATOM_MAT_TEMPLATE.texture_map['ambientOcclusion'], tex_ao)
    
        _ATOM_MAT_TEMPLATE.write(_REL_ROOT + "\\Materials\\" + str(mat[j])+".material")


@click.command()
@click.option('--maya', default=_model_asset_dir, help='Maya file folder')
@click.option('--output', default=_atom_mat_path, help='Atom Material output path')
def stingrayPBS_converter(maya, output):
    click.echo(converter(maya, output))


if __name__ == '__main__':
    stingrayPBS_converter()
