# coding:utf-8
# !/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# File Description:
# This file is designed to run in Maya Standalone- it imports existing FBX files and reformats materials as Stingray
# PBS materials, and attempts to attach PBR Metal/Rough texture sets generated for each asset. There is currently a
# bug in Maya that prevents this from being possible, although it has been officially logged with Autodesk so hopefully
# a fix will exist in an updated version
# -------------------------------------------------------------------------


#  maya imports
from PySide2 import QtCore
import maya.cmds as mc
import maya.standalone
maya.standalone.initialize(name='python')
mc.loadPlugin("fbxmaya")
import maya.mel as mel
mel.eval('loadPlugin fbxmaya')

#  built-ins
import logging as _logging
import shelve
import json
import time
import sys
import os

# local tool imports
import constants


module_name = 'legacy_asset_converter.create_maya_files'
_LOGGER = _logging.getLogger(module_name)


class CreateMayaFiles(QtCore.QObject):
    def __init__(self, files_list, base_directory, destination_directory, modify_naming, parent=None):
        super(CreateMayaFiles, self).__init__(parent)
        self.files_list = files_list
        self.base_directory = base_directory
        self.destination_directory = destination_directory
        self.modify_naming = modify_naming
        self.scene_shader_info = None
        self.materials_db = {} #shelve.open('materialsdb', protocol=2)
        self.new_file = True
        self.file_name = None
        self.target_database_listing = None
        self.material_list = {}
        self.transfer_data = {}
        self.create_maya_files()

    def create_maya_files(self):
        """
        The main function of the script- this runs when the standalone session begins, and creates a Maya file
        based on each FBX file the script is passed through the subprocess that launches it. Due to the bug mentioned
        in the description above, a template file ('stingray_helper.ma') is loaded into the file so Maya at least
        knows what a Stingray material is- the root of the bug is that when it loads up it is unaware of the Stingray
        material node as well as its extended attributes. This workaround informs Maya of what a Stingray material is,
        although it is limited to this information only- it still doesn't know what the attributes of the material are
        that are necessary for making texture connections. This function processes each file, makes a note of it, and
        continues to move through the passed list until all FBX files have had companion Maya files created, and then
        returns a receipt of converted files.
        :return:
        """
        _LOGGER.info('Base Directory: {}'.format(self.base_directory))
        _LOGGER.info('Destination Directory: {}'.format(self.destination_directory))
        _LOGGER.info('Files List: {}'.format(self.files_list))

        for count, fbx_file in enumerate(self.files_list):
            target_file_path = os.path.join(self.base_directory, fbx_file)
            target_file_path = target_file_path.replace('\\', '/')
            self.file_name = os.path.basename(fbx_file)
            maya_file_name = os.path.join(self.destination_directory,'{}.ma'.format(os.path.splitext(self.file_name)[0]))
            sys.stdout.write('maya_{}'.format(maya_file_name))
            self.flush_then_wait()
            _LOGGER.info('\n\n++++++++++\n++++++++++>>>>\n {}\n++++++++++>>>>\n++++++++++\n'.format(self.file_name))

            if not os.path.isfile(maya_file_name):
                try:
                    self.reset_transfer_values()
                    mc.file('stingray_helper.ma', o=True, force=True)
                    mc.listAttr('StingrayTemplate')
                    mc.listAttr('StingrayTemplate')
                    mc.file(target_file_path, i=True, type="FBX")
                    self.get_file_information(self.file_name, True)
                    self.replace_materials()
                    mc.delete('stingrayHelper')
                    # mel.eval('cleanUpScene 3')
                    mc.file(rename=maya_file_name)
                    mc.file(save=True, type='mayaAscii', force=True)
                    _LOGGER.info('Maya File Processing Complete... Transferring:')
                    _LOGGER.info('FileName: {}'.format(self.file_name))
                    _LOGGER.info('MayaFileName: {}'.format(maya_file_name))
                    self.transfer_data[self.file_name] = {'maya_file': maya_file_name}
                except Exception as e:
                    _LOGGER.info('Error Processing File: {} -- {}'.format(e, target_file_path))
            else:
                mc.file(maya_file_name, force=True, o=True)
                self.get_file_information(fbx_file, false)
        try:
            return_dictionary = {self.target_database_listing: self.transfer_data}
            json.dump(return_dictionary, sys.stdout)
            #self.materials_db.close()
        except Exception as e:
            _LOGGER.info('Error: {} -- {}'.format(e, self.transfer_data))

    def flush_then_wait(self):
        sys.stdout.flush()
        sys.stderr.flush()
        time.sleep(0.5)

    def get_file_information(self, fbx_file, new_file):
        """
        Draws information relating to the target directory of the shelve database to gather texture assignments
        :param fbx_file:
        :param new_file:
        :return:
        """
        self.new_file = new_file
        for key, values in self.materials_db.items():
            if os.path.normpath(values['directorypath']) == self.base_directory:
                self.target_database_listing = key
                self.material_list = values['fbxfiles'][fbx_file]
                break

    def reset_transfer_values(self):
        """
        Refreshes variables in between each FBX file processed

        :return:
        """
        self.new_file = True
        self.material_list.clear()

    def replace_materials(self):
        """
        Finds the currently assigned Phong and Lambert shaders customary with Legacy material setups in FBX files
        and replaces with Stingray PBS materials.
        :return:
        """
        for key, values in self.material_list['materials'].items():
            _LOGGER.info('\n_\n---------|| Processing material: {}'.format(key))
            if self.new_file:
                try:
                    material_name = str(self.resolve_legacy_material(key))
                    _LOGGER.info('MaterialName: {}'.format(material_name))
                    _LOGGER.info('MaterialInfo: {}'.format(values))
                    if values['assigned']:
                        target_assignments = values['assigned']
                        _LOGGER.info('TargetAssignments: {}'.format(target_assignments))
                        sha, sg = self.get_material(material_name)
                        _LOGGER.info('ShaderInfo: {}, {}'.format(sha, sg))
                        if target_assignments:
                            _LOGGER.info('Target Assignments exist- Assigning Shader...')
                            self.set_material(sha, sg, target_assignments)
                        # Disabled until Autodesk fixes bug
                        # self.set_texture_maps(material_name, values['textures'])
                except Exception as e:
                    _LOGGER.info('Cannot replace materials... Error: {}'.format(e))

    def resolve_legacy_material(self, material_name):
        """
        Deletes the legacy Phong and Lambert materials so it can name newly generated Stingray materials with the
        same name.
        :param material_name: Legacy material name to be re-applied to new material
        :return:
        """
        mc.delete(material_name)
        for listing in constants.MATERIAL_SUFFIX_LIST:
            if material_name.lower().endswith(listing) and self.modify_naming == 'True':
                string_length = len(listing) * -1
                new_material_name = material_name[:string_length]
                self.material_list[new_material_name] = self.material_list[material_name]
                del self.material_list[material_name]
                return new_material_name
        return material_name

    def get_texture_modifications(self, material_name):
        """
        Checks for tiling information drawn from the corresponding mtl file. Because the attributes are currently
        not able to be accessed, this function has been tabled for now

        :param material_name: Target material name
        :return:
        """
        _LOGGER.info('Scanning for texture modifications...')
        for material in self.textures_modifications:
            _LOGGER.info('Modification listing:::: {}'.format(material))

    def get_material(self, material_name):
        """
        Gets material assignments and swaps legacy formatted materials with Stingray PBS materials
        :param material_name: Target material name
        :return:
        """
        _LOGGER.info('Shaders in scene: {}'.format(self.get_materials_in_scene()))
        sha = None
        sg = None
        if material_name in self.get_materials_in_scene():
            _LOGGER.info('Deleting material: {}'.format(material_name))
            mc.delete(material_name)

        _LOGGER.info('Creating Stingray material: {}'.format(material_name))
        try:
            sha = mc.shadingNode('StingrayPBS', asShader=True, name=material_name)
            sg = mc.sets(renderable=True, noSurfaceShader=True, empty=True)
            mc.connectAttr(sha + '.outColor', sg + '.surfaceShader', force=True)
        except Exception as e:
            _LOGGER.info('Shader creation failed: {}'.format(e))
        return sha, sg

    def get_scene_shader_info(self):
        """
        Audits scene to gather all materials present in scene geometry (that is to be converted)
        :return:
        """
        self.scene_shader_info = {}
        scene_geo = mc.ls(v=True, geometry=True)
        for target_mesh in scene_geo:
            try:
                shading_groups = list(set(mc.listConnections(target_mesh, type='shadingEngine')))
                for sg in shading_groups:
                    if sg not in self.scene_shader_info.keys():
                        self.scene_shader_info[sg] = list(set(mc.ls(mc.listConnections(sg), materials=True)))
            except Exception:
                pass

    def get_materials_in_scene(self):
        """
        Audits scene to gather all materials present in the Hypershade, and returns them in a list
        :return:
        """
        material_list = []
        for shading_engine in mc.ls(type='shadingEngine'):
            if mc.sets(shading_engine, q=True):
                for material in mc.ls(mc.listConnections(shading_engine), materials=True):
                    material_list.append(material)
        return material_list

    def set_material(self, sha, sg, assignment_list):
        """
        Assigns specified material to specified mesh
        :param sha: Material name
        :param sg: Shading Group
        :param assignment_list: List of geometry to assign shader to
        :return:
        """
        assignment_list = list(set([x.replace('.', '|') for x in assignment_list]))
        _LOGGER.info('\n_\nSET MATERIAL: {} {{{{{{{{{{{{{{{{{{{{{{'.format(sha))
        _LOGGER.info('Assignment list--> {}'.format(assignment_list))
        for item in assignment_list:
            try:
                mc.sets(item, e=True, forceElement=sg)
            except Exception as e:
                _LOGGER.info('Material assignment failed: {}'.format(e))

    def set_texture_maps(self, material_name, texture_list):
        """
        Plugs texture files into texture slots in the shader for specified material name. Currently due to the
        aforementioned bug this functionality does not work as intended, but it remains for when Autodesk supplies a fix
        :param material_name:
        :param texture_list:
        :return:
        """
        shader_translation_keys = {
            'BaseColor': 'color',
            'Roughness': 'roughness',
            'Metallic': 'metallic',
            'Emissive': 'emissive',
            'Normal': 'normal'
        }

        _LOGGER.info('\n_\nSET_TEXTURE_MAPS {{{{{{{{{{{{{{{{{{{{{{')
        _LOGGER.info('Material--> {}'.format(material_name))
        for texture_type, texture_path in texture_list.items():
            try:
                _LOGGER.info('Texture type: {}   Texture path: {}'.format(texture_type, texture_path))
                file_count = len(mc.ls(type='file')) + 1
                texture_file = 'file{}'.format(file_count)
                mc.shadingNode('file', asTexture=True, name=texture_file)
                mc.setAttr('{}.fileTextureName'.format(texture_file), texture_path, type="string")
                _LOGGER.info('TexturePath: {}'.format(texture_path))
                _LOGGER.info('Attributes: {}'.format(mc.listAttr(material_name)))
                mc.setAttr('{}.use_{}_map'.format(material_name, shader_translation_keys[texture_type]), 1)
                mc.connectAttr('{}.outColor'.format(texture_file),
                               '{}.TEX_{}_map'.format(material_name, shader_translation_keys[texture_type]), force=True)
            except Exception as e:
                _LOGGER.info('Conversion failed: {}'.format(e))

    # Revise to make generic and put in Maya utility class --------------------------------------------------------
    # def get_textures_by_connections(self, material_name):
    #     _LOGGER.info(':::::: Get textures by connections ::::::')
    #     material_files = [x for x in mc.listConnections(material_name, plugs=1, source=1) if x.startswith('file')]
    #     searched_values = []
    #     for material_file in material_files:
    #         try:
    #             texture_path = mc.getAttr('{}.fileTextureName'.format(material_file.split('.')[0]))
    #             texture_path_list = texture_path.split('/')
    #             if 'ShaderFX' not in texture_path_list:
    #                 if texture_path:
    #                     search_string = self.get_base_texture_name(texture_path)
    #                     if search_string not in searched_values:
    #                         _LOGGER.info('TexturePath: {}'.format(texture_path))
    #                         searched_values.append(search_string)
    #                         search_results_list = self.get_texture_set(search_string, material_name)
    #                         if search_results_list:
    #                             self.material_list[material_name] = search_results_list
    #
    #         except mc.MayaAttributeError:
    #             pass

    # def get_texture_set(self, search_string, material_name):
    #     target_key = self.base_directory.split('\\')[-1]
    #     for key, values in self.materials_db.items():
    #         if values['directoryname'] == target_key:
    #             for texture_key, texture_set in values['textures'].items():
    #                 for k, v in texture_set.items():
    #                     texture_base = self.get_base_texture_name(v).lower()
    #                     if texture_base.find('_') != -1:
    #                         texture_base = '_'.join(texture_base.split('_')[:-1])
    #                     if texture_base == search_string:
    #                         _LOGGER.info('MatchFound:::::::::::::::')
    #                         _LOGGER.info('Finding modifications... MaterialName: {}'.format(material_name))
    #                         if material_name in self.textures_modifications.keys():
    #                             texture_set['modifications'] = self.textures_modifications[material_name]
    #                             _LOGGER.info('Texture modifications found: {}'.format(self.textures_modifications[material_name]))
    #                         return texture_set
    #     return None

    # def get_base_texture_name(self, texture_path):
    #     path_base = os.path.basename(texture_path)
    #     base_texture_name = os.path.splitext(path_base)[0]
    #     if path_base.find('_') != -1:
    #         base = path_base.split('_')[-1]
    #         suffix = base.split('.')[0]
    #         naming_key_found = None
    #         for key, values in self.texture_naming_dict.items():
    #             for v in values:
    #                 if suffix == v:
    #                     base_list = path_base.split('_')[:-1]
    #                     base_texture_name = '_'.join(base_list)
    #                     return base_texture_name
    #     return base_texture_name


# ++++++++++++++++++++++++++++++++++++++++++++++++#
# Maya Specific Shader Mapping #
# ++++++++++++++++++++++++++++++++++++++++++++++++#

_LOGGER.info('MAYA STANDALONE FILE FIRING: {}'.format(sys.argv))
file_list = sys.argv[1:-3]
base_directory = sys.argv[-3]
destination_directory = sys.argv[-2]
modify_naming = sys.argv[-1]
CreateMayaFiles(file_list, base_directory, destination_directory, modify_naming)
