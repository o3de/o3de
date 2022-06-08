# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! @brief This module contains several common utilities/operations for O3DE elements in the DCCsi. """

##
# @file o3de_utilities.py
#
# @brief This module contains several common utilities/operations for use with O3DE specific elements.
#
# @section O3DE Utilities Description
# Python module containing commonly needed helper functions for O3DE when scripting tools in the DCCsi.
# DCCsi (Digital Content Creation Scripting Interface)
#
# @section O3DE Utilities Notes
# - Comments are Doxygen compatible


import xml.etree.ElementTree as ET
import logging as _logging
from pathlib import Path
from box import Box
# import constants
import json
import sys
import os


module_name = 'SDK.O3DE.o3de_utilities'
_LOGGER = _logging.getLogger(module_name)

# These should go into a DCCsi Tools wide constants file
IMAGE_TYPES = ['.tif', '.tiff', '.png', '.jpg', '.jpeg', '.tga']
DIRECTORY_EXCLUSION_LIST = ['.mayaSwatches', '.Backups']
O3DE_DATA_FILES = ['.mtl', '.assetinfo', '.material']


def walk_directory(target_path: Path) -> dict:
    temp_dictionary = {
        'directoryname': target_path.parent.name,
        'directorypath': target_path.parent,
        'files': scan_directory(target_path.parent)
    }
    temp_dictionary['files']['.fbx'] = target_path
    return temp_dictionary


def walk_directories(target_path: Path):
    """
    Records file and directory structure information for all content within specified base folder for operation

    :target path: The base directory from which to perform the os.walk
    :return:
    """
    directory_audit = {}
    scan_type, base_directory = get_base_directory(target_path)
    base_directory_name = base_directory.name
    exclusion_list = DIRECTORY_EXCLUSION_LIST

    directory_index = 0
    for (root, dirs, files) in os.walk(base_directory, topdown=True):
        temp_dictionary = {}
        root = Path(root)
        directory_name = root.name
        is_object_directory = True if [x for x in root.iterdir() if x.suffix.lower() == '.fbx'] else False
        _LOGGER.info(f'\n_\nScanning Directory: {directory_name} -- Object directory? {is_object_directory}')

        if is_object_directory:
            if directory_name not in exclusion_list:
                temp_dictionary = Box({'directoryname': directory_name, 'directorypath': str(root)})
                files_dictionary = scan_directory(root)

                subdirectories = [x for x in root.iterdir() if x.is_dir()]
                for subdirectory in subdirectories:
                    target_path = subdirectory.name
                    if not [x for x in subdirectory.iterdir() if x.suffix.lower() == '.fbx'] and target_path \
                        not in (exclusion_list + [base_directory_name]):
                        files_dictionary.update(scan_directory(root / subdirectory))
                temp_dictionary['files'] = files_dictionary
            directory_audit[directory_index] = temp_dictionary
            directory_index += 1
    return Box(directory_audit)


def scan_directory(target_path: Path) -> dict:
    """
    Finds all FBX files, which are needed for the materials conversion process

    :param target_path:
    :return:
    """
    files_dictionary = Box({})
    for filename in target_path.iterdir():
        filename = Path(filename)
        extension = filename.suffix
        if extension in (IMAGE_TYPES + O3DE_DATA_FILES + ['.fbx', '.FBX']):
            files_dictionary[extension.lower()] = [target_path / filename] if filename.suffix not in \
                files_dictionary.keys() else files_dictionary[extension] + [target_path / filename]
    return files_dictionary


def get_base_directory(target_path: Path) -> list:
    """
    The "walk" function can be passed both directory paths as well as file paths (for single asset processing). In the
    event that a file path is passed, this will get the file path's containing folder for auditing related files
    :param target_path: The path to check
    :return:
    """
    base_directory = target_path
    scan_type = 'directory'
    if target_path.is_file():
        scan_type = 'file'
        base_directory = Path(target_path.parent)
    return [scan_type, base_directory]


def get_base_texture_name(texture_path: Path, texture_types: dict) -> str:
    path_base = os.path.basename(texture_path)
    base_texture_name = os.path.splitext(path_base)[0]
    if path_base.find('_') != -1:
        base = path_base.split('_')[-1]
        suffix = base.split('.')[0]
        for key, values in texture_types.items():
            for v in values:
                if suffix == v:
                    base_list = path_base.split('_')[:-1]
                    base_texture_name = '_'.join(base_list)
                    return base_texture_name
    return base_texture_name


def get_material_info(target_directory: Path, filename: str) -> Box:
    """
    Extracts material information from legacy .mtl files for conversion to the .material format
    :param target_directory:
    :param filename:
    :return:
    """
    materials_list = {}
    texture_modifications = {}
    target_mtl = target_directory / filename
    if target_mtl.is_file():
        tree = ET.parse(target_mtl)
        root = tree.getroot()
        for material in root.iter('Material'):
            material_attributes = {}
            if 'Name' in material.attrib.keys():
                for key, value in material.attrib.items():
                    material_attributes[key] = value

            material_textures = {}
            for textures in material.findall('Textures'):
                for texture in textures.findall('Texture'):
                    map_name = texture.get('Map')
                    file_path = texture.get('File')
                    material_textures[map_name.lower()] = Path(file_path)
                    if texture.findall('TexMod'):
                        listing = texture.findall('TexMod')
                        for child in listing:
                            texture_modifications[material_attributes['Name']] = child.attrib
            if material_attributes:
                temp_list = Box({
                    'assigned': [],
                    'attributes': material_attributes,
                    'modifications': texture_modifications,
                    'textures': material_textures
                })
                materials_list[material_attributes['Name']] = temp_list
    return Box(materials_list)


def get_asset_info(target_directory: Path, filename: str, target_material: str) -> list:
    """
    Gathers material assignments for specified assets for .assetinfo files that typically accompany FBX files
    :param target_directory: The directory to search for .assetinfo file
    :param filename: The name of the FBX file for which information is being gathered
    :param target_material: The material contained in the FBX file that the script is finding geo assignments for
    :return:
    """
    target_file = target_directory / filename.lower()
    assignment_list = []
    if target_file.is_file():
        try:
            tree = ET.parse(target_file)
            root = tree.getroot()
            for layer_listing in root.iter('Class'):
                target_value = layer_listing.get('value')
                if target_value and target_value.find('.') != -1:
                    search_string = target_value.split('.')[-1]

                    if search_string == target_material:
                        asset_list = target_value.split('.')
                        crop_path = asset_list[2:-1]
                        asset_path = '.'.join(crop_path)
                        if asset_path not in assignment_list:
                            assignment_list.append(asset_path)
        except Exception as e:
            _LOGGER.info('AssetInfo file found, but information extraction failed.')
    else:
        _LOGGER.info('AssetInfo file not found. Cannot gather material assignments.')
    return assignment_list


def get_material_template(shader_type: str) -> dict:
    definitions = os.path.join(os.path.dirname(os.path.abspath(__file__)), '{}.material'.format(shader_type))
    if os.path.exists(definitions):
        with open(definitions) as f:
            return json.load(f)


def export_o3de_material(output: Path, material_description: dict):
    """
    Takes one final dictionary with information gathered in the process and saves with JSON formatting into a
    .material file

    :param output:
    :param material_description:
    :return:
    """
    with open(output, 'w', encoding='utf-8') as material_file:
        json.dump(material_description, material_file, ensure_ascii=False, indent=4)



