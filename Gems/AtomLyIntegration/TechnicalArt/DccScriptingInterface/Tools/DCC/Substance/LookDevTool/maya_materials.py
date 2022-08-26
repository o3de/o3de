from PySide2 import QtCore
import maya.standalone
maya.standalone.initialize(name='python')
import maya.cmds as mc
mc.loadPlugin("fbxmaya")
import maya.mel as mel
mel.eval('loadPlugin fbxmaya')
from pathlib import Path
import logging
import shutil
import json
import ast
import sys
import os


_LOGGER = logging.getLogger('Tools.DCC.Substance.LookDevTool.maya_materials')


class MayaMaterials(QtCore.QObject):
    def __init__(self, target_files, output_directory, parent=None):
        super(MayaMaterials, self).__init__(parent)

        self.target_files = target_files
        self.output_directory = output_directory
        self.processed_files_dictionary = {}
        self.stingray_file_connections = [
            'TEX_ao_map',
            'TEX_color_map',
            'TEX_metallic_map',
            'TEX_normal_map',
            'TEX_roughness_map',
            'TEX_emissive'
        ]
        self.arnold_file_connections = {
            'metalness':         None,
            'baseColor':         None,
            'specularRoughness': None,
            'emissionColor':     None,
            'normal':            None,
            'alpha':             None,
            'subsurfaceColor':   None
        }
        self.start_conversion()

    def start_conversion(self):
        _LOGGER.info('Maya Materials conversion started...')
        for file_name in self.target_files:
            _LOGGER.info(f'FileName: {file_name}')
            self.processed_files_dictionary[file_name] = {}
            mc.file(file_name, force=True, o=True)
            object_list = self.get_scene_objects()
            for target_object in object_list:
                file_stem = os.path.splitext(os.path.basename(file_name))[0]
                asset_directory = os.path.join(self.output_directory, file_stem, target_object)
                target_material = self.get_mesh_materials(target_object)
                material_info = self.get_material_info(target_material[0], asset_directory)
                self.create_asset_directory(target_object, material_info, asset_directory)
                self.processed_files_dictionary[file_name].update({target_object: material_info})
        json.dump(self.processed_files_dictionary, sys.stdout)

    def create_asset_directory(self, target_object, material_info, asset_directory):
        self.set_export_directory(asset_directory)
        self.transfer_texture_sets(asset_directory, material_info)
        self.export_mesh_object(target_object, asset_directory)

    def get_clean_path(self, target_path):
        return target_path.replace('\\', '/')

    def transfer_texture_sets(self, asset_directory, material_info):
        textures_directory = os.path.join(asset_directory, 'textures')
        for key, values in material_info.items():
            if isinstance(values, dict):
                dst = self.get_clean_path(os.path.join(textures_directory, os.path.basename(values['path'])))
                shutil.copyfile(values['path'], dst)
                values['path'] = dst

    def set_export_directory(self, target_directory):
        textures_directory = os.path.join(target_directory, 'textures')
        if not os.path.exists(target_directory):
            os.makedirs(target_directory)
        if not os.path.exists(textures_directory):
            os.makedirs(textures_directory)
        return target_directory

    def export_mesh_object(self, target_object, target_path):
        mc.move( 0, 0, 0, target_object, absolute=True )
        target_path = os.path.join(target_path, f'{target_object}.fbx')
        target_path = self.get_clean_path(target_path)
        _LOGGER.info(f'TargetObject: {target_object}  TargetExportPath: {target_path}')
        # mel.eval(f'file -typ "FBX export" -es -f"{target_path}";')
        mc.select(target_object)
        mc.file(target_path, force=True, type='FBX export', exportSelected=True)

    def get_material_info(self, target_material: str, asset_directory: str):
        _LOGGER.info(f'AssetDirectory: {asset_directory}')
        material_info = {}
        mc.select(target_material, r=True)
        material_type = self.get_material_type(mc.ls(sl=True, long=True) or [])
        if material_type == 'aiStandardSurface':
            material_info = self.get_arnold_material_info(target_material)
            material_info['type'] = 'aiStandardSurface'
        elif material_type == 'StingrayPBS':
            material_info = self.get_stingray_material_info(target_material)
            material_info['type'] = 'StingrayPBS'
        material_info['location'] = self.get_clean_path(asset_directory)
        return material_info

    # +++++++++++++++++++++++++-->
    # Arnold Texture Handling +--->
    # +++++++++++++++++++++++++-->

    def get_arnold_material_info(self, target_material: str):
        self.arnold_file_connections.clear()
        return self.get_arnold_material_textures(target_material)

    def get_arnold_material_textures(self, target_material: str):
        nodes = mc.listConnections(target_material, source=True, destination=False, skipConversionNodes=True) or {}
        for node in nodes:
            if 'file' not in node.lower():
                node = self.get_file_node(node)

            texture_path = mc.getAttr(f'{node}.fileTextureName')
            if texture_path:
                node_connections = mc.listConnections(node, p=True)
                if node_connections[0] == 'defaultColorMgtGlobals.cmEnabled':
                    texture_slot = self.get_arnold_texture_slot(node_connections)
                else:
                    parent_node = mc.listConnections(node, p=True)[0]
                    texture_slot = parent_node.split('.')[-1]
                self.arnold_file_connections[texture_slot] = {'path': texture_path}
        return self.arnold_file_connections

    def get_arnold_texture_slot(self, node_connections):
        supported_connections = [
            'subsurfaceColor',
            'bumpValue'
        ]
        for connection in node_connections:
            connection = connection.split('.')[-1]
            if connection in supported_connections:
                connection = 'normal' if connection == 'bumpValue' else connection
                return connection

    # +++++++++++++++++++++++++++-->
    # Stingray Texture Handling +--->
    # +++++++++++++++++++++++++++-->

    def get_stingray_material_info(self, target_material: str):
        return self.get_stingray_material_textures(target_material)

    def get_stingray_material_textures(self, target_material: str):
        try:
            material_textures_dict = {}
            nodes = mc.listConnections(target_material, source=True, destination=False, skipConversionNodes=True) or {}
            for node in nodes:
                texture_path = mc.getAttr('{}.fileTextureName'.format(node.split('.')[0]))
                if os.path.splitext(os.path.basename(texture_path))[0] != 'dds':
                    texture_key = self.get_texture_key(node)
                    if texture_key:
                        material_textures_dict[texture_key] = {'path': texture_path}
            return material_textures_dict
        except Exception as e:
            _LOGGER.info(f'***************** Error: {e}')

    def get_texture_key(self, node):
        file_texture_plug = mc.listConnections(node, plugs=1)[0]
        key = file_texture_plug.split('.')[-1]
        if key in self.stingray_file_connections:
            texture_base_type = key.split('_')[1]
            texture_key = 'baseColor' if texture_base_type == 'color' else texture_base_type
            return texture_key
        return None

    # +++++++++++++++++++++++++-->
    # General Utilities +++++++--->
    # +++++++++++++++++++++++++-->

    def get_material_type(self, target_material):
        return mc.nodeType(target_material)

    def get_file_node(self, source_node):
        file_node = mc.listConnections(source_node, type='file', c=True)[1]
        return file_node

    def get_scene_objects(self):
        mesh_objects = mc.ls(type='mesh')
        return mesh_objects

    def get_mesh_materials(self, target_mesh):
        """! Gathers a list of all materials attached to each mesh's shader.

        @param target_mesh The target mesh to pull attached material information from.
        @return list - Unique material values attached to the mesh passed as an argument.
        """
        mc.select(target_mesh, r=True)
        target_nodes = mc.ls(sl=True, dag=True, s=True)
        shading_group = mc.listConnections(target_nodes, type='shadingEngine')
        materials = mc.ls(mc.listConnections(shading_group), materials=1)
        return list(set(materials))

_LOGGER.info(f'ARGV: {sys.argv}')
files = ast.literal_eval(sys.argv[1])
output = sys.argv[-1]
MayaMaterials(files, output)
