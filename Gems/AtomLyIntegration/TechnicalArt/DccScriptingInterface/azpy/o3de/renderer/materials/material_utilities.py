#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! @brief To Do: module docstring here..."""

# standard imports
import logging as _logging
import click
import json
import os
from pathlib import Path
import importlib

# dccsi 3rdparty
from box import Box

# dccsi imports
from DccScriptingInterface.azpy.o3de.utils import o3de_utilities as o3de_helpers
from DccScriptingInterface.azpy.o3de.renderer.materials import o3de_material_generator as exporter
from DccScriptingInterface.azpy.constants import FRMT_LOG_LONG
from DccScriptingInterface.azpy import general_utils as helpers
import DccScriptingInterface.azpy.config_utils

# maya and pyside imports, etc.
from PySide2 import QtWidgets


_LOGGER = _logging.getLogger('DCCsi.azpy.o3de.renderer.materials.material_utilities')


def run_cli():
    _LOGGER.info('Material Conversion called in CLI Mode')


def process_materials(dcc_app: str, operation: str, scope: str, options: list, output_path=None, source=None) -> dict:
    """
    Entry point of the material operations process.

    @param dcc_app Source application where conversion files were created
    @param operation Specifies operation to perform - 'query', 'validate', 'modify', 'convert'
    @param scope What gets converted- 'selected', 'by_name', 'scene', 'directory'
    @param options Export options for object export activated in UI
    @param output_path Specifies location of output (if applicable)
    @param source Provides a slot for passing file location when handling directory and/or scene requests outside of
    the currently open file
    @return process_dictionary Data collected in the conversion
    """
    # Gather material information
    controller = get_dcc_controller(dcc_app)
    process_dictionary = {}
    if scope == 'selected' or scope == 'by_name':
        process_dictionary = process_object_materials(controller, operation, output_path, source)
    elif scope == 'scene':
        if get_source_type(source[0]) == 'file':
            process_dictionary = process_scene_materials(controller, operation, output_path, source)
    elif scope == 'directory':
        if get_source_type(source[0]) == 'directory':
            process_dictionary = process_directory_materials(controller, operation, output_path, source)

    # Export Material
    if operation == 'convert':
        master_paths = get_master_paths(process_dictionary)
        exporter.export_standard_pbr_materials(process_dictionary, output_path, options, master_paths)
    return process_dictionary


def process_object_materials(controller, operation, output_path, source):
    source_file = helpers.get_clean_path(controller.get_current_scene().__str__())
    process_dictionary = Box({source_file: {}})
    target_objects = source if source else controller.get_selected_objects()
    for target_object in target_objects:
        try:
            _LOGGER.info(f'\n\n+++++++++++++++\nTargetObject: {target_object}')
            temp_dict = {}
            target_object = target_object[1:] if target_object.startswith('|') else target_object
            object_export_path, object_materials = controller.prepare_material_export(target_object, output_path)
            _LOGGER.info(f'ExportPath: {object_export_path}  Materials: {object_materials}')
            for target_material in object_materials:
                _LOGGER.info(f'TARGETMATERIAL:::: {target_material}')
                temp_dict[target_material] = controller.get_material_info(target_material)
                temp_dict[target_material].update({'mesh_export_location': object_export_path.as_posix()})
                temp_dict[target_material].update({'parent_hierarchy': controller.get_object_hierarchy(target_object)})
            process_dictionary[source_file].update({target_object: temp_dict})
            _LOGGER.info(f'TempDict: {temp_dict}')
        except Exception as e:
            _LOGGER.info(f'ProcessObjectMaterials Fail [{type(e)}]: {e}')

    _LOGGER.info(f'\n\n\n************************\nProcessDictionary: {process_dictionary}')
    _LOGGER.info('\n************************\n')
    controller.cleanup(target_objects)

    return process_dictionary if process_dictionary else []


def process_scene_materials(controller, operation, output_path, source):
    if source:
        _LOGGER.info('::::::: You will have to launch standalone instance here')
    else:
        source = controller.get_scene_objects()
    process_dictionary = process_object_materials(controller, operation, output_path, source)
    return process_dictionary


def process_directory_materials(controller, operation, output_path, source):
    process_dictionary = None
    return process_dictionary


def get_source_type(source):
    if source:
        if Path(source[0]).is_file():
            return 'file'
        elif Path(source[0]).is_dir():
            return 'directory'
        else:
            return 'object'


def get_master_paths(process_dictionary):
    """! This collects all paths being used for texture assignments in a conversion. With this information, when
    copying assets across to the destination folder, the system will filter duplicate copies to save on disk space,
     also re-pathing all subsequent instances in other materials to this single asset."""
    master_paths = {}
    for scene_file, scene_values in process_dictionary.items():
        for object_name, material_values in scene_values.items():
            for material_name, material_attributes in material_values.items():
                for texture_key, texture_values in material_attributes['textures'].items():
                    if texture_values:
                        if 'path' in texture_values.keys():
                            target_path = texture_values['path']
                            if target_path not in master_paths.keys():
                                master_paths.update({target_path: {'source': scene_file, 'objects': [object_name]}})
                            else:
                                master_paths[target_path]['objects'].append(object_name)
    _LOGGER.info(f'MasterPaths: {master_paths}')
    return master_paths


def get_dcc_controller(target_application: str) -> str:
    """! Because we don't ever know which Python distribution we will be using until a command is fired, this function
    uses importlib to dynamically import needed modules, and will help us to avoid failed imports and errors.

    @param target_application Which DCC application you want to extract materials from
    """
    target = None
    if target_application == 'Maya':
        target = 'azpy.dcc.maya.helpers.maya_materials_conversion'
    elif target_application == 'Blender':
        target = 'azpy.dcc.blender.helpers.blender_materials_conversion'
    return importlib.import_module(target) if target else None


def get_transferable_properties(target_application: str, material_type) -> dict:
    controller = get_dcc_controller(target_application)
    return controller.get_transferable_properties(material_type)


def get_material_templates_location():
    registry_bootstrap_file = Path(os.environ['USERPROFILE']) / '.o3de/Registry/bootstrap.setreg'
    try:
        with open(str(registry_bootstrap_file), 'r') as data:
            contents = json.loads(data.read())
            current_project_path = contents['Amazon']['AzCore']['Bootstrap']['project_path']
            _LOGGER.info(Path(current_project_path) / 'Cache/pc/materials/types')
            return Path(current_project_path) / 'Cache/pc/materials/types'
    except FileNotFoundError:
        return None


def get_all_properties_description(material_type: str):
    """! There are several available all properties material templates. They include: basepbr, enhancedpbr, eye,
    skin, standardmultilayerpbr, and standardpbr. One of these should be passed as the "material_type" class
    argument in order to retrieve the full listing of available values and begin mapping process.
    """
    description_location = get_material_templates_location()
    if description_location:
        for file in [x for x in description_location.glob('**/*') if x.is_file()]:
            if file.name == f'{material_type.lower()}_allproperties.material':
                with open(str(file), 'r') as data:
                    return json.loads(data.read())
    return None


def get_default_material_settings(target_application: str, material_type):
    controller = get_dcc_controller(target_application)
    return controller.get_default_material_settings(material_type)


def export_mesh(dcc_application, target, mesh_export_location, export_options):
    controller = get_dcc_controller(dcc_application)
    if isinstance(target, list):
        controller.export_grouped_meshes(target, mesh_export_location, export_options)
    else:
        controller.export_mesh(target, mesh_export_location, export_options)


if __name__ == '__main__':
    run_cli()
