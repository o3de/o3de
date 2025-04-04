#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
"""
This file contains all the code that has to do with creating and instantiate engine templates
"""
import argparse
import logging
import os
import pathlib
import shutil
import stat
import sys
import json
import uuid
import re
from typing import Tuple

from o3de import manifest, register, validation, utils

logger = logging.getLogger('o3de.engine_template')
logging.basicConfig(format=utils.LOG_FORMAT)

binary_file_ext = {
    '.pak',
    '.bin',
    '.png',
    '.tga',
    '.bmp',
    '.dat',
    '.trp',
    '.fbx',
    '.mb',
    '.tif',
    '.tiff',
    '.bmp',
    '.dds',
    '.dds0',
    '.dds1',
    '.dds2',
    '.dds3',
    '.dds4',
    '.dds5',
    '.dds6',
    '.dds7',
    '.dds8',
    '.dds9',
    '.ctc',
    '.bnk',
    '.wav',
    '.dat',
    '.akd',
    '.cgf',
    '.wem',
    '.assetinfo',
    '.animgraph',
    '.motionset'
}

cpp_file_ext = {
    '.cpp',
    '.h',
    '.hpp',
    '.hxx',
    '.inl'
}

expect_license_info_ext = {
    '.cpp',
    '.h',
    '.hpp',
    '.inl',
    '.py',
    '.cmake'
}

restricted_platforms = {
    'Provo',
    'Salem',
    'Jasper',
    'Paris',
    'Xenia',
    'Lancaster'
}

O3DE_LICENSE_TEXT = \
    """'# {BEGIN_LICENSE}
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
# {END_LICENSE}
"""

this_script_parent = pathlib.Path(os.path.dirname(os.path.realpath(__file__)))


def _replace_license_text(source_data: str):
    while '{BEGIN_LICENSE}' in source_data:
        start = source_data.find('{BEGIN_LICENSE}')
        if start != -1:
            line_start = source_data.rfind('\n', 0, start)
            if line_start == -1:
                line_start = 0
            end = source_data.find('{END_LICENSE}')
            if end != -1:
                end = source_data.find('\n', end)
            if end != -1:
                source_data = source_data[:line_start] + source_data[end + 1:]
    return source_data


def _remove_license_text_markers(source_data: str):
    """
    Removes the lines containing the BEGIN_LICENSE and END_LICENSE markers from the license text
    :param source_data: the source data to transform
    :return: data with the lines containing the license markers removed
    """

    # Fine the line with the BEGIN_LICENSE marker on it
    begin_marker = '{BEGIN_LICENSE}'
    end_marker = '{END_LICENSE}'
    license_block_start_index = source_data.find(begin_marker)
    while license_block_start_index != -1:
        # Store last offset of all text before the license block
        pre_license_block_end = source_data.rfind('\n', 0, license_block_start_index)
        if pre_license_block_end == -1:
            pre_license_block_end = 0
        # Find the end of line after the BEGIN_LICENSE block
        next_line = source_data.find('\n', license_block_start_index + len(begin_marker))
        if next_line != -1:
            # Skip past {BEGIN_LICENSE} line +1 for the newline character
            license_block_start_index = next_line + 1
        else:
            # Otherwise Skip pass the {BEGIN_LICENSE} text as the file contains no more newlines
            license_block_start_index = license_block_start_index + len(begin_marker)

        # Find the {END_LICENSE} marker
        license_block_end_index = source_data.find(end_marker, license_block_start_index)
        if license_block_end_index != -1:
            # Locate the lines before and after the line containing {END_LICENSE} block
            prev_line = source_data.rfind('\n', 0, license_block_end_index)
            post_license_block_begin = source_data.find('\n', license_block_end_index + len(end_marker))
            if post_license_block_begin == -1:
                post_license_block_begin = len(source_data)

            # Move the license_block_end_index back to end of the previous line
            # Also protect against the {BEGIN_LICENSE} and {END_LICENSE} block being on the same line
            if prev_line != -1 and prev_line > license_block_start_index:
                license_block_end_index = prev_line

        if license_block_end_index == -1:
            break

        source_data = source_data[:pre_license_block_end] \
            + source_data[license_block_start_index:license_block_end_index] \
            + source_data[post_license_block_begin:]

    return source_data


def _transform(s_data: str,
               replacements: list,
               keep_license_text: bool = False) -> str:
    """
    Internal function called to transform source data into templated data
    :param s_data: the source data to be transformed
    :param replacements: list of transformation pairs A->B
    :param keep_license_text: whether or not you want to keep license text
    :return: the potentially transformed data
    """
    # copy the s_data into t_data, then apply all transformations only on t_data
    t_data = str(s_data)
    for replacement in replacements:
        t_data = t_data.replace(replacement[0], replacement[1])

    # if someone hand edits the template to have ${Random_Uuid} then replace it with a randomly generated uuid
    while '${Random_Uuid}' in t_data:
        t_data = t_data.replace('${Random_Uuid}', str(uuid.uuid4()).upper(), 1)

    if keep_license_text:
        t_data = _remove_license_text_markers(t_data)
    else:
        t_data = _replace_license_text(t_data)
    return t_data


def _transform_copy(source_file: pathlib.Path,
                    destination_file: pathlib.Path,
                    replacements: list,
                    keep_license_text: bool = False) -> None:
    """
    Internal function called to transform and copy a source file into templated destination file
    :param source_file: the source file to be transformed
    :param destination_file: the destination file, this is the transformed file
    :param replacements: list of transformation pairs A->B
    :param keep_license_text: whether or not you want to keep license text
    """
    # if its a known binary type just copy it, else try to transform it.
    # if its an unknown binary type it will throw and we catch copy.
    # if we had no known binary type it would still work, but much slower
    name, ext = os.path.splitext(source_file)
    if ext in binary_file_ext:
        shutil.copy(source_file, destination_file)
    else:
        try:
            # open the file and transform its data
            with open(source_file, 'r') as s:
                s_data = s.read()
                d_data = _transform(s_data, replacements, keep_license_text)

                # if the dst file we are about to write exists already for some reason delete it
                if os.path.isfile(destination_file):
                    os.unlink(destination_file)
                with open(destination_file, 'w') as d:
                    d.write(d_data)
        except Exception as e:
            # usually happens if there is an unknown binary type
            shutil.copy(source_file, destination_file)
            pass


def _execute_template_json(json_data: dict,
                           destination_path: pathlib.Path,
                           template_path: pathlib.Path,
                           replacements: list,
                           keep_license_text: bool = False,
                           keep_restricted_in_instance: bool = False) -> None:
    # create dirs first
    # for each createDirectory entry, transform the folder name
    for create_directory in json_data['createDirectories']:
        # construct the new folder name
        new_dir = destination_path / create_directory['dir']

        # transform the folder name
        new_dir = _transform(new_dir.as_posix(), replacements, keep_license_text)
        new_dir = pathlib.Path(new_dir)

        if not keep_restricted_in_instance and 'Platform' in new_dir.parts:
            try:
                # the name of the Platform should follow the '/Platform/'
                pattern = r'/Platform/(?P<Platform>[^/:*?\"<>|\r\n]+/?)'
                found_platform = re.search(pattern, new_dir.as_posix()).group('Platform')
                found_platform = found_platform.replace('/', '')
                if found_platform in restricted_platforms:
                    continue
            except Exception as e:
                pass

        # create the folder
        os.makedirs(new_dir, exist_ok=True)

    # for each copyFiles entry, _transformCopy the templated source file into a concrete instance file or
    # regular copy if not templated
    for copy_file in json_data['copyFiles']:
        # construct the input file name
        in_file = template_path / 'Template' / copy_file['file']
        # construct the output file name
        out_file = destination_path / copy_file['file']

        # transform the output file name
        out_file = _transform(out_file.as_posix(), replacements, keep_license_text)
        out_file = pathlib.Path(out_file)

        if not keep_restricted_in_instance and 'Platform' in out_file.parts:
            try:
                # the name of the Platform should follow the '/Platform/'
                pattern = r'/Platform/(?P<Platform>[^/:*?\"<>|\r\n]+/?)'
                found_platform = re.search(pattern, out_file.as_posix()).group('Platform')
                found_platform = found_platform.replace('/', '')
                if found_platform in restricted_platforms:
                    continue
            except Exception as e:
                pass

        # if for some reason the output folder for this file was not created above do it now
        os.makedirs(os.path.dirname(out_file), exist_ok=True)

        # if templated _transformCopy the file, if not just copy it
        if copy_file['isTemplated']:
            _transform_copy(in_file, out_file, replacements, keep_license_text)
        else:
            shutil.copy(in_file, out_file)
        # If the copied file is an executable, make sure the executable flag is enabled
        if copy_file.get('isExecutable', False):
            os.chmod(out_file, stat.S_IROTH | stat.S_IRGRP | stat.S_IRUSR | stat.S_IWGRP | stat.S_IXOTH | stat.S_IXGRP | stat.S_IXUSR)


def _execute_restricted_template_json(template_json_data: dict,
                                      json_data: dict,
                                      restricted_platform: str,
                                      destination_name,
                                      destination_path: pathlib.Path,
                                      destination_restricted_path: pathlib.Path,
                                      template_path: pathlib.Path,
                                      template_restricted_path: pathlib.Path,
                                      destination_restricted_platform_relative_path: pathlib.Path,
                                      template_restricted_platform_relative_path: pathlib.Path,
                                      replacements: list,
                                      keep_restricted_in_instance: bool = False,
                                      keep_license_text: bool = False) -> None:

    # if we are not keeping restricted in instance make restricted.json if not present
    if not keep_restricted_in_instance:
        restricted_json = destination_restricted_path / 'restricted.json'
        os.makedirs(os.path.dirname(restricted_json), exist_ok=True)
        if not os.path.isfile(restricted_json):
            with open(restricted_json, 'w') as s:
                restricted_json_data = {}
                restricted_json_data.update({"restricted_name": destination_name})
                s.write(json.dumps(restricted_json_data, indent=4) + '\n')

    ###################################################################################
    # for each createDirectories in the template copy any entries in the json_data that are for this platform
    for create_directory in template_json_data['createDirectories']:
        new_dir = pathlib.Path(create_directory['dir'])
        if not keep_restricted_in_instance and 'Platform' in new_dir.parts:
            try:
                # the name of the Platform should follow the '/Platform/'
                pattern = r'/Platform/(?P<Platform>[^/:*?\"<>|\r\n]+/?)'
                found_platform = re.search(pattern, new_dir.as_posix()).group('Platform')
            except Exception as e:
                pass
            else:
                found_platform = found_platform.replace('/', '')
                if found_platform == restricted_platform:
                    create_dirs = []
                    if 'createDirectories' in json_data.keys():
                        create_dirs = json_data['createDirectories']
                    create_dirs.append(create_directory)
                    json_data.update({'createDirectories': create_dirs})

    # for each copyFiles in the template copy any entries in the json_data that are for this platform
    for copy_file in template_json_data['copyFiles']:
        new_file = pathlib.Path(copy_file['file'])
        if not keep_restricted_in_instance and 'Platform' in new_file.parts:
            try:
                # the name of the Platform should follow the '/Platform/'
                pattern = r'/Platform/(?P<Platform>[^/:*?\"<>|\r\n]+/?)'
                found_platform = re.search(pattern, new_file.as_posix()).group('Platform')
            except Exception as e:
                pass
            else:
                found_platform = found_platform.replace('/', '')
                if found_platform == restricted_platform:
                    copy_files = []
                    if 'copyFiles' in json_data.keys():
                        copy_files = json_data['copyFiles']
                    copy_files.append(copy_file)
                    json_data.update({'copyFiles': copy_files})

    ###################################################################################

    # every entry is saved in its combined location, so if not keep_restricted_in_instance
    # then we need to palify into the restricted folder

    # create dirs first
    # for each createDirectory entry, transform the folder name
    if 'createDirectories' in json_data:
        for create_directory in json_data['createDirectories']:
            # construct the new folder name
            if keep_restricted_in_instance:
                new_dir = destination_path / create_directory['dir']
            else:
                pal_dir = create_directory['dir'].replace(f'Platform/{restricted_platform}','')
                new_dir = destination_restricted_path / restricted_platform / destination_restricted_platform_relative_path / pal_dir

            # transform the folder name
            new_dir = _transform(new_dir.as_posix(), replacements, keep_license_text)

            # create the folder
            os.makedirs(new_dir, exist_ok=True)

    # for each copyFiles entry, _transformCopy the templated source file into a concrete instance file or
    # regular copy if not templated
    if 'copyFiles' in json_data:
        for copy_file in json_data['copyFiles']:
            # construct the input file name
            if template_restricted_path:
                pal_file = copy_file['file'].replace(f'Platform/{restricted_platform}/', '')
                in_file = template_restricted_path / restricted_platform / template_restricted_platform_relative_path / 'Template' / pal_file
            else:
                in_file = template_path / 'Template' / copy_file['file']

            # construct the output file name
            if keep_restricted_in_instance:
                out_file = destination_path / copy_file['file']
            else:
                pal_file = copy_file['file'].replace(f'Platform/{restricted_platform}/', '')
                out_file = destination_restricted_path / restricted_platform / destination_restricted_platform_relative_path / pal_file

            # transform the output file name
            out_file = _transform(out_file.as_posix(), replacements, keep_license_text)

            # if for some reason the output folder for this file was not created above do it now
            os.makedirs(os.path.dirname(out_file), exist_ok=True)

            # if templated _transformCopy the file, if not just copy it
            if copy_file['isTemplated']:
                _transform_copy(in_file, out_file, replacements, keep_license_text)
            else:
                shutil.copy(in_file, out_file)


def _instantiate_template(template_json_data: dict,
                          destination_name: str,
                          template_name: str,
                          destination_path: pathlib.Path,
                          template_path: pathlib.Path,
                          destination_restricted_path: pathlib.Path,
                          template_restricted_path: pathlib.Path,
                          destination_restricted_platform_relative_path: pathlib.Path,
                          template_restricted_platform_relative_path: pathlib.Path,
                          replacements: list,
                          keep_restricted_in_instance: bool = False,
                          keep_license_text: bool = False) -> int:
    """
    Internal function to create a concrete instance from a template

    :param template_json_data: the template json data
    :param destination_name: the name of folder you want to instantiate the template in
    :param template_name: the name of the template
    :param destination_path: the path you want to instantiate the template in
    :param template_path: the path of the template
    :param destination_restricted_path: the path of the restricted destination
    :param template_restricted_path: the path of the restricted template
    :param destination_restricted_platform_relative_path: any path after the Platform of the restricted destination
    :param template_restricted_platform_relative_path: any path after the Platform of the restricted template
    :param replacements: optional list of strings uses to make concrete names out of templated parameters. X->Y pairs
        Ex. ${Name},TestGem,${Player},TestGemPlayer
        This will cause all references to ${Name} be replaced by TestGem, and all ${Player} replaced by 'TestGemPlayer'
    :param keep_restricted_in_instance: whether or not you want to keep the templates restricted files in your instance
     or separate them out into the restricted folder
    :param keep_license_text: whether or not you want to keep the templates license text in your instance.
        template can have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE},
        this controls if you want to keep the license text from the template in the new instance. It is false by default
        because most customers will not want license text in their instances, but we may want to keep them.
    :return: 0 for success or non 0 failure code
    """
    # execute the template json
    # this will filter out any restricted platforms in the template
    _execute_template_json(template_json_data,
                           destination_path,
                           template_path,
                           replacements,
                           keep_license_text,
                           keep_restricted_in_instance)

    # we execute the jason data again if there are any restricted platforms in the main template and
    # execute any restricted platform jsons if separate

    for restricted_platform in restricted_platforms:
        restricted_json_data = {}
        if template_restricted_path:
            template_restricted_platform = template_restricted_path / restricted_platform
            template_restricted_platform_path_rel = template_restricted_platform / template_restricted_platform_relative_path
            platform_json = template_restricted_platform_path_rel / 'template.json'

            if os.path.isfile(platform_json):
                if not validation.valid_o3de_template_json(platform_json):
                    logger.error(f'Template json {platform_json} is invalid.')
                    return 1

                # load the template json
                with open(platform_json, 'r') as s:
                    try:
                        restricted_json_data = json.load(s)
                    except json.JSONDecodeError as e:
                        logger.error(f'Failed to load {platform_json}: ' + str(e))
                        return 1

        # execute for this restricted platform
        _execute_restricted_template_json(template_json_data,
                                          restricted_json_data,
                                          restricted_platform,
                                          destination_name,
                                          destination_path,
                                          destination_restricted_path,
                                          template_path,
                                          template_restricted_path,
                                          destination_restricted_platform_relative_path,
                                          template_restricted_platform_relative_path,
                                          replacements,
                                          keep_restricted_in_instance,
                                          keep_license_text)

    return 0


def create_template(source_path: pathlib.Path,
                    template_path: pathlib.Path,
                    source_name: str = None,
                    source_restricted_path: pathlib.Path = None,
                    source_restricted_name: str = None,
                    template_restricted_path: pathlib.Path = None,
                    template_restricted_name: str = None,
                    source_restricted_platform_relative_path: pathlib.Path = None,
                    template_restricted_platform_relative_path: pathlib.Path = None,
                    keep_restricted_in_template: bool = False,
                    keep_license_text: bool = False,
                    replace: list = None,
                    force: bool = False,
                    no_register: bool = False) -> int:
    """
    Create a template from a source directory using replacement

    :param source_path: The path to the source that you want to make into a template
    :param template_path: the path of the template to create, can be absolute or relative to default templates path
    :param source_name: Name to replace within template folder with ${Name} placeholder
            If not specified, the basename of the source_path parameter is used as the source_name instead
    :param source_restricted_path: path to the source restricted folder
    :param source_restricted_name: name of the source restricted folder
    :param template_restricted_path: path to the templates restricted folder
    :param template_restricted_name: name of the templates restricted folder
    :param source_restricted_platform_relative_path: any path after the platform in the source restricted
    :param template_restricted_platform_relative_path: any path after the platform in the template restricted
    :param replace: optional list of strings uses to make templated parameters out of concrete names. X->Y pairs
     Ex. TestGem,${Name},TestGemPlayer,${Player}
     This will cause all references to 'TestGem' be replaced by ${Name}, and all 'TestGemPlayer' replaced by ${Player}
     Note these replacements are executed in order, so if you have larger matches, do them first, i.e.
     TestGemPlayer,${Player},TestGem,${Name}
     TestGemPlayer will get matched first and become ${Player} and will not become ${Name}Player
    :param keep_restricted_in_template: whether or not you want to keep the templates restricted in your template.
    :param keep_license_text: whether or not you want to keep the templates license text in your instance.
     Templated files can have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE},
     this controls if you want to keep the license text from the template in the new instance. It is false by default
     because most people will not want license text in their instances.
     :param force Overrides existing files even if they exist
     :param no_register: whether or not after completion that the new object is registered
    :return: 0 for success or non 0 failure code
    """

    # if no src path error
    if not source_path:
        logger.error('Src path cannot be empty.')
        return 1
    if not source_path.is_dir():
        logger.error(f'Src path {source_path} is not a folder.')
        return 1
    source_path = source_path.resolve()

    # if not specified, source_name defaults to the last component of the source_path
    if not source_name:
        source_name = os.path.basename(source_path)
    sanitized_source_name = utils.sanitize_identifier_for_cpp(source_name)

    # if no template path, use default_templates_folder path
    if not template_path:
        logger.info(f'Template path empty. Using source name {source_name}')
        template_path = source_name
    # if the template_path is not an absolute path, then it default to relative from the default template folder
    if not template_path.is_absolute():
        default_templates_folder = manifest.get_registered(default_folder='templates')
        template_path = default_templates_folder / source_name
        logger.info(f'Template path empty. Using default templates folder {template_path}')
    if not force and template_path.is_dir() and len(list(template_path.iterdir())):
        logger.error(f'Template path {template_path} already exists; use --force to overwrite existing contents.')
        return 1

    # Make sure the output directory for the template is outside the source path directory
    try:
        template_path.relative_to(source_path)
    except ValueError:
        pass
    else:
        logger.error(
            f'Template output path {template_path} cannot be a subdirectory of the source_path {source_path}\n')
        return 1

    # template name is now the last component of the template_path
    template_name = os.path.basename(template_path)

    # template name cannot be the same as a restricted platform name
    if template_name in restricted_platforms:
        logger.error(f'Template path cannot be a restricted name. {template_name}')
        return 1

    # if the source restricted name was given and no source restricted path, look up the restricted name to fill
    # in the path
    if source_restricted_name and not source_restricted_path:
        source_restricted_path = manifest.get_registered(restricted_name=source_restricted_name)

    # if we have a source restricted path, make sure its a real restricted object
    if source_restricted_path:
        if not source_restricted_path.is_dir():
            logger.error(f'Source restricted path {source_restricted_path} is not a folder.')
            return 1
        restricted_json = source_restricted_path / 'restricted.json'
        if not validation.valid_o3de_restricted_json(restricted_json):
            logger.error(f"Restricted json {restricted_json} is not valid.")
            return 1
        with open(restricted_json, 'r') as s:
            try:
                restricted_json_data = json.load(s)
            except json.JSONDecodeError as e:
                logger.error(f'Failed to load {restricted_json}: ' + str(e))
                return 1
            try:
                source_restricted_name = restricted_json_data['restricted_name']
            except KeyError as e:
                logger.error(f'Failed to read restricted_name from {restricted_json}')
                return 1

    # if the template restricted name was given and no template restricted path, look up the restricted name to fill
    # in the path
    if template_restricted_name and not template_restricted_path:
        template_restricted_path = manifest.get_registered(restricted_name=template_restricted_name)

    # if we dont have a template restricted name then set it to the templates name
    if not template_restricted_name:
        template_restricted_name = template_name

    # if we have a template restricted path, it must either not exist yet or must be a restricted object already
    if template_restricted_path:
        if template_restricted_path.is_dir():
            # see if this is already a restricted path, if it is get the "restricted_name" from the restricted json
            # so we can set "restricted_name" to it for this template
            restricted_json = template_restricted_path / 'restricted.json'
            if not validation.valid_o3de_restricted_json(restricted_json):
                logger.error(f'{restricted_json} is not valid.')
                return 1
            with open(restricted_json, 'r') as s:
                try:
                    restricted_json_data = json.load(s)
                except json.JSONDecodeError as e:
                    logger.error(f'Failed to load {restricted_json}: ' + str(e))
                    return 1
                try:
                    template_restricted_name = restricted_json_data['restricted_name']
                except KeyError as e:
                    logger.error(f'Failed to read restricted_name from {restricted_json}')
                    return 1
        else:
            os.makedirs(template_restricted_path, exist_ok=True)

    # source restricted relative
    if not source_restricted_platform_relative_path:
        source_restricted_platform_relative_path = ''

    # template restricted relative
    if not template_restricted_platform_relative_path:
        template_restricted_platform_relative_path = ''

    logger.info(f'Processing Src: {source_path}')

    # set all user defined replacements first
    replacements = list()
    while replace:
        replace_this = replace.pop(0)
        with_this = replace.pop(0)
        replacements.append((replace_this, with_this))

    os.makedirs(template_path, exist_ok=True)

    # set the src templates name as the ${Name}
    replacements.append((source_name.lower(), '${NameLower}'))
    replacements.append((source_name.upper(), '${NameUpper}'))
    replacements.append((source_name, '${Name}'))
    replacements.append((sanitized_source_name, '${SanitizedCppName}'))
    sanitized_name_index = len(replacements) - 1

    def _is_cpp_file(file_path: pathlib.Path) -> bool:
        """
        Internal helper method to check if a file is a C++ file based
        on its extension, so we can determine if we need to prefer
        the ${SanitizedCppName}
        :param file_path: The input file path
        :return: bool: Whether or not the input file path has a C++ extension
        """
        name, ext = os.path.splitext(file_path)

        return ext.lower() in cpp_file_ext

    def _transform_into_template(s_data: object,
                                 prefer_sanitized_name: bool = False) -> Tuple[bool, str]:
        """
        Internal function to transform any data into templated data
        :param s_data: the input data, this could be file data or file name data
        :param prefer_sanitized_name: Optionally swap the sanitized name with the normal name
                                      This can be necessary when creating the template, the source
                                      name and sanitized source name might be the same, but C++
                                      files will need to prefer the sanitized version, or else
                                      there might be compile errors (e.g. '-' characters in the name)
        :return: bool: whether or not the returned data MAY need to be transformed to instantiate it
                 t_data: potentially transformed data 0 for success or non 0 failure code
        """
        def swap_sanitized_name_and_normal():
            replacements[sanitized_name_index-1], replacements[sanitized_name_index] = \
                replacements[sanitized_name_index], replacements[sanitized_name_index-1]

        # copy the src data to the transformed data, then operate only on transformed data
        t_data = str(s_data)

        # If we need to prefer the sanitized name, then swap it for the normal
        if prefer_sanitized_name:
            swap_sanitized_name_and_normal()

        # run all the replacements
        for replacement in replacements:
            t_data = t_data.replace(replacement[0], replacement[1])

        # Once we are done running the replacements, reset the list if we had modified it
        if prefer_sanitized_name:
            swap_sanitized_name_and_normal()

        if not keep_license_text:
            t_data = _replace_license_text(t_data)

        def find_pattern_and_add_replacement(pattern: str, replace_placeholder: str):
            """
            Searchs for pattern containing Uuid within the file data
            and replaces matched Uuids with with the specified placeholder
            :param pattern: Regular expression pattern to search for Uuid
                            Pattern must contain a symbolic group of (?P<Uuid>...)
                            as documented at https://docs.python.org/3/library/re.html#regular-expression-syntax
            :replace_placeholder: text to replaced matched Uuid symbolic group pattern with
            """
            nonlocal t_data
            match_result_list = re.findall(pattern, t_data)
            if match_result_list:
                for uuid_text in match_result_list:
                    replacements.append((uuid_text, replace_placeholder))
                    t_data = t_data.replace(uuid_text, replace_placeholder)

        # See if this file has the ModuleClassId
        find_pattern_and_add_replacement( \
            r'.*AZ_RTTI\(\$\{SanitizedCppName\}Module,\s*"(?P<Uuid>\{?[A-Fa-f0-9]{8}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{12}\}?)"', \
            '${ModuleClassId}')

        # See if this file has the SysCompClassId
        find_pattern_and_add_replacement( \
            r'.*AZ_COMPONENT\(\$\{SanitizedCppName\}SystemComponent,\s*"(?P<Uuid>\{?[A-Fa-f0-9]{8}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{12}\}?)"', \
            '${SysCompClassId}')

        # See if this file has the EditorSysCompClassId
        find_pattern_and_add_replacement( \
            r'.*AZ_COMPONENT\(\$\{SanitizedCppName\}EditorSystemComponent,\s*"(?P<Uuid>\{?[A-Fa-f0-9]{8}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{12}\}?)"', \
            '${EditorSysCompClassId}')

        # Replace TypeIds.h TypeIds values with placeholders
        # Replace <Uuid> values for EditorSystemComponentTypeId with the ${EditorSysCompClassId} placeholder
        # Users non-capturing group of (?:...) to match a separating '=', '{' or '('
        find_pattern_and_add_replacement( \
            r'.*\$\{SanitizedCppName\}EditorSystemComponentTypeId\s*(?:{|=|\()\s*"(?P<Uuid>\{?[A-Fa-f0-9]{8}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{12}\}?)"', \
            '${EditorSysCompClassId}')

        # Replace the <Uuid> values from a line that matches patterns such as
        # 1. *${SanitizedCppName}SystemComponentTypeId = <Uuid>;
        # 2. *${SanitizedCppName}SystemComponentTypeId{<Uuid>};
        # 3. *${SanitizedCppName}SystemComponentTypeId(<Uuid>);
        # with the ${SysCompClassId} placeholder
        find_pattern_and_add_replacement( \
            r'.*\$\{SanitizedCppName\}SystemComponentTypeId\s*(?:{|=|\()\s*"(?P<Uuid>\{?[A-Fa-f0-9]{8}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{12}\}?)"', \
            '${SysCompClassId}')

        # Replace <Uuid> values for ModuleTypeId with the ${EditorSysCompClassId} placeholder
        find_pattern_and_add_replacement( \
            r'.*\$\{SanitizedCppName\}ModuleTypeId\s*(?:{|=|\()\s*"(?P<Uuid>\{?[A-Fa-f0-9]{8}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{12}\}?)"', \
            '${ModuleClassId}')

        # Replace <Uuid> values for remaining *TypeId variables with the {${Random_Uuid}} placeholder
        find_pattern_and_add_replacement( \
            r'.*TypeId\s*(?:{|=|\()\s*"(?P<Uuid>\{?[A-Fa-f0-9]{8}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{12}\}?)"', \
            '{${Random_Uuid}}')

        # Finally replace any remaining AZ_TYPE_INFO*, AZ_RTTI*, AZ_COMPONENT*, and AZ_EDITOR_COMPONENT*
        # with the {${Random_Uuid}} placehlder
        find_pattern_and_add_replacement( \
            r'.*AZ_TYPE_INFO.*?\(.+,\s*"(?P<Uuid>\{?[A-Fa-f0-9]{8}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{12}\}?)"', \
            '{${Random_Uuid}}')
        find_pattern_and_add_replacement( \
            r'.*AZ_RTTI.*?\(.+,\s*"(?P<Uuid>\{?[A-Fa-f0-9]{8}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{12}\}?)"', \
            '{${Random_Uuid}}')
        find_pattern_and_add_replacement( \
            r'.*AZ_COMPONENT.*?\(.+,\s*"(?P<Uuid>\{?[A-Fa-f0-9]{8}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{12}\}?)"', \
            '{${Random_Uuid}}')
        find_pattern_and_add_replacement( \
            r'.*AZ_EDITOR_COMPONENT.*?\(.+,\s*"(?P<Uuid>\{?[A-Fa-f0-9]{8}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{12}\}?)"', \
            '{${Random_Uuid}}')

        # we want to send back the transformed data and whether or not this file
        # may require transformation when instantiated. So if the input data is not the
        # same as the output, then we transformed it which means there may be a transformation
        # needed to instance it. Or if we chose not to remove the license text then potentially it
        # may need to be transformed when instanced
        if s_data != t_data or '{BEGIN_LICENSE}' in t_data:
            return True, t_data
        else:
            return False, t_data

    def _transform_restricted_into_copyfiles_and_createdirs(root_abs: pathlib.Path,
                                                            path_abs: pathlib.Path = None) -> None:
        """
        Internal function recursively called to transform any paths files into copyfiles and create dirs relative to
        the root. This will transform and copy the files, and save the copyfiles and createdirs data, no not save it
        :param root_abs: This is the path everything will end up relative to
        :path_abs: This is the path being processed, it is always either root_abs (where it starts) or a subdir
         of root_abs
        """
        if not path_abs:
            path_abs = root_abs

        entries = os.listdir(path_abs)
        for entry in entries:

            # create the absolute entry by joining the path_abs and the entry
            entry_abs = path_abs / entry

            # report what file we are processing so we have a good idea if it breaks on what file it broke on
            logger.info(f'Processing file: {entry_abs}')

            # create the relative entry by removing the root_abs
            try:
                entry_rel = entry_abs.relative_to(root_abs)
            except ValueError as err:
                logger.fatal(f'Unable to create relative path: {str(err)}')

            # templatize the entry relative into the destination entry relative
            _, destination_entry_rel = _transform_into_template(entry_rel.as_posix())
            destination_entry_rel = pathlib.Path(destination_entry_rel)

            # clean up any relative leading slashes
            if destination_entry_rel.as_posix().startswith('/'):
                destination_entry_rel = pathlib.Path(destination_entry_rel.as_posix().lstrip('/'))
            if isinstance(destination_entry_rel, pathlib.Path):
                destination_entry_rel = destination_entry_rel.as_posix()

            if template_restricted_path:
                destination_entry_abs = template_restricted_path / restricted_platform / template_restricted_platform_relative_path / 'Template' / destination_entry_rel
                destination_entry_rel = pathlib.Path(destination_entry_rel)
                first = True
                for component in destination_entry_rel.parts:
                    if first:
                        first = False
                        result = pathlib.Path(component) / 'Platform' / restricted_platform
                    else:
                        result = result / component
                destination_entry_rel = result.as_posix()
            else:
                destination_entry_rel = pathlib.Path(destination_entry_rel)
                first = True
                for component in destination_entry_rel.parts:
                    if first:
                        first = False
                        result = pathlib.Path(component) / 'Platform' / restricted_platform
                    else:
                        result = result / component
                destination_entry_rel = result.as_posix()
                destination_entry_abs = template_path / 'Template' / destination_entry_rel

            # the destination folder may or may not exist yet, make sure it does exist before we transform
            # data into it
            os.makedirs(os.path.dirname(destination_entry_abs), exist_ok=True)

            # if the entry is a file, we need to transform it and add the entries into the copyfiles
            # if the entry is a folder then we need to add the entry to the createDirs and recurse into that folder
            templated = False
            if os.path.isfile(entry_abs):

                # if this file is a known binary file, there is no transformation needed and just copy it
                # if not a known binary file open it and try to transform the data.
                # if it is an unknown binary type it will throw and we catch copy
                # if we had no known binary type it would still work, but much slower
                name, ext = os.path.splitext(entry)
                if ext in binary_file_ext:
                    shutil.copy(entry_abs, destination_entry_abs)
                else:
                    try:
                        # open the file and attempt to transform it
                        with open(entry_abs, 'r') as s:
                            source_data = s.read()
                            templated, source_data = _transform_into_template(source_data, _is_cpp_file(entry_abs))

                            # if the file type is a file that we expect to find a license header and we don't find any
                            # warn that the we didn't find the license info, this makes it easy to make sure we didn't
                            # miss any files we want to have license info in.
                            if keep_license_text and ext in expect_license_info_ext:
                                if 'Copyright (c)' not in source_data or '{BEGIN_LICENSE}' not in source_data:
                                    logger.warning(f'Un-templated License header in {entry_abs}')

                        # if the transformed file we are about to write already exists for some reason, delete it
                        if os.path.isfile(destination_entry_abs):
                            os.unlink(destination_entry_abs)
                        with open(destination_entry_abs, 'w') as s:
                            s.write(source_data)
                    except Exception as e:
                        # we were not able to template the file, this is usually due to a unknown binary format
                        # so we catch copy it
                        shutil.copy(entry_abs, destination_entry_abs)
                        pass

                if keep_restricted_in_template:
                    copy_files.append({
                        "file": destination_entry_rel,
                        "isTemplated": templated
                    })
                else:
                    restricted_platform_entries[restricted_platform]['copyFiles'].append({
                        "file": destination_entry_rel,
                        "isTemplated": templated
                    })
            else:
                if keep_restricted_in_template:
                    create_dirs.append({
                        "dir": destination_entry_rel
                    })
                else:
                    restricted_platform_entries[restricted_platform]['createDirs'].append({
                        "dir": destination_entry_rel
                    })
                _transform_restricted_into_copyfiles_and_createdirs(root_abs, entry_abs)

    def _transform_dir_into_copyfiles_and_createdirs(root_abs: pathlib.Path,
                                                     path_abs: pathlib.Path = None) -> None:
        """
        Internal function recursively called to transform any paths files into copyfiles and create dirs relative to
        the root. This will transform and copy the files, and save the copyfiles and createdirs data, no not save it
        :param root_abs: This is the path everything will end up relative to
        :path_abs: This is the path being processed, it is always either root_abs (where it starts) or a subdir
         of root_abs
        """
        if not path_abs:
            path_abs = root_abs

        entries = os.listdir(path_abs)
        for entry in entries:

            # create the absolute entry by joining the path_abs and the entry
            entry_abs = path_abs / entry

            # report what file we are processing so we have a good idea if it breaks on what file it broke on
            logger.info(f'Processing file: {entry_abs}')

            # create the relative entry by removing the root_abs
            try:
                entry_rel = entry_abs.relative_to(root_abs).as_posix()
            except ValueError as err:
                logger.fatal(f'Unable to create relative path: {str(err)}')

            # templatize the entry relative into the origin entry relative
            _, destination_entry_rel = _transform_into_template(entry_rel)
            destination_entry_rel = pathlib.Path(destination_entry_rel)

            # see if the entry is a restricted platform file, if it is then we save its copyfile data in a
            # platform specific list then at the end we can save the restricted ones separately
            found_platform = ''
            platform = False
            if not keep_restricted_in_template and 'Platform' in entry_abs.parts:
                platform = True
                try:
                    # the name of the Platform should follow the '/Platform/'
                    pattern = r'/Platform/(?P<Platform>[^/:*?\"<>|\r\n]+/?)'
                    found_platform = re.search(pattern, entry_abs.as_posix()).group('Platform')
                    found_platform = found_platform.replace('/', '')
                except Exception as e:
                    pass

                # if the entry is a file and the found_platform isn't yet in the restricted_platform_entries
                # then set found_platform to empty as if we didn't find it as we should have already found a folder
                # with the platform name containing this file
                if os.path.isfile(entry_abs) and found_platform not in restricted_platform_entries:
                    found_platform = ''

                # if we found a platform that is not yet in the restricted_platform_entries and it is a restricted
                # platform, then add empty copyfiles and createDirs for this found restricted platform
                if found_platform not in restricted_platform_entries and found_platform in restricted_platforms:
                    restricted_platform_entries.update({found_platform: {'copyFiles': [], 'createDirs': []}})

            # Now if we found a platform and still have a found_platform which is a restricted platform
            # then transform the entry relative name into a dst relative entry name and dst abs entry.
            # if not then create a normal relative and abs dst entry name
            if platform and found_platform in restricted_platforms:
                # if we don't have a template restricted path and we found restricted files... warn and skip
                # the file/dir
                if not template_restricted_path:
                    logger.warning("Restricted platform file found!!! {destination_entry_rel}, {found_platform}")
                    continue

                # run all the replacements
                for replacement in replacements:
                    destination_entry_rel = destination_entry_rel.replace(replacement[0], replacement[1])

                # the name of the Platform should follow the '/Platform/{found_platform}'
                destination_entry_rel = destination_entry_rel.replace(f"Platform/{found_platform}", '')
                destination_entry_rel = destination_entry_rel.lstrip('/')

                # construct the absolute entry from the relative
                if template_restricted_platform_relative_path:
                    destination_entry_abs = template_restricted_path / found_platform / template_restricted_platform_relative_path / template_name / 'Template' / destination_entry_rel
                else:
                    destination_entry_abs = template_restricted_path / found_platform / 'Template' / destination_entry_rel
            else:
                # construct the absolute entry from the relative
                destination_entry_abs = template_path / 'Template' / destination_entry_rel

            # clean up any relative leading slashes
            if isinstance(destination_entry_rel, pathlib.Path):
                destination_entry_rel = destination_entry_rel.as_posix()
            if destination_entry_rel.startswith('/'):
                destination_entry_rel = destination_entry_rel.lstrip('/')

            # make sure the dst folder may or may not exist yet, make sure it does exist before we transform
            # data into it
            os.makedirs(os.path.dirname(destination_entry_abs), exist_ok=True)

            # if the entry is a file, we need to transform it and add the entries into the copyfiles
            # if the entry is a folder then we need to add the entry to the createDirs and recurse into that folder
            templated = False
            if os.path.isfile(entry_abs):

                # if this file is a known binary file, there is no transformation needed and just copy it
                # if not a known binary file open it and try to transform the data. if it is an unknown binary
                # type it will throw and we catch copy
                # if we had no known binary type it would still work, but much slower
                name, ext = os.path.splitext(entry)
                if ext in binary_file_ext:
                    shutil.copy(entry_abs, destination_entry_abs)
                else:
                    try:
                        # open the file and attempt to transform it
                        with open(entry_abs, 'r') as s:
                            source_data = s.read()
                            templated, source_data = _transform_into_template(source_data, _is_cpp_file(entry_abs))

                            # if the file type is a file that we expect to fins license header and we don't find any
                            # warn that the we didn't find the license info, this makes it easy to make sure we didn't
                            # miss any files we want to have license info in.
                            if keep_license_text and ext in expect_license_info_ext:
                                if 'Copyright (c)' not in source_data or '{BEGIN_LICENSE}' not in source_data:
                                    logger.warning(f'Un-templated License header in {entry_abs}')

                        # if the transformed file we are about to write already exists for some reason, delete it
                        if os.path.isfile(destination_entry_abs):
                            os.unlink(destination_entry_abs)
                        with open(destination_entry_abs, 'w') as s:
                            s.write(source_data)
                    except Exception as e:
                        # we were not able to template the file, this is usually due to a unknown binary format
                        # so we catch copy it
                        shutil.copy(entry_abs, destination_entry_abs)
                        pass

                # if the file was for a restricted platform add the entry to the restricted platform, otherwise add it
                # to the non restricted
                if platform and found_platform in restricted_platforms:
                    restricted_platform_entries[found_platform]['copyFiles'].append({
                        "file": destination_entry_rel,
                        "isTemplated": templated
                    })
                else:
                    copy_files.append({
                        "file": destination_entry_rel,
                        "isTemplated": templated
                    })
            else:
                # if the folder was for a restricted platform add the entry to the restricted platform, otherwise add it
                # to the non restricted
                if platform and found_platform in restricted_platforms:
                    restricted_platform_entries[found_platform]['createDirs'].append({
                        "dir": destination_entry_rel
                    })
                else:
                    create_dirs.append({
                        "dir": destination_entry_rel
                    })

                # recurse using the same root and this folder
                _transform_dir_into_copyfiles_and_createdirs(root_abs, entry_abs)

    # when we run the transformation to create copyfiles, createdirs, any we find will go in here
    copy_files = []
    create_dirs = []

    # when we run the transformation any restricted platforms entries we find will go in here
    restricted_platform_entries = {}

    # Every template will have a unrestricted folder which is src_path_abs which MAY have restricted files in it, and
    # each template MAY have a restricted folder which will only have restricted files in them. The process is the
    # same for all of them and the result will be a separation of all restricted files from unrestricted files. We do
    # this by running the transformation first over the src path abs and then on each restricted folder for this
    # template we find. This will effectively combine all sources then separates all the restricted.

    # run the transformation on the src, which may or may not have restricted files
    _transform_dir_into_copyfiles_and_createdirs(source_path)

    # every src may have a matching restricted folder per restricted platform
    # run the transformation on each src restricted folder
    if source_restricted_path:
        for restricted_platform in os.listdir(source_restricted_path):
            restricted_platform_src_path_abs = source_restricted_path / restricted_platform \
                                               / source_restricted_platform_relative_path
            if os.path.isdir(restricted_platform_src_path_abs):
                if restricted_platform not in restricted_platform_entries:
                    restricted_platform_entries.update({restricted_platform: {'copyFiles': [], 'createDirs': []}})
                _transform_restricted_into_copyfiles_and_createdirs(restricted_platform_src_path_abs)

    # sort
    copy_files.sort(key=lambda x: x['file'])
    create_dirs.sort(key=lambda x: x['dir'])

    # now we should have all our copyFiles and createDirs entries, so write out the Template json for the unrestricted
    # platforms all together
    json_data = {}
    json_data.update({'template_name': template_name})
    json_data.update({'origin': f'The primary repo for {template_name} goes here: i.e. http://www.mydomain.com'})
    json_data.update(
        {'license': f'What license {template_name} uses goes here: i.e. https://opensource.org/licenses/MIT'})
    json_data.update({'display_name': template_name})
    json_data.update({'summary': f"A short description of {template_name}."})
    json_data.update({'canonical_tags': []})
    json_data.update({'user_tags': [f"{template_name}"]})
    json_data.update({'icon_path': "preview.png"})
    if not keep_restricted_in_template and template_restricted_path:
        json_data.update({'restricted_name': template_restricted_name})
        if template_restricted_platform_relative_path != '':
            json_data.update({'template_restricted_platform_relative_path': template_restricted_platform_relative_path.as_posix()})
    json_data.update({'copyFiles': copy_files})
    json_data.update({'createDirectories': create_dirs})

    json_name = template_path / source_restricted_platform_relative_path / 'template.json'

    with json_name.open('w') as s:
        s.write(json.dumps(json_data, indent=4) + '\n')

    # copy the default preview.png
    preview_png_src = this_script_parent / 'resources' / 'preview.png'
    preview_png_dst = template_path / 'preview.png'
    if not os.path.isfile(preview_png_dst):
        shutil.copy(preview_png_src, preview_png_dst)

    # if no restricted template path was given and restricted platform files were found
    if not keep_restricted_in_template and not template_restricted_path and len(restricted_platform_entries):
        logger.info(f'Restricted platform files found!!! and no template restricted path was found...')

    if not keep_restricted_in_template and template_restricted_path:
        json_name = template_restricted_path / 'restricted.json'
        if not json_name.is_file():
            json_data = {}
            json_data.update({'restricted_name': template_restricted_name})
            os.makedirs(os.path.dirname(json_name), exist_ok=True)

            with json_name.open('w') as s:
                s.write(json.dumps(json_data, indent=4) + '\n')

        # now write out each restricted platform template json separately
        for restricted_platform in restricted_platform_entries:
            restricted_template_path = template_restricted_path / restricted_platform / template_restricted_platform_relative_path
            # sort
            restricted_platform_entries[restricted_platform]['copyFiles'].sort(key=lambda x: x['file'])
            restricted_platform_entries[restricted_platform]['createDirs'].sort(key=lambda x: x['dir'])

            json_data = {}
            json_data.update({'restricted_name': template_name})
            json_data.update({'template_name': template_name})
            json_data.update(
                {'origin': f'The primary repo for {template_name} goes here: i.e. http://www.mydomain.com'})
            json_data.update(
                {'license': f'What license {template_name} uses goes here: i.e. https://opensource.org/licenses/MIT'})
            json_data.update({'display_name': template_name})
            json_data.update({'summary': f"A short description of {template_name}."})
            json_data.update({'canonical_tags': [f'{restricted_platform}']})
            json_data.update({'user_tags': [f'{template_name}']})
            json_data.update({'copyFiles': restricted_platform_entries[restricted_platform]['copyFiles']})
            json_data.update({'createDirectories': restricted_platform_entries[restricted_platform]['createDirs']})

            json_name = restricted_template_path / 'template.json'
            os.makedirs(os.path.dirname(json_name), exist_ok=True)

            with json_name.open('w') as s:
                s.write(json.dumps(json_data, indent=4) + '\n')

        # Register the restricted
        if not no_register:
            if register.register(restricted_path=template_restricted_path):
                logger.error(f'Failed to register the restricted {template_restricted_path}.')
                return 1

    # Register the template
    return register.register(template_path=template_path) if not no_register else 0


def create_from_template(destination_path: pathlib.Path,
                         template_path: pathlib.Path = None,
                         template_name: str = None,
                         destination_name: str = None,
                         destination_restricted_path: pathlib.Path = None,
                         destination_restricted_name: str = None,
                         template_restricted_path: pathlib.Path = None,
                         template_restricted_name: str = None,
                         destination_restricted_platform_relative_path: pathlib.Path = None,
                         template_restricted_platform_relative_path: pathlib.Path = None,
                         keep_restricted_in_instance: bool = False,
                         keep_license_text: bool = False,
                         replace: list = None,
                         force: bool = False,
                         no_register: bool = False) -> int:
    """
    Generic template instantiation for non o3de object templates. This function makes NO assumptions!
     Assumptions are made only for specializations like create_project or create_gem etc... So this function
     will NOT try to divine intent.
    :param destination_path: the folder you want to instantiate the template into
    :param template_path: the path to the template you want to instance
    :param template_name: the name of the template you want to instance, resolves template_path
    :param destination_name: the name that will be substituted when instantiating the template.
           The placeholders of ${Name} and ${SanitizedCppName} will be replaced with destination name and a sanitized
           version of the destination name that is suitable as a C++ identifier. If not specified, defaults to the
           last path component of the destination_path
    :param destination_restricted_path: path to the projects restricted folder
    :param destination_restricted_name: name of the projects restricted folder, resolves destination_restricted_path
    :param template_restricted_path: path of the templates restricted folder
    :param template_restricted_name: name of the templates restricted folder, resolves template_restricted_path
    :param destination_restricted_platform_relative_path: any path after the platform in the destination restricted
    :param template_restricted_platform_relative_path: any path after the platform in the template restricted
    :param keep_restricted_in_instance: whether or not you want to keep the templates restricted files in your instance
     or separate them out into the restricted folder
    :param keep_license_text: whether or not you want to keep the templates license text in your instance.
        template can have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE},
        this controls if you want to keep the license text from the template in the new instance. It is false by default
        because most customers will not want license text in their instances, but we may want to keep them.
    :param replace: optional list of strings uses to make concrete names out of templated parameters. X->Y pairs
        Ex. ${Name},TestGem,${Player},TestGemPlayer
        This will cause all references to ${Name} be replaced by TestGem, and all ${Player} replaced by 'TestGemPlayer'
    :param force Overrides existing files even if they exist
    :return: 0 for success or non 0 failure code
    """
    if template_name and template_path:
        logger.error(f'Template Name and Template Path provided, these are mutually exclusive.')
        return 1

    if destination_restricted_name and destination_restricted_path:
        logger.error(f'Destination Restricted Name and Destination Restricted Path provided, these are mutually'
                     f' exclusive.')
        return 1

    if template_restricted_name and template_restricted_path:
        logger.error(f'Template Restricted Name and Template Restricted Path provided, these are mutually exclusive.')
        return 1

    # need either the template name or path
    if not template_path and not template_name:
        logger.error(f'Template Name or Template Path must be specified.')
        return 1

    # if replacements are provided we need an even number of
    # replace arguments because they are A->B pairs
    if replace and len(replace) % 2 == 1:
        replacement_pairs = []
        for replace_index in range(0, len(replace), 2):
            if replace_index + 1 < len(replace):
                replacement_pairs.append(f' {replace[replace_index]} -> {replace[replace_index + 1]}')
            else:
                replacement_pairs.append(f' {replace[replace_index]} is missing replacement')
        logger.error("Invalid replacement argument pairs.  "
                     "Verify you have provided replacement match and value pairs "
                     "and your replacement arguments are in single quotes "
                     "e.g. -r '${GemName}' 'NameValue'\n\n"
                     "The current set of replacement pairs are:\n" + 
                     ("\n".join(replacement_pairs)))
        return 1

    if template_name:
        template_path = manifest.get_registered(template_name=template_name)

    if not template_path:
        logger.error(f'Could not find the template path using name {template_name}.\n'
                     f'Has the engine been registered yet. It can be registered via the "o3de.py register --this-engine" command')
        return 1
    if not os.path.isdir(template_path):
        logger.error(f'Could not find the template {template_name}=>{template_path}')
        return 1

    # the template.json should be in the template_path, make sure it is valid
    template_json = template_path / 'template.json'
    if not validation.valid_o3de_template_json(template_json):
        logger.error(f'Template json {template_path} is invalid.')
        return 1

    # read in the template.json
    with open(template_json) as s:
        try:
            template_json_data = json.load(s)
        except KeyError as e:
            logger.error(f'Could not read template json {template_json}: {str(e)}.')
            return 1

    # read template name from the json
    try:
        template_name = template_json_data['template_name']
    except KeyError as e:
        logger.error(f'Could not read "template_name" from template json {template_json}: {str(e)}.')
        return 1

    # if the user has not specified either a restricted name or restricted path
    # see if the template itself specifies a restricted name
    if not template_restricted_name and not template_restricted_path:
        try:
            template_json_restricted_name = template_json_data['restricted_name']
        except KeyError as e:
            # the template json doesn't have a 'restricted_name' element warn and use it
            logger.info(f'The template does not specify a "restricted_name".')
        else:
            template_restricted_name = template_json_restricted_name

    # if no restricted name or path we continue on as if there is no template restricted files.
    if template_restricted_name or template_restricted_path:
        # If the user specified a --template-restricted-name we need to check that against the templates
        # 'restricted_name' if it has one and see if they match. If they match then we don't have a problem.
        # If they don't then we error out. If supplied but not present in the template we warn and use it.
        # If not supplied we set what's in the template. If not supplied and not in the template we continue
        # on as if there is no template restricted files.
        if template_restricted_name:
            # The user specified a --template-restricted-name
            try:
                template_json_restricted_name = template_json_data['restricted_name']
            except KeyError as e:
                # the template json doesn't have a 'restricted_name' element warn and use it
                logger.info(f'The template does not specify a "restricted_name".'
                            f' Using supplied {template_restricted_name}')
            else:
                if template_json_restricted_name != template_restricted_name:
                    logger.error(
                        f'The supplied --template-restricted-name {template_restricted_name} does not match the'
                        f' templates "restricted_name". Either the the --template-restricted-name is incorrect or the'
                        f' templates "restricted_name" is wrong. Note that since this template specifies "restricted_name" as'
                        f' {template_json_restricted_name}, --template-restricted-name need not be supplied.')

            template_restricted_path = manifest.get_registered(restricted_name=template_restricted_name)
        else:
            # The user has supplied the --template-restricted-path, see if that matches the template specifies.
            # If it does then we do not have a problem. If it doesn't match then error out. If not specified
            # in the template then warn and use the --template-restricted-path
            try:
                template_json_restricted_name = template_json_data['restricted_name']
            except KeyError as e:
                # the template json doesn't have a 'restricted_name' element warn and use it
                logger.info(f'The template does not specify a "restricted_name".'
                            f' Using supplied {template_restricted_path}')
            else:
                template_json_restricted_path = manifest.get_registered(
                    restricted_name=template_json_restricted_name)
                if template_json_restricted_path != template_restricted_path:
                    logger.error(
                        f'The supplied --template-restricted-path {template_restricted_path} does not match the'
                        f' templates "restricted_name" {template_restricted_name} => {template_json_restricted_path}.'
                        f' Either the the supplied --template-restricted-path is incorrect or the templates'
                        f' "restricted_name" is wrong. Note that since this template specifies "restricted_name" as'
                        f' {template_json_restricted_name} --template-restricted-path need not be supplied'
                        f' and {template_json_restricted_path} will be used.')
                    return 1

        # check and make sure the restricted exists
        if template_restricted_path and not os.path.isdir(template_restricted_path):
            logger.error(f'Template restricted path {template_restricted_path} does not exist.')
            return 1

        # If the user specified a --template-restricted-platform-relative-path we need to check that against
        # the templates 'restricted_platform_relative_path' and see if they match. If they match we don't have
        # a problem. If they don't match then we error out. If supplied but not present in the template we warn
        # and use --template-restricted-platform-relative-path. If not supplied we set what's in the template.
        # If not supplied and not in the template set empty string.
        if template_restricted_platform_relative_path:
            # The user specified a --template-restricted-platform-relative-path
            try:
                template_json_restricted_platform_relative_path = pathlib.Path(
                    template_json_data['restricted_platform_relative_path'])
            except KeyError as e:
                # the template json doesn't have a 'restricted_platform_relative_path' element warn and use it
                logger.info(f'The template does not specify a "restricted_platform_relative_path".'
                            f' Using {template_restricted_platform_relative_path}')
            else:
                # the template has a 'restricted_platform_relative_path', if it matches we are fine, if not
                # something is wrong with either the --template-restricted-platform-relative or the template is.
                if template_restricted_platform_relative_path != template_json_restricted_platform_relative_path:
                    logger.error(f'The supplied --template-restricted-platform-relative-path'
                                 f' "{template_restricted_platform_relative_path}" does not match the'
                                 f' templates.json  "restricted_platform_relative_path". Either'
                                 f' --template-restricted-platform-relative-path is incorrect or the templates'
                                 f' "restricted_platform_relative_path" is wrong. Note that since this template'
                                 f' specifies "restricted_platform_relative_path" it need not be supplied and'
                                 f' "{template_json_restricted_platform_relative_path}" will be used.')
                    return 1
    else:
        # The user has not supplied --template-restricted-platform-relative-path, try to read it from
        # the template json.
        try:
            template_restricted_platform_relative_path = pathlib.Path(
                template_json_data['restricted_platform_relative_path'])
        except KeyError as e:
            # The template json doesn't have a 'restricted_platform_relative_path' element, set empty string.
            template_restricted_platform_relative_path = ''

    if not template_restricted_platform_relative_path:
        template_restricted_platform_relative_path = ''

    # if no destination_path, error
    if not destination_path:
        logger.error('Destination path cannot be empty.')
        return 1
    if not force and os.path.isdir(destination_path):
        logger.error(f'Destination path {destination_path} already exists; use --force to overwrite existing contents.')
        return 1
    else:
        os.makedirs(destination_path, exist_ok=force)

    if not destination_name:
        # destination name is now the last component of the destination_path
        destination_name = destination_path.name

    # destination name cannot be the same as a restricted platform name
    if destination_name in restricted_platforms:
        logger.error(f'Destination path cannot be a restricted name. {destination_name}')
        return 1

    # destination restricted name
    if destination_restricted_name:
        destination_restricted_path = manifest.get_registered(restricted_name=destination_restricted_name)

    # destination restricted path
    elif destination_restricted_path:
        if not os.path.isabs(destination_restricted_path):
            restricted_default_path = manifest.get_registered(default_folder='restricted')
            new_destination_restricted_path = restricted_default_path / "Templates" / destination_restricted_path
            logger.info(f'{destination_restricted_path} is not a full path, making it relative'
                        f' to default restricted path = {new_destination_restricted_path}')
            destination_restricted_path = new_destination_restricted_path
    else:
        restricted_default_path = manifest.get_registered(default_folder='restricted')
        new_destination_restricted_path = restricted_default_path / "Templates" / destination_name
        logger.info(f'--destination-restricted-path is not specified, using default restricted path'
                    f' / Templates / destination name = {new_destination_restricted_path}')
        destination_restricted_path = new_destination_restricted_path

    # destination restricted relative
    if not destination_restricted_platform_relative_path:
        destination_restricted_platform_relative_path = ''

    # any user supplied replacements
    replacements = list()
    while replace:
        replace_this = replace.pop(0)
        with_this = replace.pop(0)
        replacements.append((replace_this, with_this))

    sanitized_cpp_name = utils.sanitize_identifier_for_cpp(destination_name)
    lowercase_name = destination_name.lower()
    sanitized_lowercase_name = utils.sanitize_identifier_for_cpp(lowercase_name)
    # dst name is Name
    replacements.append(("${Name}", destination_name))
    replacements.append(("${NameUpper}", destination_name.upper()))
    replacements.append(("${NameLower}", lowercase_name))
    replacements.append(("${SanitizedNameLower}", lowercase_name))
    replacements.append(("${SanitizedCppName}", sanitized_cpp_name))

    if _instantiate_template(template_json_data,
                             destination_name,
                             template_name,
                             destination_path,
                             template_path,
                             destination_restricted_path,
                             template_restricted_path,
                             destination_restricted_platform_relative_path,
                             template_restricted_platform_relative_path,
                             replacements,
                             keep_restricted_in_instance,
                             keep_license_text):
        logger.error(f'Instantiation of the template has failed.')
        return 1

    # We created the destination, now do anything extra that a destination requires

    # If we are not keeping the restricted in the destination read the destination restricted this might be a new
    # restricted folder, so make sure the restricted has this destinations name
    # Note there is no linking of non o3de objects to o3de restricted. So this will make no attempt to figure out
    # if this destination was actually an o3de object and try to alter the <type>.json
    if not keep_restricted_in_instance:
        if destination_restricted_path:
            os.makedirs(destination_restricted_path, exist_ok=True)

            # write the restricted_name to the destination restricted.json
            restricted_json = destination_restricted_path / 'restricted.json'
            if not os.path.isfile(restricted_json):
                with open(restricted_json, 'w') as s:
                    restricted_json_data = {}
                    restricted_json_data.update({'restricted_name': destination_name})
                    s.write(json.dumps(restricted_json_data, indent=4) + '\n')

            # Register the restricted
            if not no_register:
                if register.register(restricted_path=destination_restricted_path):
                    logger.error(f'Failed to register the restricted {destination_restricted_path}.')
                    return 1

    logger.warning(f'Instantiation successful. NOTE: This is a generic instantiation of the template. If this'
                   f' was a template of an o3de object like a project, gem, template, etc. then the create-project'
                   f' or create-gem command can be used to register the object type via its project.json or gem.json, etc.'
                   f' Create from template is meant only to instance a template of a non o3de object.')

    return 0


def create_project(project_path: pathlib.Path,
                   project_name: str = None,
                   template_path: pathlib.Path = None,
                   template_name: str = None,
                   project_restricted_path: pathlib.Path = None,
                   project_restricted_name: str = None,
                   template_restricted_path: pathlib.Path = None,
                   template_restricted_name: str = None,
                   project_restricted_platform_relative_path: pathlib.Path = None,
                   template_restricted_platform_relative_path: pathlib.Path = None,
                   keep_restricted_in_project: bool = False,
                   keep_license_text: bool = False,
                   replace: list = None,
                   force: bool = False,
                   no_register: bool = False,
                   system_component_class_id: str = None,
                   editor_system_component_class_id: str = None,
                   module_id: str = None,
                   project_id: str = None,
                   version:str = None) -> int:
    """
    Template instantiation specialization that makes all default assumptions for a Project template instantiation,
     reducing the effort needed in instancing a project
    :param project_path: the project path, can be absolute or relative to default projects path
    :param project_name: the project name, defaults to project_path basename if not provided
    :param template_path: the path to the template you want to instance, can be absolute or relative to default templates path
    :param template_name: the name the registered template you want to instance, defaults to DefaultProject, resolves template_path
    :param project_restricted_path: path to the projects restricted folder, can be absolute or relative to the restricted='projects'
    :param project_restricted_name: name of the registered projects restricted path, resolves project_restricted_path
    :param template_restricted_path: templates restricted path can be absolute or relative to restricted='templates'
    :param template_restricted_name: name of the registered templates restricted path, resolves template_restricted_path
    :param project_restricted_platform_relative_path: any path after the platform to append to the project_restricted_path
    :param template_restricted_platform_relative_path: any path after the platform to append to the template_restricted_path
    :param keep_restricted_in_project: whether or not you want to keep the templates restricted files in your project or
     separate them out into the restricted folder
    :param keep_license_text: whether or not you want to keep the templates license text in your instance.
        template can have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE},
        this controls if you want to keep the license text from the template in the new instance. It is false by default
        because most customers will not want license text in their instances, but we may want to keep them.
    :param replace: optional list of strings uses to make concrete names out of templated parameters. X->Y pairs
        Ex. ${Name},TestGem,${Player},TestGemPlayer
        This will cause all references to ${Name} be replaced by TestGem, and all ${Player} replaced by 'TestGemPlayer'
    :param force Overrides existing files even if they exist
    :param no_register: whether or not after completion that the new object is registered
    :param system_component_class_id: optionally specify a uuid for the system component class, default is random uuid
    :param editor_system_component_class_id: optionally specify a uuid for the editor system component class, default is
     random uuid
    :param module_id: optionally specify a uuid for the module class, default is random uuid
    :param project_id: optionally specify a str for the project id, default is random uuid
    :param version: optionally specify a str for the project version, default is 1.0.0
    :return: 0 for success or non 0 failure code
    """
    if template_name and template_path:
        logger.error(f'Template Name and Template Path provided, these are mutually exclusive.')
        return 1

    if project_restricted_name and project_restricted_path:
        logger.error(f'Project Restricted Name and Project Restricted Path provided, these are mutually exclusive.')
        return 1

    if template_restricted_name and template_restricted_path:
        logger.error(f'Template Restricted Name and Template Restricted Path provided, these are mutually exclusive.')
        return 1

    if not template_path and not template_name:
        template_name = 'DefaultProject'

    if template_name and not template_path:
        template_path = manifest.get_registered(template_name=template_name)

    if not template_path:
        logger.error(f'Could not find the template path using name {template_name}.\n'
                     f'Has the engine been registered yet. It can be registered via the "o3de.py register --this-engine" command')
        return 1
    if not os.path.isdir(template_path):
        logger.error(f'Could not find the template {template_name}=>{template_path}')
        return 1

    # the template.json should be in the template_path, make sure it's there and valid
    template_json = template_path / 'template.json'
    if not validation.valid_o3de_template_json(template_json):
        logger.error(f'Template json {template_path} is not valid.')
        return 1

    # read in the template.json
    with open(template_json) as s:
        try:
            template_json_data = json.load(s)
        except json.JSONDecodeError as e:
            logger.error(f'Could not read template json {template_json}: {str(e)}.')
            return 1

    # read template name from the json
    try:
        template_name = template_json_data['template_name']
    except KeyError as e:
        logger.error(f'Could not read "template_name" from template json {template_json}: {str(e)}.')
        return 1

    # if the user has not specified either a restricted name or restricted path
    # see if the template itself specifies a restricted name
    if not template_restricted_name and not template_restricted_path:
        try:
            template_restricted_name = template_json_data['restricted_name']
        except KeyError as e:
            # the template json doesn't have a 'restricted_name' element warn and use it
            logger.info(f'The template does not specify a "restricted_name".')

    # if no restricted name or path we continue on as if there is no template restricted files.
    if template_restricted_name or template_restricted_path:
        # If the user specified a --template-restricted-name we need to check that against the templates
        # 'restricted_name' if it has one and see if they match. If they match then we don't have a problem.
        # If they don't then we error out. If supplied but not present in the template we warn and use it.
        # If not supplied we set what's in the template. If not supplied and not in the template we continue
        # on as if there is no template restricted files.
        if template_restricted_name and not template_restricted_path:
            # The user specified a --template-restricted-name
            try:
                template_json_restricted_name = template_json_data['restricted_name']
            except KeyError as e:
                # the template json doesn't have a 'restricted_name' element warn and use it
                logger.info(f'The template does not specify a "restricted_name".'
                            f' Using supplied {template_restricted_name}')
            else:
                if template_json_restricted_name != template_restricted_name:
                    logger.error(
                        f'The supplied --template-restricted-name {template_restricted_name} does not match the'
                        f' templates "restricted_name". Either the the --template-restricted-name is incorrect or the'
                        f' templates "restricted_name" is wrong. Note that since this template specifies "restricted_name" as'
                        f' {template_json_restricted_name}, --template-restricted-name need not be supplied.')

            template_restricted_path = manifest.get_registered(restricted_name=template_restricted_name)
        else:
            # The user has supplied the --template-restricted-path, see if that matches the template specifies.
            # If it does then we do not have a problem. If it doesn't match then error out. If not specified
            # in the template then warn and use the --template-restricted-path
            try:
                template_json_restricted_name = template_json_data['restricted_name']
            except KeyError as e:
                # the template json doesn't have a 'restricted_name' element warn and use it
                logger.info(f'The template does not specify a "restricted_name".'
                            f' Using supplied {template_restricted_path}')
            else:
                template_json_restricted_path = manifest.get_registered(
                    restricted_name=template_json_restricted_name)
                if template_json_restricted_path != template_restricted_path:
                    logger.error(
                        f'The supplied --template-restricted-path {template_restricted_path} does not match the'
                        f' templates "restricted_name" {template_restricted_name} => {template_json_restricted_path}.'
                        f' Either the the supplied --template-restricted-path is incorrect or the templates'
                        f' "restricted_name" is wrong. Note that since this template specifies "restricted_name" as'
                        f' {template_json_restricted_name} --template-restricted-path need not be supplied'
                        f' and {template_json_restricted_path} will be used.')
                    return 1

        # check and make sure the restricted exists
        if template_restricted_path and not os.path.isdir(template_restricted_path):
            logger.error(f'Template restricted path {template_restricted_path} does not exist.')
            return 1

        # If the user specified a --template-restricted-platform-relative-path we need to check that against
        # the templates 'restricted_platform_relative_path' and see if they match. If they match we don't have
        # a problem. If they don't match then we error out. If supplied but not present in the template we warn
        # and use --template-restricted-platform-relative-path. If not supplied we set what's in the template.
        # If not supplied and not in the template set empty string.
        if template_restricted_platform_relative_path:
            # The user specified a --template-restricted-platform-relative-path
            try:
                template_json_restricted_platform_relative_path = pathlib.Path(
                    template_json_data['restricted_platform_relative_path'])
            except KeyError as e:
                # the template json doesn't have a 'restricted_platform_relative_path' element warn and use it
                logger.info(f'The template does not specify a "restricted_platform_relative_path".'
                            f' Using {template_restricted_platform_relative_path}')
            else:
                # the template has a 'restricted_platform_relative_path', if it matches we are fine, if not
                # something is wrong with either the --template-restricted-platform-relative or the template is.
                if template_restricted_platform_relative_path != template_json_restricted_platform_relative_path:
                    logger.error(f'The supplied --template-restricted-platform-relative-path'
                                 f' "{template_restricted_platform_relative_path}" does not match the'
                                 f' templates.json  "restricted_platform_relative_path". Either'
                                 f' --template-restricted-platform-relative-path is incorrect or the templates'
                                 f' "restricted_platform_relative_path" is wrong. Note that since this template'
                                 f' specifies "restricted_platform_relative_path" it need not be supplied and'
                                 f' "{template_json_restricted_platform_relative_path}" will be used.')
                    return 1
        else:
            # The user has not supplied --template-restricted-platform-relative-path, try to read it from
            # the template json.
            try:
                template_restricted_platform_relative_path = pathlib.Path(
                    template_json_data['restricted_platform_relative_path'])
            except KeyError as e:
                # The template json doesn't have a 'restricted_platform_relative_path' element, set empty string.
                template_restricted_platform_relative_path = ''
    if not template_restricted_platform_relative_path:
        template_restricted_platform_relative_path = ''

    # if no project path, error
    if not project_path:
        logger.error('Project path cannot be empty.')
        return 1
    project_path = project_path.resolve()
    if not os.path.isabs(project_path):
        default_projects_folder = manifest.get_registered(default_folder='projects')
        new_project_path = default_projects_folder / project_path
        logger.info(f'Project Path {project_path} is not a full path, we must assume its relative'
                    f' to default projects path = {new_project_path}')
        project_path = new_project_path
    if not force and project_path.is_dir() and len(list(project_path.iterdir())):
        logger.error(f'Project path {project_path} already exists and is not empty.')
        return 1
    elif not os.path.isdir(project_path):
        os.makedirs(project_path, exist_ok=force)

    if not project_name:
        # project name is now the last component of the project_path
        project_name = os.path.basename(project_path)

    if not utils.validate_identifier(project_name):
        logger.error(
            f'Project name must be fewer than 64 characters, contain only alphanumeric, "_" or "-" characters, and start with a letter.  {project_name}')
        return 1

    # project name cannot be the same as a restricted platform name
    if project_name in restricted_platforms:
        logger.error(f'Project name cannot be a restricted name. {project_name}')
        return 1

    # the generic launcher (and the engine, often) are referred to as o3de, so prevent the user from 
    # accidentally creating a confusing error situation.
    if project_name.lower() == 'o3de':
        logger.error(f"Project name cannot be 'o3de' as this is reserved for the generic launcher.")
        return 1

    # project restricted name
    if project_restricted_name and not project_restricted_path:
        gem_restricted_path = manifest.get_registered(restricted_name=project_restricted_name)
        if not gem_restricted_path:
            logger.error(f'Project Restricted Name {project_restricted_name} cannot be found.')
            return 1

    # project restricted path
    if project_restricted_path:
        if not os.path.isabs(project_restricted_path):
            logger.error(f'Project Restricted Path {project_restricted_path} is not an absolute path.')
            return 1
    # neither put it in the default restricted projects
    else:
        project_restricted_path = manifest.get_o3de_restricted_folder() / 'Projects' / project_name

    # project restricted relative path
    if not project_restricted_platform_relative_path:
        project_restricted_platform_relative_path = ''

    # any user supplied replacements
    replacements = list()
    while replace:
        replace_this = replace.pop(0)
        with_this = replace.pop(0)
        replacements.append((replace_this, with_this))

    sanitized_cpp_name = utils.sanitize_identifier_for_cpp(project_name)
    # project name
    replacements.append(("${Name}", project_name))
    replacements.append(("${NameUpper}", project_name.upper()))
    replacements.append(("${NameLower}", project_name.lower()))
    replacements.append(("${SanitizedCppName}", sanitized_cpp_name))
    replacements.append(("${Version}", version if version else "1.0.0"))
    replacements.append(("${ProjectPath}", project_path.as_posix()))
    replacements.append(("${EnginePath}", manifest.get_this_engine_path().as_posix()))

    # was a project id specified
    if project_id:
        replacements.append(("${ProjectId}", project_id))
    else:
        replacements.append(("${ProjectId}", '{' + str(uuid.uuid4()).upper() + '}'))

    # module id is a uuid with { and -
    if module_id:
        replacements.append(("${ModuleClassId}", module_id))
    else:
        replacements.append(("${ModuleClassId}", '{' + str(uuid.uuid4()).upper() + '}'))

    # system component class id is a uuid with { and -
    if system_component_class_id:
        if '{' not in system_component_class_id or '-' not in system_component_class_id:
            logger.error(
                f'System component class id {system_component_class_id} is malformed. Should look like Ex.' +
                '{b60c92eb-3139-454b-a917-a9d3c5819594}')
            return 1
        replacements.append(("${SysCompClassId}", system_component_class_id))
    else:
        replacements.append(("${SysCompClassId}", '{' + str(uuid.uuid4()).upper() + '}'))

    # editor system component class id is a uuid with { and -
    if editor_system_component_class_id:
        if '{' not in editor_system_component_class_id or '-' not in editor_system_component_class_id:
            logger.error(
                f'Editor System component class id {editor_system_component_class_id} is malformed. Should look like'
                f' Ex.' + '{b60c92eb-3139-454b-a917-a9d3c5819594}')
            return 1
        replacements.append(("${EditorSysCompClassId}", editor_system_component_class_id))
    else:
        replacements.append(("${EditorSysCompClassId}", '{' + str(uuid.uuid4()).upper() + '}'))

    if _instantiate_template(template_json_data,
                             project_name,
                             template_name,
                             project_path,
                             template_path,
                             project_restricted_path,
                             template_restricted_path,
                             project_restricted_platform_relative_path,
                             template_restricted_platform_relative_path,
                             replacements,
                             keep_restricted_in_project,
                             keep_license_text):
        logger.error(f'Instantiation of the template has failed.')
        return 1

    # We created the project, now do anything extra that a project requires
    project_json = project_path / 'project.json'

    # If we are not keeping the restricted in the project read the name of the restricted folder from the
    # restricted json and set that as this projects restricted
    if not keep_restricted_in_project:
        if project_restricted_path:
            os.makedirs(project_restricted_path, exist_ok=True)

            # read the restricted_name from the projects restricted.json
            restricted_json = project_restricted_path / 'restricted.json'
            if os.path.isfile(restricted_json):
                if not validation.valid_o3de_restricted_json(restricted_json):
                    logger.error(f'Restricted json {restricted_json} is not valid.')
                    return 1
            else:
                with open(restricted_json, 'w') as s:
                    restricted_json_data = {}
                    restricted_json_data.update({'restricted_name': project_name})
                    s.write(json.dumps(restricted_json_data, indent=4) + '\n')

            with open(restricted_json, 'r') as s:
                try:
                    restricted_json_data = json.load(s)
                except json.JSONDecodeError as e:
                    logger.error(f'Failed to load restricted json {restricted_json}.')
                    return 1

            try:
                restricted_name = restricted_json_data["restricted_name"]
            except KeyError as e:
                logger.error(f'Failed to read "restricted_name" from restricted json {restricted_json}.')
                return 1

            # set the "restricted": <restricted_name> element of the project.json
            project_json = project_path / 'project.json'
            if not validation.valid_o3de_project_json(project_json):
                logger.error(f'Project json {project_json} is not valid.')
                return 1

            with open(project_json, 'r') as s:
                try:
                    project_json_data = json.load(s)
                except json.JSONDecodeError as e:
                    logger.error(f'Failed to load project json {project_json}.')
                    return 1

            project_json_data.update({"restricted": restricted_name})
            os.unlink(project_json)
            with open(project_json, 'w') as s:
                try:
                    s.write(json.dumps(project_json_data, indent=4) + '\n')
                except OSError as e:
                    logger.error(f'Failed to write project json {project_json}.')
                    return 1

            # Register the restricted
            if not no_register:
                if register.register(restricted_path=project_restricted_path):
                    logger.error(f'Failed to register the restricted {project_restricted_path}.')
                    return 1

    print(f'Project created at {project_path}')

    # Register the project with the either o3de_manifest.json or engine.json
    # and set the project.json "engine" field to match the
    # engine.json "engine_name" field
    return register.register(project_path=project_path) if not no_register else 0


def create_gem(gem_path: pathlib.Path,
               template_path: pathlib.Path = None,
               template_name: str = None,
               gem_name: str = None,
               display_name: str = None,
               summary: str = None,
               requirements: str = None,
               license: str = None,
               license_url: str = None,
               origin: str = None,
               origin_url: str = None,
               user_tags: list or str = None,
               platforms: list or str = None,
               icon_path: str = None,
               documentation_url: str = None,
               repo_uri: str = None,
               gem_restricted_path: pathlib.Path = None,
               gem_restricted_name: str = None,
               template_restricted_path: pathlib.Path = None,
               template_restricted_name: str = None,
               gem_restricted_platform_relative_path: pathlib.Path = None,
               template_restricted_platform_relative_path: pathlib.Path = None,
               keep_restricted_in_gem: bool = False,
               keep_license_text: bool = False,
               replace: list = None,
               force: bool = False,
               no_register: bool = False,
               system_component_class_id: str = None,
               editor_system_component_class_id: str = None,
               module_id: str = None,
               version: str = None) -> int:
    """
    Template instantiation specialization that makes all default assumptions for a Gem template instantiation,
     reducing the effort needed in instancing a gem
    :param gem_path: the gem path, can be absolute or relative to default gems path
    :param template_path: the template path you want to instance, can be absolute or relative to default templates path
    :param template_name: the name of the registered template you want to instance, defaults to DefaultGem, resolves template_path
    :param gem_name: the name that will be substituted when instantiating the template.
       The placeholders of ${Name} and ${SanitizedCppName} will be replaced with gem name and a sanitized
       version of the gem name that is suitable as a C++ identifier. If not specified, defaults to the
       last path component of the gem_path
    :param display_name: The colloquial name displayed throughout Project Manager, may contain spaces and special characters
    :param summary: A short description of your Gem
    :param requirements: Notice of any requirements for your Gem i.e. This requires X other gem
    :param license: License used by your Gem i.e. Apache-2.0 or MIT
    :param license_url: Link to the license web site i.e. https://opensource.org/licenses/Apache-2.0
    :param origin: The name of the originator i.e. XYZ Inc.
    :param origin_url: The primary website for your Gem i.e. http://www.mydomain.com
    :param user_tags: A comma separated string of user tags
    :param platforms: A comma separated string of platforms for your Gem
    :param icon_path: The relative path to the icon PNG image in your Gem i.e. preview.png
    :param documentation_url: Link to any documentation for your Gem i.e. https://o3de.org/docs/user-guide/gems/...
    :param repo_uri: Optional link to the gem repository for your Gem.
    :param gem_restricted_path: path to the gems restricted folder, can be absolute or relative to the restricted='gems'
    :param gem_restricted_name: str = name of the registered gems restricted path, resolves gem_restricted_path
    :param template_restricted_path: the templates restricted path, can be absolute or relative to the restricted='templates'
    :param template_restricted_name: name of the registered templates restricted path, resolves template_restricted_path
    :param gem_restricted_platform_relative_path: any path after the platform to append to the gem_restricted_path
    :param template_restricted_platform_relative_path: any path after the platform to append to the template_restricted_path
    :param keep_restricted_in_gem: whether or not you want to keep the templates restricted files in your instance or
     separate them out into the restricted folder
    :param keep_license_text: whether or not you want to keep the templates license text in your instance. template can
     have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE}, this controls if you want to keep
      the license text from the template in the new instance. It is false by default because most customers will not
       want license text in their instances, but we may want to keep them.
    :param replace: optional list of strings uses to make concrete names out of templated parameters. X->Y pairs
        Ex. ${Name},TestGem,${Player},TestGemPlayer
        This will cause all references to ${Name} be replaced by TestGem, and all ${Player} replaced by 'TestGemPlayer'
    :param force Overrides existing files even if they exist
    :param system_component_class_id: optionally specify a uuid for the system component class, default is random uuid
    :param editor_system_component_class_id: optionally specify a uuid for the editor system component class, default is
     random uuid
    :param module_id: optionally specify a uuid for the module class, default is random uuid
    :param version: optionally specify a version, default is 1.0.0
    :return: 0 for success or non 0 failure code
    """
    if template_name and template_path:
        logger.error(f'Template Name and Template Path provided, these are mutually exclusive.')
        return 1

    if gem_restricted_name and gem_restricted_path:
        logger.error(f'Gem Restricted Name and Gem Restricted Path provided, these are mutually exclusive.')
        return 1

    if template_restricted_name and template_restricted_path:
        logger.error(f'Template Restricted Name and Template Restricted Path provided, these are mutually exclusive.')
        return 1

    if not template_name and not template_path:
        template_name = 'DefaultGem'

    if template_name and not template_path:
        template_path = manifest.get_registered(template_name=template_name)

    if not template_path:
        logger.error(f'Could not find the template path using name {template_name}.\n'
                     'Has the template been registered yet? It can be registered via the '
                     '"o3de.py register --tp <template-path>" command')
        return 1
    if not os.path.isdir(template_path):
        logger.error(f'Could not find the template {template_name}=>{template_path}')
        return 1

    # the template.json should be in the template_path, make sure it's there and valid
    template_json = template_path / 'template.json'
    if not validation.valid_o3de_template_json(template_json):
        logger.error(f'Template json {template_path} is not valid.')
        return 1

    # read in the template.json
    with open(template_json) as s:
        try:
            template_json_data = json.load(s)
        except json.JSONDecodeError as e:
            logger.error(f'Could not read template json {template_json}: {str(e)}.')
            return 1

    # read template name from the json
    try:
        template_name = template_json_data['template_name']
    except KeyError as e:
        logger.error(f'Could not read "template_name" from template json {template_json}: {str(e)}.')
        return 1

    # if the user has not specified either a restricted name or restricted path
    # see if the template itself specifies a restricted name
    if not template_restricted_name and not template_restricted_path:
        try:
            template_json_restricted_name = template_json_data['restricted_name']
        except KeyError as e:
            # the template json doesn't have a 'restricted_name' element warn and use it
            logger.info(f'The template does not specify a "restricted_name".')
        else:
            template_restricted_name = template_json_restricted_name

    # if no restricted name or path we continue on as if there is no template restricted files.
    if template_restricted_name or template_restricted_path:
        # if the user specified a --template-restricted-name we need to check that against the templates 'restricted_name'
        # if it has one and see if they match. If they match then we don't have a problem. If they don't then we error
        # out. If supplied but not present in the template we warn and use it. If not supplied we set what's in the
        # template. If not supplied and not in the template we continue on as if there is no template restricted files.
        if template_restricted_name and not template_restricted_path:
            # The user specified a --template-restricted-name
            try:
                template_json_restricted_name = template_json_data['restricted_name']
            except KeyError as e:
                # the template json doesn't have a 'restricted_name' element warn and use it
                logger.info(f'The template does not specify a "restricted_name".'
                            f' Using supplied {template_restricted_name}')
            else:
                if template_json_restricted_name != template_restricted_name:
                    logger.error(
                        f'The supplied --template-restricted-name {template_restricted_name} does not match the'
                        f' templates "restricted_name". Either the the --template-restricted-name is incorrect or the'
                        f' templates "restricted_name" is wrong. Note that since this template specifies "restricted_name" as'
                        f' {template_json_restricted_name}, --template-restricted-name need not be supplied.')

            template_restricted_path = manifest.get_registered(restricted_name=template_restricted_name)
        else:
            # The user has supplied the --template-restricted-path, see if that matches the template specifies.
            # If it does then we do not have a problem. If it doesn't match then error out. If not specified
            # in the template then warn and use the --template-restricted-path
            try:
                template_json_restricted_name = template_json_data['restricted_name']
            except KeyError as e:
                # the template json doesn't have a 'restricted_name' element warn and use it
                logger.info(f'The template does not specify a "restricted_name".'
                            f' Using supplied {template_restricted_path}')
            else:
                template_json_restricted_path = manifest.get_registered(
                    restricted_name=template_json_restricted_name)
                if template_json_restricted_path != template_restricted_path:
                    logger.error(
                        f'The supplied --template-restricted-path {template_restricted_path} does not match the'
                        f' templates "restricted_name" {template_restricted_name} => {template_json_restricted_path}.'
                        f' Either the the supplied --template-restricted-path is incorrect or the templates'
                        f' "restricted_name" is wrong. Note that since this template specifies "restricted_name" as'
                        f' {template_json_restricted_name} --template-restricted-path need not be supplied'
                        f' and {template_json_restricted_path} will be used.')
                    return 1
        # check and make sure the restricted path exists
        if template_restricted_path and not os.path.isdir(template_restricted_path):
            logger.error(f'Template restricted path {template_restricted_path} does not exist.')
            return 1

        # If the user specified a --template-restricted-platform-relative-path we need to check that against
        # the templates 'restricted_platform_relative_path' and see if they match. If they match we don't have
        # a problem. If they don't match then we error out. If supplied but not present in the template we warn
        # and use --template-restricted-platform-relative-path. If not supplied we set what's in the template.
        # If not supplied and not in the template set empty string.
        if template_restricted_platform_relative_path:
            # The user specified a --template-restricted-platform-relative-path
            try:
                template_json_restricted_platform_relative_path = pathlib.Path(
                    template_json_data['restricted_platform_relative_path'])
            except KeyError as e:
                # the template json doesn't have a 'restricted_platform_relative_path' element warn and use it
                logger.info(f'The template does not specify a "restricted_platform_relative_path".'
                            f' Using {template_restricted_platform_relative_path}')
            else:
                # the template has a 'restricted_platform_relative_path', if it matches we are fine, if not something is
                # wrong with either the --template-restricted-platform-relative or the template is
                if template_restricted_platform_relative_path != template_json_restricted_platform_relative_path:
                    logger.error(f'The supplied --template-restricted-platform-relative-path'
                                 f' "{template_restricted_platform_relative_path}" does not match the'
                                 f' templates.json  "restricted_platform_relative_path". Either'
                                 f' --template-restricted-platform-relative-path is incorrect or the templates'
                                 f' "restricted_platform_relative_path" is wrong. Note that since this template'
                                 f' specifies "restricted_platform_relative_path" it need not be supplied and'
                                 f' "{template_json_restricted_platform_relative_path}" will be used.')
                    return 1
        else:
            # The user has not supplied --template-restricted-platform-relative-path, try to read it from
            # the template json.
            try:
                template_restricted_platform_relative_path = template_json_data[
                    'restricted_platform_relative_path']
            except KeyError as e:
                # The template json doesn't have a 'restricted_platform_relative_path' element, set empty string.
                template_restricted_platform_relative_path = ''
    if not template_restricted_platform_relative_path:
        template_restricted_platform_relative_path = ''

    # if no gem_path, error
    if not gem_path:
        logger.error('Gem path cannot be empty.')
        return 1
    gem_path = gem_path.resolve()
    if not os.path.isabs(gem_path):
        default_gems_folder = manifest.get_registered(default_folder='gems')
        new_gem_path = default_gems_folder / gem_path
        logger.info(f'Gem Path {gem_path} is not a full path, we must assume its relative'
                    f' to default gems path = {new_gem_path}')
        gem_path = new_gem_path
    if not force and gem_path.is_dir() and len(list(gem_path.iterdir())):
        logger.error(f'Gem path {gem_path} already exists.')
        return 1
    else:
        os.makedirs(gem_path, exist_ok=True)

    # Default to the gem path basename component if gem_name has not been supplied
    if not gem_name:
        gem_name = os.path.basename(gem_path)

    if not utils.validate_identifier(gem_name):
        logger.error(f'Gem name must be fewer than 64 characters, contain only alphanumeric, "_" or "-" characters, and start with a letter.  {gem_name}')
        return 1

    # gem name cannot be the same as a restricted platform name
    if gem_name in restricted_platforms:
        logger.error(f'Gem path cannot be a restricted name. {gem_name}')
        return 1

    # gem restricted name
    if gem_restricted_name and not gem_restricted_path:
        gem_restricted_path = manifest.get_registered(restricted_name=gem_restricted_name)
        if not gem_restricted_path:
            logger.error(f'Gem Restricted Name {gem_restricted_name} cannot be found.')
            return 1

    # gem restricted path
    if gem_restricted_path:
        if not os.path.isabs(gem_restricted_path):
            logger.error(f'Gem Restricted Path {gem_restricted_path} is not an absolute path.')
            return 1
    # neither put it in the default restricted gems
    else:
        gem_restricted_path = manifest.get_o3de_restricted_folder() / "Gems" / gem_name

    # gem restricted relative
    if not gem_restricted_platform_relative_path:
        gem_restricted_platform_relative_path = ''

    # any user supplied replacements
    replacements = list()
    while replace:
        replace_this = replace.pop(0)
        with_this = replace.pop(0)
        replacements.append((replace_this, with_this))

    sanitized_cpp_name = utils.sanitize_identifier_for_cpp(gem_name)
    lowercase_name = gem_name.lower()
    sanitized_lowercase_name = utils.sanitize_identifier_for_cpp(lowercase_name)
    # gem name
    replacements.append(("${Name}", gem_name))
    replacements.append(("${NameUpper}", gem_name.upper()))
    replacements.append(("${NameLower}", lowercase_name))
    replacements.append(("${SanitizedNameLower}", sanitized_lowercase_name))
    replacements.append(("${SanitizedCppName}", sanitized_cpp_name))

    replacements.append(("${DisplayName}", display_name if display_name else gem_name))
    replacements.append(("${Summary}", summary if summary else ""))
    replacements.append(("${License}", license if license else ""))
    replacements.append(("${LicenseURL}", license_url if license_url else ""))
    replacements.append(("${Origin}", origin if origin else ""))
    replacements.append(("${OriginURL}", origin_url if origin_url else ""))
    replacements.append(("${Version}", version if version else "1.0.0"))


    tags = [gem_name]
    if user_tags:
        # Allow commas or spaces as tag separators
        new_tags = user_tags.replace(',', ' ').split() if isinstance(user_tags, str) else user_tags
        tags.extend(new_tags)
    tags_quoted = ','.join(f'"{word.strip()}"' for word in set(tags))
    # remove the first and last quote because those already exist in gem.json
    replacements.append(("${UserTags}", tags_quoted[1:-1]))

    temp_platforms = []
    if platforms:
        #same as tags, allow commas or spaces as platform separators
        new_platforms = platforms.replace(',', ' ').split() if isinstance(platforms, str) else platforms
        temp_platforms.extend(new_platforms)
    platforms_quoted = ','.join(f'"{word.strip()}"' for word in set(temp_platforms))
    #remove the first and last quote because those already exist in gem.json
    replacements.append(("${Platforms}", platforms_quoted[1:-1]))

    replacements.append(("${IconPath}", pathlib.PurePath(icon_path).as_posix() if icon_path else ""))
    replacements.append(("${Requirements}", requirements if requirements else ""))
    replacements.append(("${DocumentationURL}", documentation_url if documentation_url else ""))
    replacements.append(("${RepoURI}", repo_uri if repo_uri else ""))

    # module id is a uuid with { and -
    if module_id:
        replacements.append(("${ModuleClassId}", module_id))
    else:
        replacements.append(("${ModuleClassId}", '{' + str(uuid.uuid4()).upper() + '}'))

    # system component class id is a uuid with { and -
    if system_component_class_id:
        if '{' not in system_component_class_id or '-' not in system_component_class_id:
            logger.error(
                f'System component class id {system_component_class_id} is malformed. Should look like Ex.' +
                '{b60c92eb-3139-454b-a917-a9d3c5819594}')
            return 1
        replacements.append(("${SysCompClassId}", system_component_class_id))
    else:
        replacements.append(("${SysCompClassId}", '{' + str(uuid.uuid4()).upper() + '}'))

    # editor system component class id is a uuid with { and -
    if editor_system_component_class_id:
        if '{' not in editor_system_component_class_id or '-' not in editor_system_component_class_id:
            logger.error(
                f'Editor System component class id {editor_system_component_class_id} is malformed. Should look like'
                f' Ex.' + '{b60c92eb-3139-454b-a917-a9d3c5819594}')
            return 1
        replacements.append(("${EditorSysCompClassId}", editor_system_component_class_id))
    else:
        replacements.append(("${EditorSysCompClassId}", '{' + str(uuid.uuid4()).upper() + '}'))

    if _instantiate_template(template_json_data,
                             gem_name,
                             template_name,
                             gem_path,
                             template_path,
                             gem_restricted_path,
                             template_restricted_path,
                             gem_restricted_platform_relative_path,
                             template_restricted_platform_relative_path,
                             replacements,
                             keep_restricted_in_gem,
                             keep_license_text):
        logger.error(f'Instantiation of the template has failed.')
        return 1

    # We created the gem, now do anything extra that a gem requires

    # If we are not keeping the restricted in the gem read the name of the restricted folder from the
    # restricted json and set that as this gems restricted
    if not keep_restricted_in_gem:
        if gem_restricted_path:
            os.makedirs(gem_restricted_path, exist_ok=True)

            # read the restricted_name from the gems restricted.json
            restricted_json = gem_restricted_path / 'restricted.json'
            if os.path.isfile(restricted_json):
                if not validation.valid_o3de_restricted_json(restricted_json):
                    logger.error(f'Restricted json {restricted_json} is not valid.')
                    return 1
            else:
                with open(restricted_json, 'w') as s:
                    restricted_json_data = {}
                    restricted_json_data.update({'restricted_name': gem_name})
                    s.write(json.dumps(restricted_json_data, indent=4) + '\n')

            with open(restricted_json, 'r') as s:
                try:
                    restricted_json_data = json.load(s)
                except json.JSONDecodeError as e:
                    logger.error(f'Failed to load restricted json {restricted_json}.')
                    return 1

            try:
                restricted_name = restricted_json_data["restricted_name"]
            except KeyError as e:
                logger.error(f'Failed to read "restricted_name" from restricted json {restricted_json}.')
                return 1

            # set the "restricted_name": <restricted_name> element of the gem.json
            gem_json = gem_path / 'gem.json'
            if not validation.valid_o3de_gem_json(gem_json):
                logger.error(f'Gem json {gem_json} is not valid.')
                return 1

            with open(gem_json, 'r') as s:
                try:
                    gem_json_data = json.load(s)
                except json.JSONDecodeError as e:
                    logger.error(f'Failed to load gem json {gem_json}.')
                    return 1

            gem_json_data.update({"restricted": restricted_name})
            os.unlink(gem_json)
            with open(gem_json, 'w') as s:
                try:
                    s.write(json.dumps(gem_json_data, indent=4) + '\n')
                except OSError as e:
                    logger.error(f'Failed to write project json {gem_json}.')
                    return 1
            '''
            for restricted_platform in restricted_platforms:
                restricted_gem = gem_restricted_path / restricted_platform / gem_name
                os.makedirs(restricted_gem, exist_ok=True)
                cmakelists_file_name = restricted_gem / 'CMakeLists.txt'
                if not os.path.isfile(cmakelists_file_name):
                    with open(cmakelists_file_name, 'w') as d:
                        if keep_license_text:
                            d.write(O3DE_LICENSE_TEXT)
            '''
            # Register the restricted
            if not no_register:
                if register.register(restricted_path=gem_restricted_path):
                    logger.error(f'Failed to register the restricted {gem_restricted_path}.')
                    return 1

    print(f'Gem created at {gem_path}')

    # Register the gem with the either o3de_manifest.json, engine.json or project.json based on the gem path
    return register.register(gem_path=gem_path) if not no_register else 0

def create_repo(repo_path: pathlib.Path,
                repo_uri: str,
                repo_name: str = None,
                origin: str = None,
                origin_url: str = None,
                summary: str = None,
                additional_info: str = None,
                force: bool = None,
                replace: list = None,
                no_register: bool = False) -> int:

    template_name = 'RemoteRepo'

    template_path = manifest.get_registered(template_name=template_name)

    template_json = template_path / 'template.json'
  
    # read in the template.json
    with open(template_json) as s:
        try:
            template_json_data = json.load(s)
        except KeyError as e:
            logger.error(f'Could not read template json {template_json}: {str(e)}.')
            return 1

    if not repo_path:
        logger.error('Destination path on local machine cannot be empty.')
        return 1
    if not repo_uri:
        logger.error('Remote repository URI cannot be empty.')
        return 1
    
    repo_json_path = repo_path / 'repo.json'
    # if a repo.json file already exist in directory - can only have one repo.json file per project
    if not force and repo_json_path.is_file():
        logger.error(f'{repo_json_path} already exists. If you want to edit the repo.json properties, use edit-repo-properties command.'
                     'Use --force if you want to override the current repo.json file')
        return 1
    elif repo_path.is_file():
        logger.error(f'{repo_path} already exists and is a file instead of a directory.')
        return 1
    elif not repo_path.is_dir():
        os.makedirs(repo_path, exist_ok=True)

    if not repo_name:
        # repo name default is the last component of repo_path
        repo_name = repo_path.name

    # any user supplied replacements
    replacements = list()
    while replace:
        replace_this = replace.pop(0)
        with_this = replace.pop(0)
        replacements.append((replace_this, with_this))

    replacements.append(("${Name}", repo_name))
    replacements.append(("${RepoURI}", repo_uri))
    replacements.append(("${Origin}", origin if origin else "${Origin}"))
    replacements.append(("${OriginURL}", origin_url if origin_url else "${OriginURL}"))
    replacements.append(("${Summary}", summary if summary else "${Summary}"))
    replacements.append(("${AdditionalInfo}", additional_info if additional_info else "${AdditionalInfo}"))

    # create repo.json file
    _execute_template_json(template_json_data, repo_path, template_path, replacements)

    print(f'Repo created at {repo_path}')

    return register.register(repo_uri=repo_path.as_posix(), force_register_with_o3de_manifest=True) if not no_register else 0

def _run_create_template(args: argparse) -> int:
    return create_template(args.source_path,
                           args.template_path,
                           args.source_name,
                           args.source_restricted_path,
                           args.source_restricted_name,
                           args.template_restricted_path,
                           args.template_restricted_name,
                           args.source_restricted_platform_relative_path,
                           args.template_restricted_platform_relative_path,
                           args.keep_restricted_in_template,
                           args.keep_license_text,
                           args.replace,
                           args.force,
                           args.no_register)


def _run_create_from_template(args: argparse) -> int:
    return create_from_template(args.destination_path,
                                args.template_path,
                                args.template_name,
                                args.destination_name,
                                args.destination_restricted_path,
                                args.destination_restricted_name,
                                args.template_restricted_path,
                                args.template_restricted_name,
                                args.destination_restricted_platform_relative_path,
                                args.template_restricted_platform_relative_path,
                                args.keep_restricted_in_instance,
                                args.keep_license_text,
                                args.replace,
                                args.force,
                                args.no_register)


def _run_create_project(args: argparse) -> int:
    return create_project(args.project_path,
                          args.project_name,
                          args.template_path,
                          args.template_name,
                          args.project_restricted_path,
                          args.project_restricted_name,
                          args.template_restricted_path,
                          args.template_restricted_name,
                          args.project_restricted_platform_relative_path,
                          args.template_restricted_platform_relative_path,
                          args.keep_restricted_in_project,
                          args.keep_license_text,
                          args.replace,
                          args.force,
                          args.no_register,
                          args.system_component_class_id,
                          args.editor_system_component_class_id,
                          args.module_id,
                          args.project_id,
                          args.version)


def _run_create_gem(args: argparse) -> int:
    return create_gem(args.gem_path,
                      args.template_path,
                      args.template_name,
                      args.gem_name,
                      args.display_name,
                      args.summary,
                      args.requirements,
                      args.license,
                      args.license_url,
                      args.origin,
                      args.origin_url,
                      args.user_tags,
                      args.platforms,
                      args.icon_path,
                      args.documentation_url,
                      args.repo_uri,
                      args.gem_restricted_path,
                      args.gem_restricted_name,
                      args.template_restricted_path,
                      args.template_restricted_name,
                      args.gem_restricted_platform_relative_path,
                      args.template_restricted_platform_relative_path,
                      args.keep_restricted_in_gem,
                      args.keep_license_text,
                      args.replace,
                      args.force,
                      args.no_register,
                      args.system_component_class_id,
                      args.editor_system_component_class_id,
                      args.module_id,
                      args.version)

def _run_create_repo(args: argparse) -> int:
    return create_repo(args.repo_path,
                       args.repo_uri,
                       args.repo_name,
                       args.origin,
                       args.origin_url,
                       args.summary,
                       args.additional_info,
                       args.force,
                       args.replace,
                       args.no_register)

def add_args(subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or aggregated by a central python file.
    Ex. Directly run from this file alone with: python engine_template.py create-gem --gem-path TestGem
    OR
    o3de.py can aggregate commands by importing engine_template,
    call add_args and execute: python o3de.py create-gem --gem-path TestGem
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    # turn a directory into a template
    create_template_subparser = subparsers.add_parser('create-template')

    # Sub-commands should declare their own verbosity flag, if desired
    utils.add_verbosity_arg(create_template_subparser)

    create_template_subparser.add_argument('-sp', '--source-path', type=pathlib.Path, required=True,
                                           help='The path to the source that you want to make into a template')
    create_template_subparser.add_argument('-tp', '--template-path', type=pathlib.Path, required=False,
                                           help='The path to the template to create, can be absolute or relative'
                                                ' to default templates path')
    group = create_template_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-srp', '--source-restricted-path', type=pathlib.Path, required=False,
                       default=None,
                       help='The path to the source restricted folder.')
    group.add_argument('-srn', '--source-restricted-name', type=str, required=False,
                       default=None,
                       help='The name of the source restricted folder. If supplied this will resolve'
                            ' the --source-restricted-path.')

    group = create_template_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-trp', '--template-restricted-path', type=pathlib.Path, required=False,
                       default=None,
                       help='The path to the templates restricted folder.')
    group.add_argument('-trn', '--template-restricted-name', type=str, required=False,
                       default=None,
                       help='The name of the templates restricted folder. If supplied this will resolve'
                            ' the --template-restricted-path.')

    create_template_subparser.add_argument('-sn', '--source-name',
                                           type=str,
                                           help='Substitutes any file and path entries which match the source'
                                                ' name within the source-path directory with the ${Name} and'
                                                ' ${SanitizedCppName}.'
                                                'Ex: Path substitution'
                                                '--source-name Foo'
                                                '<source-path>/Code/Include/FooBus.h -> <source-path>/Code/Include/${Name}Bus.h'
                                                'Ex: File content substitution.'
                                                'class FooRequests -> class ${SanitizedCppName}Requests')
    create_template_subparser.add_argument('-srprp', '--source-restricted-platform-relative-path', type=pathlib.Path,
                                           required=False,
                                           default=None,
                                           help='Any path to append to the --source-restricted-path/<platform>'
                                                ' to where the restricted source is. EX.'
                                                ' --source-restricted-path C:/restricted'
                                                ' --source-restricted-platform-relative-path some/folder'
                                                ' => C:/restricted/<platform>/some/folder/<source_name>')
    create_template_subparser.add_argument('-trprp', '--template-restricted-platform-relative-path', type=pathlib.Path,
                                           required=False,
                                           default=None,
                                           help='Any path to append to the --template-restricted-path/<platform>'
                                                ' to where the restricted template source is.'
                                                ' --template-restricted-path C:/restricted'
                                                ' --template-restricted-platform-relative-path some/folder'
                                                ' => C:/restricted/<platform>/some/folder/<template_name>')
    create_template_subparser.add_argument('--keep-restricted-in-template', '-kr', action='store_true',
                                           default=False,
                                           help='Should the template keep the restricted platforms in the template, or'
                                                ' create the restricted files in the restricted folder, default is'
                                                ' False so it will create a restricted folder by default')
    create_template_subparser.add_argument('--keep-license-text', '-kl', action='store_true',
                                           default=False,
                                           help='Should license in the template files text be kept in the'
                                                ' instantiation, default is False, so will not keep license text'
                                                ' by default. License text is defined as all lines of text starting'
                                                ' on a line with {BEGIN_LICENSE} and ending line {END_LICENSE}.')
    create_template_subparser.add_argument('-r', '--replace', type=str, required=False,
                                           nargs='*',
                                           help='String that specifies A->B replacement pairs.'
                                                ' Ex. --replace CoolThing ${the_thing} 1723905 ${id}'
                                                ' Note: <TemplateName> is the last component of template_path'
                                                ' Note: <TemplateName> is automatically ${Name}'
                                                ' Note: <templatename> is automatically ${NameLower}'
                                                ' Note: <TEMPLATENAME> is automatically ${NameUpper}')
    create_template_subparser.add_argument('--force', '-f', action='store_true', default=False,
                                           help='Copies to new template directory even if it exist.')
    create_template_subparser.add_argument('--no-register', action='store_true', default=False,
                                           help='If the template is created successfully, it will not register the'
                                                ' template with the global or engine manifest file.')
    create_template_subparser.set_defaults(func=_run_create_template)

    # create from template
    create_from_template_subparser = subparsers.add_parser('create-from-template')

    # Sub-commands should declare their own verbosity flag, if desired
    utils.add_verbosity_arg(create_from_template_subparser)

    create_from_template_subparser.add_argument('-dp', '--destination-path', type=pathlib.Path, required=True,
                                                help='The path to where you want the template instantiated,'
                                                     ' can be absolute or relative to the current working directory.'
                                                     'Ex. C:/o3de/Test'
                                                     'Test = <destination_name>')

    group = create_from_template_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-tp', '--template-path', type=pathlib.Path, required=False,
                       help='The path to the template you want to instantiate, can be absolute'
                            ' or relative to the current working directory.'
                            'Ex. C:/o3de/Template/TestTemplate'
                            'TestTemplate = <template_name>')
    group.add_argument('-tn', '--template-name', type=str, required=False,
                       help='The name to the registered template you want to instantiate. If supplied this will'
                            ' resolve the --template-path.')

    create_from_template_subparser.add_argument('-dn', '--destination-name', type=str,
                                                help='The name to use when substituting the ${Name} placeholder in instantiated template,'
                                                     ' must be alphanumeric, '
                                                     ' and can contain _ and - characters.'
                                                     ' If no name is provided, will use last component of destination path.'
                                                     ' Ex. New_Gem')

    group = create_from_template_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-drp', '--destination-restricted-path', type=pathlib.Path, required=False,
                       default=None,
                       help='The destination restricted path is where the restricted files'
                            ' will be written to.')
    group.add_argument('-drn', '--destination-restricted-name', type=str, required=False,
                       default=None,
                       help='The name the registered restricted path where the restricted files'
                            ' will be written to. If supplied this will resolve the --destination-restricted-path.')

    group = create_from_template_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-trp', '--template-restricted-path', type=pathlib.Path, required=False,
                       default=None,
                       help='The template restricted path to read from if any')
    group.add_argument('-trn', '--template-restricted-name', type=str, required=False,
                       default=None,
                       help='The name of the registered restricted path to read from if any. If supplied this will'
                            ' resolve the --template-restricted-path.')

    create_from_template_subparser.add_argument('-drprp', '--destination-restricted-platform-relative-path',
                                                type=pathlib.Path,
                                                required=False,
                                                default=None,
                                                help='Any path to append to the --destination-restricted-path/<platform>'
                                                     ' to where the restricted destination is.'
                                                     ' --destination-restricted-path C:/instance'
                                                     ' --destination-restricted-platform-relative-path some/folder'
                                                     ' => C:/instance/<platform>/some/folder/<destination_name>')
    create_from_template_subparser.add_argument('-trprp', '--template-restricted-platform-relative-path',
                                                type=pathlib.Path,
                                                required=False,
                                                default=None,
                                                help='Any path to append to the --template-restricted-path/<platform>'
                                                     ' to where the restricted template is.'
                                                     ' --template-restricted-path C:/restricted'
                                                     ' --template-restricted-platform-relative-path some/folder'
                                                     ' => C:/restricted/<platform>/some/folder/<template_name>')
    create_from_template_subparser.add_argument('--keep-restricted-in-instance', '-kr', action='store_true',
                                                default=False,
                                                help='Should the instance keep the restricted platforms in the instance,'
                                                     ' or create the restricted files in the restricted folder, default'
                                                     ' is False')
    create_from_template_subparser.add_argument('--keep-license-text', '-kl', action='store_true',
                                                default=False,
                                                help='Should license in the template files text be kept in the instantiation,'
                                                     ' default is False, so will not keep license text by default.'
                                                     ' License text is defined as all lines of text starting on a line'
                                                     ' with {BEGIN_LICENSE} and ending line {END_LICENSE}.')
    create_from_template_subparser.add_argument('-r', '--replace', type=str, required=False,
                                                nargs='*',
                                                help='String that specifies A->B replacement pairs.'
                                                     ' Ex. --replace CoolThing \'${the_thing}\' \'${id}\' 1723905'
                                                     ' Note: <DestinationName> is the last component of destination_path'
                                                     ' Note: ${Name} is automatically <DestinationName>'
                                                     ' Note: ${NameLower} is automatically <destinationname>'
                                                     ' Note: ${NameUpper} is automatically <DESTINATIONNAME>')
    create_from_template_subparser.add_argument('--force', '-f', action='store_true', default=False,
                                                help='Copies over instantiated template directory even if it exist.')
    create_from_template_subparser.add_argument('--no-register', action='store_true', default=False,
                                                help='If the project template is instantiated successfully, it will not register the'
                                                     ' project with the global or engine manifest file.')
    create_from_template_subparser.set_defaults(func=_run_create_from_template)

    # creation of a project from a template (like create from template but makes project assumptions)
    create_project_subparser = subparsers.add_parser('create-project')

    # Sub-commands should declare their own verbosity flag, if desired
    utils.add_verbosity_arg(create_project_subparser)

    create_project_subparser.add_argument('-pp', '--project-path', type=pathlib.Path, required=True,
                                          help='The location of the project you wish to create from the template,'
                                               ' can be an absolute path or relative to the current working directory.'
                                               ' Ex. C:/o3de/TestProject'
                                               ' TestProject = <project_name> if --project-name not provided')
    create_project_subparser.add_argument('-pn', '--project-name', type=str, required=False,
                                          help='The name of the project you wish to use, must be alphanumeric, '
                                               ' and can contain _ and - characters.'
                                               ' If no name is provided, will use last component of project path.'
                                               ' Ex. New_Project-123')

    group = create_project_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-tp', '--template-path', type=pathlib.Path, required=False,
                       default=None,
                       help='The path to the template you want to instance, can be absolute or'
                            ' relative to default templates path')
    group.add_argument('-tn', '--template-name', type=str, required=False,
                       default=None,
                       help='The name the registered template you want to instance, defaults'
                            ' to DefaultProject. If supplied this will resolve the --template-path.')

    group = create_project_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-prp', '--project-restricted-path', type=pathlib.Path, required=False,
                       default=None,
                       help='The path to the projects restricted folder, can be absolute or relative to'
                            ' the default restricted projects directory')
    group.add_argument('-prn', '--project-restricted-name', type=str, required=False,
                       default=None,
                       help='The name of the registered projects restricted path. If supplied this will resolve'
                            ' the --project-restricted-path.')

    group = create_project_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-trp', '--template-restricted-path', type=pathlib.Path, required=False,
                       default=None,
                       help='The templates restricted path can be absolute or relative to'
                            ' the default restricted templates directory')
    group.add_argument('-trn', '--template-restricted-name', type=str, required=False,
                       default=None,
                       help='The name of the registered templates restricted path. If supplied this will resolve'
                            ' the --template-restricted-path.')

    create_project_subparser.add_argument('-prprp', '--project-restricted-platform-relative-path', type=pathlib.Path,
                                          required=False,
                                          default=None,
                                          help='Any path to append to the --project-restricted-path/<platform>'
                                               ' to where the restricted project is.'
                                               ' --project-restricted-path C:/restricted'
                                               ' --project-restricted-platform-relative-path some/folder'
                                               ' => C:/restricted/<platform>/some/folder/<project_name>')
    create_project_subparser.add_argument('-trprp', '--template-restricted-platform-relative-path', type=pathlib.Path,
                                          required=False,
                                          default=None,
                                          help='Any path to append to the --template-restricted-path/<platform>'
                                               ' to where the restricted template is.'
                                               ' --template-restricted-path C:/restricted'
                                               ' --template-restricted-platform-relative-path some/folder'
                                               ' => C:/restricted/<platform>/some/folder/<template_name>')
    create_project_subparser.add_argument('--keep-restricted-in-project', '-kr', action='store_true',
                                          default=False,
                                          help='Should the new project keep the restricted platforms in the project, or'
                                               'create the restricted files in the restricted folder, default is False')
    create_project_subparser.add_argument('--keep-license-text', '-kl', action='store_true',
                                          default=False,
                                          help='Should license in the template files text be kept in the instantiation,'
                                               ' default is False, so will not keep license text by default.'
                                               ' License text is defined as all lines of text starting on a line'
                                               ' with {BEGIN_LICENSE} and ending line {END_LICENSE}.')
    create_project_subparser.add_argument('-r', '--replace', required=False,
                                          nargs='*',
                                          help='String that specifies ADDITIONAL A->B replacement pairs. ${Name} and'
                                               ' all other standard project replacements will be automatically'
                                               ' inferred from the project name. These replacements will superseded'
                                               ' all inferred replacements.'
                                               ' Ex. --replace \'${DATE}\' 1/1/2020 \'${id}\' 1723905'
                                               ' Note: <ProjectName> is the last component of project_path'
                                               ' Note: ${Name} is automatically <ProjectName>'
                                               ' Note: ${NameLower} is automatically <projectname>'
                                               ' Note: ${NameUpper} is automatically <PROJECTNAME>')
    create_project_subparser.add_argument('--system-component-class-id', type=uuid.UUID, required=False,
                                          help='The uuid you want to associate with the system class component, default'
                                               ' is a random uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_project_subparser.add_argument('--editor-system-component-class-id', type=uuid.UUID,
                                          required=False,
                                          help='The uuid you want to associate with the editor system class component,'
                                               ' default is a random uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_project_subparser.add_argument('--module-id', type=uuid.UUID, required=False,
                                          help='The uuid you want to associate with the module, default is a random'
                                               ' uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_project_subparser.add_argument('--project-id', type=str, required=False,
                                          help='The str id you want to associate with the project, default is a random uuid'
                                               ' Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_project_subparser.add_argument('--force', '-f', action='store_true', default=False,
                                          help='Copies over instantiated template directory even if it exist.')
    create_project_subparser.add_argument('--no-register', action='store_true', default=False,
                                          help='If the project template is instantiated successfully, it will not register the'
                                               ' project with the global or engine manifest file.')
    create_project_subparser.add_argument('--version', type=str, required=False,
                                          help='An optional version. Defaults to 1.0.0')
    create_project_subparser.set_defaults(func=_run_create_project)

    # creation of a gem from a template (like create from template but makes gem assumptions)
    create_gem_subparser = subparsers.add_parser('create-gem')

    # Sub-commands should declare their own verbosity flag, if desired
    utils.add_verbosity_arg(create_gem_subparser)

    create_gem_subparser.add_argument('-gp', '--gem-path', type=pathlib.Path, required=True,
                                      help='The gem path, can be absolute or relative to the current working directory')
    create_gem_subparser.add_argument('-gn', '--gem-name', type=str,
                                      help='The name to use when substituting the ${Name} placeholder for the gem,'
                                           ' must be alphanumeric, '
                                           ' and can contain _ and - characters.'
                                           ' If no name is provided, will use last component of gem path.'
                                           ' Ex. New_Gem')

    group = create_gem_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-tp', '--template-path', type=pathlib.Path, required=False,
                       default=None,
                       help='The template path you want to instance, can be absolute or relative'
                            ' to default templates path')
    group.add_argument('-tn', '--template-name', type=str, required=False,
                       default=None,
                       help='The name of the registered template you want to instance, defaults'
                            ' to DefaultGem. If supplied this will resolve the --template-path.')

    group = create_gem_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-grp', '--gem-restricted-path', type=pathlib.Path, required=False,
                       default=None,
                       help='The gem restricted path, can be absolute or relative to'
                            ' the default restricted gems directory')
    group.add_argument('-grn', '--gem-restricted-name', type=str, required=False,
                       default=None,
                       help='The name of the gem to look up the gem restricted path if any.'
                            'If supplied this will resolve the --gem-restricted-path.')

    group = create_gem_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-trp', '--template-restricted-path', type=pathlib.Path, required=False,
                       default=None,
                       help='The templates restricted path, can be absolute or relative to'
                            ' the default restricted templates directory')
    group.add_argument('-trn', '--template-restricted-name', type=str, required=False,
                       default=None,
                       help='The name of the registered templates restricted path. If supplied'
                            ' this will resolve the --template-restricted-path.')

    create_gem_subparser.add_argument('-grprp', '--gem-restricted-platform-relative-path', type=pathlib.Path,
                                      required=False,
                                      default=None,
                                      help='Any path to append to the --gem-restricted-path/<platform>'
                                           ' to where the restricted template is.'
                                           ' --gem-restricted-path C:/restricted'
                                           ' --gem-restricted-platform-relative-path some/folder'
                                           ' => C:/restricted/<platform>/some/folder/<gem_name>')
    create_gem_subparser.add_argument('-trprp', '--template-restricted-platform-relative-path', type=pathlib.Path,
                                      required=False,
                                      default=None,
                                      help='Any path to append to the --template-restricted-path/<platform>'
                                           ' to where the restricted template is.'
                                           ' --template-restricted-path C:/restricted'
                                           ' --template-restricted-platform-relative-path some/folder'
                                           ' => C:/restricted/<platform>/some/folder/<template_name>')
    create_gem_subparser.add_argument('-r', '--replace', type=str, required=False,
                                      nargs='*',
                                      help='String that specifies ADDITIONAL A->B replacement pairs. ${Name} and'
                                           ' all other standard gem replacements will be automatically inferred'
                                           ' from the gem name. These replacements will superseded all inferred'
                                           ' replacement pairs.'
                                           ' Ex. --replace \'${DATE}\' 1/1/2020 \'${id}\' 1723905'
                                           ' Note: <GemName> is the last component of gem_path'
                                           ' Note: ${Name} is automatically <GemName>'
                                           ' Note: ${NameLower} is automatically <gemname>'
                                           ' Note: ${NameUpper} is automatically <GEMANME>')
    create_gem_subparser.add_argument('--keep-restricted-in-gem', '-kr', action='store_true',
                                      default=False,
                                      help='Should the new gem keep the restricted platforms in the project, or'
                                           'create the restricted files in the restricted folder, default is False')
    create_gem_subparser.add_argument('--keep-license-text', '-kl', action='store_true',
                                      default=False,
                                      help='Should license in the template files text be kept in the instantiation,'
                                           ' default is False, so will not keep license text by default.'
                                           ' License text is defined as all lines of text starting on a line'
                                           ' with {BEGIN_LICENSE} and ending line {END_LICENSE}.')
    create_gem_subparser.add_argument('--system-component-class-id', type=uuid.UUID, required=False,
                                      help='The uuid you want to associate with the system class component, default'
                                           ' is a random uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_gem_subparser.add_argument('--editor-system-component-class-id', type=uuid.UUID,
                                      required=False,
                                      help='The uuid you want to associate with the editor system class component,'
                                           ' default is a random uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_gem_subparser.add_argument('--module-id', type=uuid.UUID, required=False,
                                      help='The uuid you want to associate with the gem module,'
                                           ' default is a random uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_gem_subparser.add_argument('--force', '-f', action='store_true', default=False,
                                      help='Copies over instantiated template directory even if it exist.')
    create_gem_subparser.add_argument('--no-register', action='store_true', default=False,
                                      help='If the gem template is instantiated successfully, it will not register the'
                                           ' gem with the global, project or engine manifest file.')
    create_gem_subparser.add_argument('-dn', '--display-name', type=str, required=False,
                       help='The name displayed on the Gem Catalog')
    create_gem_subparser.add_argument('-s', '--summary', type=str, required=False,
                       default='A short description of this Gem',
                       help='A short description of this Gem')
    create_gem_subparser.add_argument('-req', '--requirements', type=str, required=False,
                       default='Notice of any requirements for this Gem i.e. This requires X other gem',
                       help='Notice of any requirements for this Gem i.e. This requires X other gem')
    create_gem_subparser.add_argument('-l', '--license', type=str, required=False,
                       default='License used i.e. Apache-2.0 or MIT',
                       help='License used i.e. Apache-2.0 or MIT')
    create_gem_subparser.add_argument('-lu', '--license-url', type=str, required=False,
                       default='Link to the license web site i.e. https://opensource.org/licenses/Apache-2.0',
                       help='Link to the license web site i.e. https://opensource.org/licenses/Apache-2.0')
    create_gem_subparser.add_argument('-o', '--origin', type=str, required=False,
                       default='The name of the originator or creator',
                       help='The name of the originator or creator i.e. XYZ Inc.')
    create_gem_subparser.add_argument('-ou', '--origin-url', type=str, required=False,
                       default='The website for this Gem',
                       help='The website for your Gem. i.e. http://www.mydomain.com')
    create_gem_subparser.add_argument('-ut', '--user-tags', type=str, nargs='*', required=False,
                       help='Adds tag(s) to user_tags property. Can be specified multiple times.')
    create_gem_subparser.add_argument('-pl', '--platforms', type=str, nargs='*', required=False,
                       help='Add platform(s) to platforms property. Can be specified multiple times.')
    create_gem_subparser.add_argument('-ip', '--icon-path', type=str, required=False,
                       default="preview.png",
                       help='Select Gem icon path')
    create_gem_subparser.add_argument('-du', '--documentation-url', type=str, required=False,
                       default='Link to any documentation of your Gem',
                       help='Link to any documentation of your Gem i.e. https://o3de.org/docs/user-guide/gems/...')
    create_gem_subparser.add_argument('-ru', '--repo-uri', type=str, required=False,
                       help='An optional URI for the gem repository where your Gem can be downloaded')
    create_gem_subparser.add_argument('--version', type=str, required=False,
                       help='An optional version. Defaults to 1.0.0')
    create_gem_subparser.set_defaults(func=_run_create_gem)


    create_repo_subparser = subparsers.add_parser('create-repo')
    create_repo_subparser.add_argument('-rp','--repo-path', type=pathlib.Path, required=True,
                                       help = 'The location of the remote repo you wish to create from the RemoteRepo template,'
                                       'can be an absolute path or relative to the current working directory'
                                       'Ex. C:/o3de/TestRemoteRepo'
                                       'TestRemoteRepo = <RemoteRepo_name> if --project_name not provided')
    create_repo_subparser.add_argument('-ru', '--repo-uri', type=str, required=True,
                                       help = 'The online remote repository uri')
    create_repo_subparser.add_argument('-rn', '--repo-name', type=str, required=False,
                                       help = 'The name to use when substituting the ${Name} placeholder for the remote repo,'
                                           ' If no name is provided, will use last component of the repo-path provided by user.'
                                           ' Ex. RemoteRepo1')
    create_repo_subparser.add_argument('-o', '--origin', type=str, required=False,
                                       default='o3de',
                                       help = 'The name of the originator or creator i.e. o3de.')
    create_repo_subparser.add_argument('-ou', '--origin-url', type=str, required=False,
                                       help='The origin website for your remote repository. i.e. http://www.mydomain.com')
    create_repo_subparser.add_argument('-s', '--summary', type=str, required=False,
                                       help='A short description of this Repo')
    create_repo_subparser.add_argument('-ai', '--additional-info', type=str, required=False,
                                       help='Any additional info you want to add to this Remote repo that is not necessary to know')
    create_repo_subparser.add_argument('-f','--force', action='store_true', default=False,
                                      help='Copies over instantiated RemoteRepo directory even if it exist.')
    create_repo_subparser.add_argument('-r', '--replace', type=str, required=False,
                                      nargs='*',
                                      help='String that specifies ADDITIONAL A->B replacement pairs. ${Name} and'
                                           ' all other standard gem replacements will be automatically inferred'
                                           ' from the repo name. These replacements will superseded all inferred'
                                           ' replacement pairs.'
                                           ' Ex. --replace \'${DATE}\' 1/1/2020 \'${id}\' 1723905'
                                           ' Note: <RepoName> is the last component of repo_path'
                                           ' Note: ${Name} is automatically <Name>')
    create_repo_subparser.add_argument('--no-register', action='store_true', default=False,
                                      help='If the repo template is instantiated successfully, it will not register the'
                                               ' repo with the global manifest file.')                                       
    create_repo_subparser.set_defaults(func=_run_create_repo)

if __name__ == "__main__":
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers
    subparsers = the_parser.add_subparsers(help='To get help on a sub-command:\nengine_template.py <sub-command> -h',
                                           title='Sub-Commands', dest='command', required=True)

    # add args to the parsers
    add_args(subparsers)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1
    logger.info('Success!' if ret == 0 else 'Completed with issues: result {}'.format(ret))

    # return
    sys.exit(ret)
