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
# This file contains Lumberyard specific file and helper functions for handling material conversion
# -------------------------------------------------------------------------


import xml.etree.ElementTree as ET
import logging as _logging
from pathlib import Path
from box import Box
import constants
import json
import sys
import os


module_name = 'legacy_asset_converter.main.lumberyard_data'
_LOGGER = _logging.getLogger(module_name)


def walk_directories(target_path):
    """
    Records file and directory structure information for all content within specified base folder for operation

    :target path: The base directory from which to perform the os.walk
    :return:
    """
    directory_audit = {}
    scan_type, base_directory = get_base_directory(target_path)
    base_directory_name = base_directory.name
    exclusion_list = constants.DIRECTORY_EXCLUSION_LIST

    directory_index = 0
    for (root, dirs, files) in os.walk(base_directory, topdown=True):
        temp_dictionary = {}
        root = Path(root)
        directory_name = root.name
        is_object_directory = True if [x for x in root.iterdir() if x.suffix == '.fbx'] else False
        _LOGGER.info(f'\n_\nScanning Directory: {directory_name} -- Object directory? {is_object_directory}')

        if is_object_directory:
            if directory_name not in exclusion_list:
                temp_dictionary = Box({'directoryname': directory_name, 'directorypath': str(root)})
                files_dictionary = scan_directory(root)

                subdirectories = [x for x in root.iterdir() if x.is_dir()]
                for subdirectory in subdirectories:
                    target_path = subdirectory.name
                    if not [x for x in subdirectory.iterdir() if x.suffix == '.fbx'] and target_path \
                        not in (exclusion_list +[base_directory_name]):
                        files_dictionary.update(scan_directory(root / subdirectory))
                temp_dictionary['files'] = files_dictionary
            directory_audit[directory_index] = temp_dictionary
            directory_index += 1
    return Box(directory_audit)


def scan_directory(directory_path):
    """
    Finds all FBX files, which are needed for the materials conversion process

    :param directory_path:
    :return:
    """
    files_dictionary = {}
    for filename in directory_path.iterdir():
        filename = Path(filename)
        extension = filename.suffix
        if extension in (constants.IMAGE_TYPES + constants.LUMBERYARD_DATA_FILES + ['.fbx']):
            files_dictionary[extension] = [directory_path / filename] if filename.suffix not in list(files_dictionary) else \
                files_dictionary[extension] + [directory_path / filename]
    return files_dictionary


def get_base_directory(target_path):
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
    return scan_type, base_directory


def get_material_info(target_directory, filename):
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
                    # As Standard PBR shader uses same UV set for all textures,
                    # we only want to carry over changes made to the main texture
                    if map_name.lower() == 'diffuse' and texture.findall('TexMod'):
                        listing = texture.findall('TexMod')
                        for child in listing:
                            texture_modifications[material_attributes['Name']] = child.attrib
            if material_attributes:
                temp_list = Box({'assigned': [], 'attributes': material_attributes, 'modifications': texture_modifications,
                             'textures': material_textures})
                materials_list[material_attributes['Name']] = temp_list
    return Box(materials_list)


def get_asset_info(target_directory, filename, target_material):
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


def export_lumberyard_material(output, material_description):
    """
    Takes one final dictionary with information gathered in the process and saves with JSON formatting into a
    .material file

    :param output:
    :param material_description:
    :return:
    """
    with open(output, 'w', encoding='utf-8') as material_file:
        json.dump(material_description, material_file, ensure_ascii=False, indent=4)

