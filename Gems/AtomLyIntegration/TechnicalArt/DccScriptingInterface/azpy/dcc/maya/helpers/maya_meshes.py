import maya.cmds as mc
from pathlib import Path
import json
import os


# Plugins
mc.loadPlugin("fbxmaya")


def get_scene_objects():
    mesh_dict = {}
    mesh_objects = mc.ls(type='mesh')
    for target_mesh in mesh_objects:
        transform_node = mc.listRelatives(target_mesh, parent=True)[0]
        mesh_attributes = get_mesh_attributes(transform_node)
        mesh_dict[transform_node] = {
            'translation':   mesh_attributes['center'],
            'rotation':   mesh_attributes['rotate'],
            'scale': mesh_attributes['scale'],
            'objectColorRGB': mesh_attributes['objectColorRGB'],
            'visibility': mesh_attributes['visibility']
        }
    return mesh_dict


def get_mesh_attributes(target_mesh):
    target_attributes = ['scale', 'center', 'rotate', 'objectColorRGB', 'visibility']
    mesh_attributes = {}
    for attr in mc.listAttr(target_mesh):
        if attr in target_attributes:
            attr_value = mc.getAttr(f'{target_mesh}.{attr}')
            if isinstance(attr_value, list):
                if isinstance(attr_value, tuple):
                    attr_value = []
                    for count, axis in enumerate(['X', 'Y', 'Z']):
                        attr_value.append(attr_value[0][axis])
            mesh_attributes[attr] = attr_value
    return mesh_attributes


def export_fbx(target_mesh, output_path):
    """
    Exports sub-objects from Kitbash3d assets, groups of assets are contained in a single FBX for delivery.

    :param target_mesh: Mesh name to be exported
    :param output_path: The destination path for the exported FBX object
    :return:
    """
    if not Path(output_path).exists():
        mesh_attributes = get_mesh_attributes(target_mesh)
        print(f'MeshAttributes: {mesh_attributes}')
        mc.move(0, 0, 0, target_mesh, absolute=True)
        # mc.select(target_mesh, hierarchy=True) you might need to handle grouped items.. leave this here until sorted
        mc.select(target_mesh)
        mc.FBXExport('-file', output_path, '-s')
        mc.move(mesh_attributes['center'][0][0], mesh_attributes['center'][0][1], mesh_attributes['center'][0][2])
        mc.select(clear=True)


def export_lumberyard_material(output_path, material_description):
    with open(output_path, 'w') as material_file:
        json.dump(dict(material_description), material_file, ensure_ascii=False, indent=4)
