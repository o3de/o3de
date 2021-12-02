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
import sys
import json
import uuid
import re


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
    'Paris'
}

template_file_name = 'template.json'
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
        t_data = t_data.replace('${Random_Uuid}', str(uuid.uuid4()), 1)

    if not keep_license_text:
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
                           keep_license_text: bool = False) -> None:
    # create dirs first
    # for each createDirectory entry, transform the folder name
    for create_directory in json_data['createDirectories']:
        # construct the new folder name
        new_dir = destination_path / create_directory['dir']

        # transform the folder name
        new_dir = _transform(new_dir.as_posix(), replacements, keep_license_text)

        # create the folder
        os.makedirs(new_dir, exist_ok=True)

    # for each copyFiles entry, _transformCopy the templated source file into a concrete instance file or
    # regular copy if not templated
    for copy_file in json_data['copyFiles']:
        # construct the input file name
        in_file = template_path / 'Template' / copy_file['file']

        # the file can be marked as optional, if it is and it does not exist skip
        if copy_file['isOptional'] and copy_file['isOptional'] == 'true':
            if not os.path.isfile(in_file):
                continue

        # construct the output file name
        out_file = destination_path / copy_file['file']

        # transform the output file name
        out_file = _transform(out_file.as_posix(), replacements, keep_license_text)

        # if for some reason the output folder for this file was not created above do it now
        os.makedirs(os.path.dirname(out_file), exist_ok=True)

        # if templated _transformCopy the file, if not just copy it
        if copy_file['isTemplated']:
            _transform_copy(in_file, out_file, replacements, keep_license_text)
        else:
            shutil.copy(in_file, out_file)


def _execute_restricted_template_json(json_data: dict,
                                      restricted_platform: str,
                                      destination_name,
                                      template_name,
                                      destination_path: pathlib.Path,
                                      destination_restricted_path: pathlib.Path,
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

    # create dirs first
    # for each createDirectory entry, transform the folder name
    for create_directory in json_data['createDirectories']:
        # construct the new folder name
        new_dir = destination_restricted_path / restricted_platform / destination_restricted_platform_relative_path\
                  / destination_name / create_directory['dir']
        if keep_restricted_in_instance:
            new_dir = destination_path / create_directory['origin']

        # transform the folder name
        new_dir = _transform(new_dir.as_posix(), replacements, keep_license_text)

        # create the folder
        os.makedirs(new_dir, exist_ok=True)

    # for each copyFiles entry, _transformCopy the templated source file into a concrete instance file or
    # regular copy if not templated
    for copy_file in json_data['copyFiles']:
        # construct the input file name
        in_file = template_restricted_path / restricted_platform / template_restricted_platform_relative_path\
                  / template_name / 'Template' / copy_file['file']

        # the file can be marked as optional, if it is and it does not exist skip
        if copy_file['isOptional'] and copy_file['isOptional'] == 'true':
            if not os.path.isfile(in_file):
                continue

        # construct the output file name
        out_file = destination_restricted_path / restricted_platform / destination_restricted_platform_relative_path\
                   / destination_name / copy_file['file']
        if keep_restricted_in_instance:
            out_file = destination_path / copy_file['origin']

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
    _execute_template_json(template_json_data,
                           destination_path,
                           template_path,
                           replacements,
                           keep_license_text)

    # execute restricted platform jsons if any
    if template_restricted_path:
        for restricted_platform in os.listdir(template_restricted_path):
            if os.path.isfile(restricted_platform):
                continue
            template_restricted_platform = template_restricted_path / restricted_platform
            template_restricted_platform_path_rel = template_restricted_platform / template_restricted_platform_relative_path / template_name
            platform_json = template_restricted_platform_path_rel / template_file_name

            if os.path.isfile(platform_json):
                if not validation.valid_o3de_template_json(platform_json):
                    logger.error(f'Template json {platform_json} is invalid.')
                    return 1

                # load the template json and execute it
                with open(platform_json, 'r') as s:
                    try:
                        json_data = json.load(s)
                    except json.JSONDecodeError as e:
                        logger.error(f'Failed to load {platform_json}: ' + str(e))
                        return 1
                    else:
                        _execute_restricted_template_json(json_data,
                                                          restricted_platform,
                                                          destination_name,
                                                          template_name,
                                                          destination_path,
                                                          destination_restricted_path,
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
                    force: bool = False) -> int:
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
    # source_name is now the last component of the source_path
    if not source_name:
        source_name = os.path.basename(source_path)
    sanitized_source_name = utils.sanitize_identifier_for_cpp(source_name)

    # if no template path, use default_templates_folder path
    if not template_path:
        default_templates_folder = manifest.get_registered(default_folder='templates')
        template_path = default_templates_folder / source_name
        logger.info(f'Template path empty. Using default templates folder {template_path}')
    if not force and template_path.is_dir() and len(list(template_path.iterdir())):
        logger.error(f'Template path {template_path} already exists.')
        return 1

    # Make sure the output directory for the template is outside the source path directory
    try:
        template_path.relative_to(source_path)
    except ValueError:
        pass
    else:
        logger.error(f'Template output path {template_path} cannot be a subdirectory of the source_path {source_path}\n')
        return 1

    # template name is now the last component of the template_path
    template_name = os.path.basename(template_path)

    # template name cannot be the same as a restricted platform name
    if template_name in restricted_platforms:
        logger.error(f'Template path cannot be a restricted name. {template_name}')
        return 1

    if source_restricted_name and not source_restricted_path:
        source_restricted_path = manifest.get_registered(restricted_name=source_restricted_name)

    # source_restricted_path
    if source_restricted_path:
        if not os.path.isabs(source_restricted_path):
            engine_json = manifest.get_this_engine_path() / 'engine.json'
            if not validation.valid_o3de_engine_json(engine_json):
                logger.error(f"Engine json {engine_json} is not valid.")
                return 1
            with open(engine_json) as s:
                try:
                    engine_json_data = json.load(s)
                except json.JSONDecodeError as e:
                    logger.error(f"Failed to read engine json {engine_json}: {str(e)}")
                    return 1
                try:
                    engine_restricted = engine_json_data['restricted_name']
                except KeyError as e:
                    logger.error(f"Engine json {engine_json} restricted not found.")
                    return 1
            engine_restricted_folder = manifest.get_registered(restricted_name=engine_restricted)
            new_source_restricted_path = engine_restricted_folder / source_restricted_path
            logger.info(f'Source restricted path {source_restricted_path} not a full path. We must assume this engines'
                        f' restricted folder {new_source_restricted_path}')
        if not os.path.isdir(source_restricted_path):
            logger.error(f'Source restricted path {source_restricted_path} is not a folder.')
            return 1

    if template_restricted_name and not template_restricted_path:
        template_restricted_path = manifest.get_registered(restricted_name=template_restricted_name)

    if not template_restricted_name:
        template_restricted_name = template_name

    # template_restricted_path
    if template_restricted_path:
        if not os.path.isabs(template_restricted_path):
            default_templates_restricted_folder = manifest.get_registered(restricted_name='templates')
            new_template_restricted_path = default_templates_restricted_folder / template_restricted_path
            logger.info(f'Template restricted path {template_restricted_path} not a full path. We must assume the'
                        f' default templates restricted folder {new_template_restricted_path}')
            template_restricted_path = new_template_restricted_path

        if os.path.isdir(template_restricted_path):
            # see if this is already a restricted path, if it is get the "restricted_name" from the restricted json
            # so we can set "restricted_name" to it for this template
            restricted_json = template_restricted_path / 'restricted.json'
            if os.path.isfile(restricted_json):
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
                                 prefer_sanitized_name: bool = False) -> (bool, str):
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

        # See if this file has the ModuleClassId
        try:
            pattern = r'.*AZ_RTTI\(\$\{SanitizedCppName\}Module, \"(?P<ModuleClassId>\{.*-.*-.*-.*-.*\})\",'
            module_class_id = re.search(pattern, t_data).group('ModuleClassId')
            replacements.append((module_class_id, '${ModuleClassId}'))
            t_data = t_data.replace(module_class_id, '${ModuleClassId}')
        except Exception as e:
            pass

        # See if this file has the SysCompClassId
        try:
            pattern = r'.*AZ_COMPONENT\(\$\{SanitizedCppName\}SystemComponent, \"(?P<SysCompClassId>\{.*-.*-.*-.*-.*\})\"'
            sys_comp_class_id = re.search(pattern, t_data).group('SysCompClassId')
            replacements.append((sys_comp_class_id, '${SysCompClassId}'))
            t_data = t_data.replace(sys_comp_class_id, '${SysCompClassId}')
        except Exception as e:
            pass

        # See if this file has the EditorSysCompClassId
        try:
            pattern = r'.*AZ_COMPONENT\(\$\{SanitizedCppName\}EditorSystemComponent, \"(?P<EditorSysCompClassId>\{.*-.*-.*-.*-.*\})\"'
            editor_sys_comp_class_id = re.search(pattern, t_data).group('EditorSysCompClassId')
            replacements.append((editor_sys_comp_class_id, '${EditorSysCompClassId}'))
            t_data = t_data.replace(editor_sys_comp_class_id, '${EditorSysCompClassId}')
        except Exception as e:
            pass

        # we want to send back the transformed data and whether or not this file
        # may require transformation when instantiated. So if the input data is not the
        # same as the output, then we transformed it which means there may be a transformation
        # needed to instance it. Or if we chose not to remove the license text then potentially it
        # may need to be transformed when instanced
        if s_data != t_data or '{BEGIN_LICENSE}' in t_data:
            return True, t_data
        else:
            return False, t_data

    def _transform_into_template_restricted_filename(s_data: object,
                                                     platform: str) -> (bool, object):
        """
        Internal function to transform a restricted platform file name into restricted template file name
        :param s_data: the input data, this could be file data or file name data
        :return: bool: whether or not the returned data MAY need to be transformed to instantiate it
                 t_data: potentially transformed data 0 for success or non 0 failure code
        """
        # copy the src data to the transformed data, then operate only on transformed data
        t_data = s_data

        # run all the replacements
        for replacement in replacements:
            t_data = t_data.replace(replacement[0], replacement[1])

        # the name of the Platform should follow the '/Platform/{platform}'
        t_data = t_data.replace(f"Platform/{platform}", '')

        # we want to send back the transformed data and whether or not this file
        # may require transformation when instantiated. So if the input data is not the
        # same as the output, then we transformed it which means there may be a transformation
        # needed to instance it.
        if s_data != t_data:
            return True, t_data
        else:
            return False, t_data

    def _transform_restricted_into_copyfiles_and_createdirs(source_path: pathlib.Path,
                                                            restricted_platform: str,
                                                            root_abs: pathlib.Path,
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

            # create the relative entry by removing the root_abs
            try:
                entry_rel = entry_abs.relative_to(root_abs)
            except ValueError as err:
                logger.warning(f'Unable to create relative path: {str(err)}')

            # report what file we are processing so we have a good idea if it breaks on what file it broke on
            logger.info(f'Processing file: {entry_abs}')

            # this is a restricted file, so we need to transform it, unpalify it
            # restricted/<platform>/<source_path_rel>/some/folders/<file> ->
            # <source_path_rel>/some/folders/Platform/<platform>/<file>
            #
            # C:/repo/Lumberyard/restricted/Jasper/TestDP/CMakeLists.txt ->
            # C:/repo/Lumberyard/TestDP/Platform/Jasper/CMakeLists.txt
            #
            _, origin_entry_rel = _transform_into_template(entry_rel.as_posix())
            components = list(origin_entry_rel.parts)
            num_components = len(components)

            # see how far along the source path the restricted folder matches
            # then hopefully there is a Platform folder, warn if there isn't
            before = []
            after = []
            relative = ''

            if os.path.isdir(entry_abs):
                for x in range(0, num_components):
                    relative += f'{components[x]}/'
                    if os.path.isdir(f'{source_path}/{relative}'):
                        before.append(components[x])
                    else:
                        after.append(components[x])
            else:
                for x in range(0, num_components - 1):
                    relative += f'{components[x]}/'
                    if os.path.isdir(f'{source_path}/{relative}'):
                        before.append(components[x])
                    else:
                        after.append(components[x])

                after.append(components[num_components - 1])

            before.append("Platform")
            warn_if_not_platform = source_path / pathlib.Path(*before)
            before.append(restricted_platform)
            before.extend(after)

            origin_entry_rel = pathlib.Path(*before)

            if not os.path.isdir(warn_if_not_platform):
                logger.warning(
                    f'{entry_abs} -> {origin_entry_rel}: Other Platforms not found in {warn_if_not_platform}')

            destination_entry_rel = origin_entry_rel
            destination_entry_abs = template_path / 'Template' / origin_entry_rel

            # clean up any relative leading slashes
            if origin_entry_rel.as_posix().startswith('/'):
                origin_entry_rel = pathlib.Path(origin_entry_rel.as_posix().lstrip('/'))
            if destination_entry_rel.as_posix().startswith('/'):
                destination_entry_rel = pathlib.Path(destination_entry_rel.as_posix().lstrip('/'))

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

                copy_files.append({
                    "file": destination_entry_rel,
                    "origin": origin_entry_rel,
                    "isTemplated": templated,
                    "isOptional": False
                })
            else:
                create_dirs.append({
                    "dir": destination_entry_rel,
                    "origin": origin_entry_rel
                })
                _transform_restricted_into_copyfiles_and_createdirs(source_path, restricted_platform, root_abs,
                                                                    entry_abs)

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

            # create the relative entry by removing the root_abs
            entry_rel = entry_abs
            try:
                entry_rel = entry_abs.relative_to(root_abs)
            except ValueError as err:
                logger.warning(f'Unable to create relative path: {str(err)}')

            # report what file we are processing so we have a good idea if it breaks on what file it broke on
            logger.info(f'Processing file: {entry_abs}')

            # see if the entry is a platform file, if it is then we save its copyfile data in a platform specific list
            # then at the end we can save the restricted ones separately
            found_platform = ''
            platform = False
            if not keep_restricted_in_template and 'Platform' in entry_abs.parts:
                platform = True
                try:
                    # the name of the Platform should follow the '/Platform/'
                    pattern = r'/Platform/(?P<Platform>[^/:*?\"<>|\r\n]+/?)'
                    found_platform = re.search(pattern, entry_abs).group('Platform')
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
            _, origin_entry_rel = _transform_into_template(entry_rel.as_posix())
            if platform and found_platform in restricted_platforms:
                # if we don't have a template restricted path and we found restricted files... warn and skip
                # the file/dir
                if not template_restricted_path:
                    logger.warning("Restricted platform files found!!! {entry_rel}, {found_platform}")
                    continue
                _, destination_entry_rel = _transform_into_template_restricted_filename(entry_rel, found_platform)
                destination_entry_abs = template_restricted_path / found_platform\
                                        / template_restricted_platform_relative_path / template_name / 'Template'\
                                        / destination_entry_rel
            else:
                destination_entry_rel = origin_entry_rel
                destination_entry_abs = template_path / 'Template' / destination_entry_rel

            # clean up any relative leading slashes
            if isinstance(origin_entry_rel, pathlib.Path):
                origin_entry_rel = origin_entry_rel.as_posix()
            if origin_entry_rel.startswith('/'):
                origin_entry_rel = pathlib.Path(origin_entry_rel.lstrip('/'))
            if isinstance(destination_entry_rel, pathlib.Path):
                destination_entry_rel = destination_entry_rel.as_posix()
            if destination_entry_rel.startswith('/'):
                destination_entry_rel = pathlib.Path(destination_entry_rel.lstrip('/'))

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
                        "origin": origin_entry_rel,
                        "isTemplated": templated,
                        "isOptional": False
                    })
                else:
                    copy_files.append({
                        "file": destination_entry_rel,
                        "origin": origin_entry_rel,
                        "isTemplated": templated,
                        "isOptional": False
                    })
            else:
                # if the folder was for a restricted platform add the entry to the restricted platform, otherwise add it
                # to the non restricted
                if platform and found_platform in restricted_platforms:
                    restricted_platform_entries[found_platform]['createDirs'].append({
                        "dir": destination_entry_rel,
                        "origin": origin_entry_rel
                    })
                else:
                    create_dirs.append({
                        "dir": destination_entry_rel,
                        "origin": origin_entry_rel
                    })

                # recurse using the same root and this folder
                _transform_dir_into_copyfiles_and_createdirs(root_abs, entry_abs)

    # when we run the transformation to create copyfiles, createdirs, any we find will go in here
    copy_files = []
    create_dirs = []

    # when we run the transformation any restricted platforms entries we find will go in here
    restricted_platform_entries = {}

    # Every project will have a unrestricted folder which is src_path_abs which MAY have restricted files in it, and
    # each project MAY have a restricted folder which will only have restricted files in them. The process is the
    # same for all of them and the result will be a separation of all restricted files from unrestricted files. We do
    # this by running the transformation first over the src path abs and then on each restricted folder for this project
    # we find. This will effectively combine all sources then separates all the restricted.

    # run the transformation on the src, which may or may not have restricted files
    _transform_dir_into_copyfiles_and_createdirs(source_path)

    # every src may have a matching restricted folder per restricted platform
    # run the transformation on each src restricted folder
    if source_restricted_path:
        for restricted_platform in os.listdir(source_restricted_path):
            restricted_platform_src_path_abs = source_restricted_path / restricted_platform\
                                               / source_restricted_platform_relative_path / source_name
            if os.path.isdir(restricted_platform_src_path_abs):
                _transform_restricted_into_copyfiles_and_createdirs(source_path, restricted_platform,
                                                                    restricted_platform_src_path_abs)

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
    if template_restricted_path:
        json_data.update({'restricted_name': template_restricted_name})
        if template_restricted_platform_relative_path != '':
            json_data.update({'template_restricted_platform_relative_path': template_restricted_platform_relative_path})
    json_data.update({'copyFiles': copy_files})
    json_data.update({'createDirectories': create_dirs})

    json_name =  template_path / template_file_name

    with json_name.open('w') as s:
        s.write(json.dumps(json_data, indent=4) + '\n')

    # copy the default preview.png
    preview_png_src = this_script_parent / 'resources' / 'preview.png'
    preview_png_dst = template_path / 'Template' / 'preview.png'
    if not os.path.isfile(preview_png_dst):
        shutil.copy(preview_png_src, preview_png_dst)

    # if no restricted template path was given and restricted platform files were found
    if not template_restricted_path and len(restricted_platform_entries):
        logger.info(f'Restricted platform files found!!! and no template restricted path was found...')

    if template_restricted_path:
        # now write out each restricted platform template json separately
        for restricted_platform in restricted_platform_entries:
            restricted_template_path = template_restricted_path / restricted_platform\
                                       / template_restricted_platform_relative_path / template_name

            # sort
            restricted_platform_entries[restricted_platform]['copyFiles'].sort(key=lambda x: x['file'])
            restricted_platform_entries[restricted_platform]['createDirs'].sort(key=lambda x: x['dir'])

            json_data = {}
            json_data.update({'template_name': template_name})
            json_data.update(
                {'origin': f'The primary repo for {template_name} goes here: i.e. http://www.mydomain.com'})
            json_data.update(
                {'license': f'What license {template_name} uses goes here: i.e. https://opensource.org/licenses/MIT'})
            json_data.update({'display_name': template_name})
            json_data.update({'summary': f"A short description of {template_name}."})
            json_data.update({'canonical_tags': []})
            json_data.update({'user_tags': [f'{template_name}']})
            json_data.update({'icon_path': "preview.png"})
            json_data.update({'copyFiles': restricted_platform_entries[restricted_platform]['copyFiles']})
            json_data.update({'createDirectories': restricted_platform_entries[restricted_platform]['createDirs']})

            json_name = restricted_template_path / template_file_name
            os.makedirs(os.path.dirname(json_name), exist_ok=True)

            with json_name.open('w') as s:
                s.write(json.dumps(json_data, indent=4) + '\n')

            preview_png_dst = restricted_template_path / 'Template' /' preview.png'
            if not os.path.isfile(preview_png_dst):
                shutil.copy(preview_png_src, preview_png_dst)

    return 0


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
                         force: bool = False) -> int:
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
            logger.error(f'Could read template json {template_json}: {str(e)}.')
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
        logger.error(f'Destination path {destination_path} already exists.')
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
        if os.path.isabs(destination_restricted_path):
            restricted_default_path = manifest.get_registered(default_folder='restricted')
            new_destination_restricted_path = restricted_default_path / destination_restricted_path
            logger.info(f'{destination_restricted_path} is not a full path, making it relative'
                        f' to default restricted path = {new_destination_restricted_path}')
            destination_restricted_path = new_destination_restricted_path
    elif template_restricted_path:
        restricted_default_path = manifest.get_registered(restricted_name='restricted')
        logger.info(f'--destination-restricted-path is not specified, using default restricted path / destination name'
                    f' = {restricted_default_path}')
        destination_restricted_path = restricted_default_path

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
    # dst name is Name
    replacements.append(("${Name}", destination_name))
    replacements.append(("${NameUpper}", destination_name.upper()))
    replacements.append(("${NameLower}", destination_name.lower()))
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

            # read the restricted_name from the destination restricted.json
            restricted_json = destination_restricted_path / 'restricted.json'
            if not os.path.isfile(restricted_json):
                with open(restricted_json, 'w') as s:
                    restricted_json_data = {}
                    restricted_json_data.update({'restricted_name': destination_name})
                    s.write(json.dumps(restricted_json_data, indent=4) + '\n')

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
                   module_id: str = None) -> int:
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
    :param system_component_class_id: optionally specify a uuid for the system component class, default is random uuid
    :param editor_system_component_class_id: optionally specify a uuid for the editor system component class, default is
     random uuid
    :param module_id: optionally specify a uuid for the module class, default is random uuid
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
            logger.error(f'Could read template json {template_json}: {str(e)}.')
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
    if not force and project_path.is_dir() and len(list(project_path.iterdir())):
        logger.error(f'Project path {project_path} already exists and is not empty.')
        return 1
    elif not os.path.isdir(project_path):
        os.makedirs(project_path, exist_ok=force)

    if not project_name:
        # project name is now the last component of the project_path
        project_name = os.path.basename(project_path)

    if not utils.validate_identifier(project_name):
        logger.error(f'Project name must be fewer than 64 characters, contain only alphanumeric, "_" or "-" characters, and start with a letter.  {project_name}')
        return 1

    # project name cannot be the same as a restricted platform name
    if project_name in restricted_platforms:
        logger.error(f'Project name cannot be a restricted name. {project_name}')
        return 1

    # project restricted name
    if project_restricted_name and not project_restricted_path:
        project_restricted_path = manifest.get_registered(restricted_name=project_restricted_name)

    # project restricted path
    elif project_restricted_path:
        if not os.path.isabs(project_restricted_path):
            default_projects_restricted_folder = manifest.get_registered(restricted_name='projects')
            new_project_restricted_path = default_projects_restricted_folder/ project_restricted_path
            logger.info(f'Project restricted path {project_restricted_path} is not a full path, we must assume its'
                        f' relative to default projects restricted path = {new_project_restricted_path}')
            project_restricted_path = new_project_restricted_path
    elif template_restricted_path:
        project_restricted_default_path = manifest.get_registered(restricted_name='projects')
        logger.info(f'--project-restricted-path is not specified, using default project restricted path / project name'
                    f' = {project_restricted_default_path}')
        project_restricted_path = project_restricted_default_path

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

    # module id is a uuid with { and -
    if module_id:
        replacements.append(("${ModuleClassId}", module_id))
    else:
        replacements.append(("${ModuleClassId}", '{' + str(uuid.uuid4()) + '}'))

    # system component class id is a uuid with { and -
    if system_component_class_id:
        if '{' not in system_component_class_id or '-' not in system_component_class_id:
            logger.error(
                f'System component class id {system_component_class_id} is malformed. Should look like Ex.' +
                '{b60c92eb-3139-454b-a917-a9d3c5819594}')
            return 1
        replacements.append(("${SysCompClassId}", system_component_class_id))
    else:
        replacements.append(("${SysCompClassId}", '{' + str(uuid.uuid4()) + '}'))

    # editor system component class id is a uuid with { and -
    if editor_system_component_class_id:
        if '{' not in editor_system_component_class_id or '-' not in editor_system_component_class_id:
            logger.error(
                f'Editor System component class id {editor_system_component_class_id} is malformed. Should look like'
                f' Ex.' + '{b60c92eb-3139-454b-a917-a9d3c5819594}')
            return 1
        replacements.append(("${EditorSysCompClassId}", editor_system_component_class_id))
    else:
        replacements.append(("${EditorSysCompClassId}", '{' + str(uuid.uuid4()) + '}'))

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
            restricted_json =  project_restricted_path / 'restricted.json'
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

            # set the "restricted_name": "restricted_name" element of the project.json
            if not validation.valid_o3de_project_json(project_json):
                logger.error(f'Project json {project_json} is not valid.')
                return 1

            with open(project_json, 'r') as s:
                try:
                    project_json_data = json.load(s)
                except json.JSONDecodeError as e:
                    logger.error(f'Failed to load project json {project_json}.')
                    return 1

            project_json_data.update({"restricted_name": restricted_name})
            os.unlink(project_json)
            with open(project_json, 'w') as s:
                try:
                    s.write(json.dumps(project_json_data, indent=4) + '\n')
                except OSError as e:
                    logger.error(f'Failed to write project json {project_json}.')
                    return 1

            for restricted_platform in restricted_platforms:
                restricted_project = project_restricted_path / restricted_platform / project_name
                os.makedirs(restricted_project, exist_ok=True)
                cmakelists_file_name = restricted_project/ 'CMakeLists.txt'
                if not os.path.isfile(cmakelists_file_name):
                    with open(cmakelists_file_name, 'w') as d:
                        if keep_license_text:
                            d.write('# {BEGIN_LICENSE}\n')
                            d.write('# Copyright (c) Contributors to the Open 3D Engine Project.\n')
                            d.write('# For complete copyright and license terms please see the LICENSE at the root of this distribution.\n')
                            d.write('#\n')
                            d.write('# SPDX-License-Identifier: Apache-2.0 OR MIT\n')
                            d.write('# {END_LICENSE}\n')


    # Register the project with the either o3de_manifest.json or engine.json
    # and set the project.json "engine" field to match the
    # engine.json "engine_name" field
    return register.register(project_path=project_path) if not no_register else 0


def create_gem(gem_path: pathlib.Path,
               template_path: pathlib.Path = None,
               template_name: str = None,
               gem_name: str = None,
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
               module_id: str = None) -> int:
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
            logger.error(f'Could read template json {template_json}: {str(e)}.')
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
    if not force and gem_path.is_dir() and len(list(gem_path.iterdir())):
        logger.error(f'Gem path {gem_path} already exists and is not empty.')
        return 1
    else:
        os.makedirs(gem_path, exist_ok=force)

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

    # gem restricted path
    elif gem_restricted_path:
        if not os.path.isabs(gem_restricted_path):
            gem_restricted_default_path = manifest.get_registered(restricted_name='gems')
            if gem_restricted_default_path:
                new_gem_restricted_path = gem_restricted_default_path / gem_restricted_path
                logger.info(f'Gem restricted path {gem_restricted_path} is not a full path, we must assume its'
                            f' relative to default gems restricted path = {new_gem_restricted_path}')
                gem_restricted_path = new_gem_restricted_path
    else:
        gem_restricted_default_path = manifest.get_registered(restricted_name='gems')
        if gem_restricted_default_path:
            logger.info(f'--gem-restricted-path is not specified, using default <gem restricted path> / <gem name>'
                        f' = {gem_restricted_default_path}')
            gem_restricted_path = gem_restricted_default_path / gem_name

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
    # gem name
    replacements.append(("${Name}", gem_name))
    replacements.append(("${NameUpper}", gem_name.upper()))
    replacements.append(("${NameLower}", gem_name.lower()))
    replacements.append(("${SanitizedCppName}", sanitized_cpp_name))

    # module id is a uuid with { and -
    if module_id:
        replacements.append(("${ModuleClassId}", module_id))
    else:
        replacements.append(("${ModuleClassId}", '{' + str(uuid.uuid4()) + '}'))

    # system component class id is a uuid with { and -
    if system_component_class_id:
        if '{' not in system_component_class_id or '-' not in system_component_class_id:
            logger.error(
                f'System component class id {system_component_class_id} is malformed. Should look like Ex.' +
                '{b60c92eb-3139-454b-a917-a9d3c5819594}')
            return 1
        replacements.append(("${SysCompClassId}", system_component_class_id))
    else:
        replacements.append(("${SysCompClassId}", '{' + str(uuid.uuid4()) + '}'))

    # editor system component class id is a uuid with { and -
    if editor_system_component_class_id:
        if '{' not in editor_system_component_class_id or '-' not in editor_system_component_class_id:
            logger.error(
                f'Editor System component class id {editor_system_component_class_id} is malformed. Should look like'
                f' Ex.' + '{b60c92eb-3139-454b-a917-a9d3c5819594}')
            return 1
        replacements.append(("${EditorSysCompClassId}", editor_system_component_class_id))
    else:
        replacements.append(("${EditorSysCompClassId}", '{' + str(uuid.uuid4()) + '}'))

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

                # set the "restricted_name": "restricted_name" element of the gem.json
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

                gem_json_data.update({"restricted_name": restricted_name})
                os.unlink(gem_json)
                with open(gem_json, 'w') as s:
                    try:
                        s.write(json.dumps(gem_json_data, indent=4) + '\n')
                    except OSError as e:
                        logger.error(f'Failed to write project json {gem_json}.')
                        return 1

                for restricted_platform in restricted_platforms:
                    restricted_gem = gem_restricted_path / restricted_platform/ gem_name
                    os.makedirs(restricted_gem, exist_ok=True)
                    cmakelists_file_name = restricted_gem / 'CMakeLists.txt'
                    if not os.path.isfile(cmakelists_file_name):
                        with open(cmakelists_file_name, 'w') as d:
                            if keep_license_text:
                                d.write('# {BEGIN_LICENSE}\n')
                                d.write('# Copyright (c) Contributors to the Open 3D Engine Project.\n')
                                d.write('# For complete copyright and license terms please see the LICENSE at the root of this distribution.\n')
                                d.write('#\n')
                                d.write('# SPDX-License-Identifier: Apache-2.0 OR MIT\n')
                                d.write('# {END_LICENSE}\n')
    # Register the gem with the either o3de_manifest.json, engine.json or project.json based on the gem path
    return register.register(gem_path=gem_path) if not no_register else 0


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
                           args.force)


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
                                args.force)


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
                          args.module_id)


def _run_create_gem(args: argparse) -> int:
    return create_gem(args.gem_path,
                      args.template_path,
                      args.template_name,
                      args.gem_name,
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
                      args.module_id)


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
    create_template_subparser.add_argument('-kr', '--keep-restricted-in-template', action='store_true',
                                           default=False,
                                           help='Should the template keep the restricted platforms in the template, or'
                                                ' create the restricted files in the restricted folder, default is'
                                                ' False so it will create a restricted folder by default')
    create_template_subparser.add_argument('-kl', '--keep-license-text', action='store_true',
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
    create_template_subparser.add_argument('-f', '--force', action='store_true', default=False,
                                      help='Copies to new template directory even if it exist.')
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

    create_from_template_subparser.add_argument('-drprp', '--destination-restricted-platform-relative-path', type=pathlib.Path,
                                                required=False,
                                                default=None,
                                                help='Any path to append to the --destination-restricted-path/<platform>'
                                                     ' to where the restricted destination is.'
                                                     ' --destination-restricted-path C:/instance'
                                                     ' --destination-restricted-platform-relative-path some/folder'
                                                     ' => C:/instance/<platform>/some/folder/<destination_name>')
    create_from_template_subparser.add_argument('-trprp', '--template-restricted-platform-relative-path', type=pathlib.Path,
                                                required=False,
                                                default=None,
                                                help='Any path to append to the --template-restricted-path/<platform>'
                                                     ' to where the restricted template is.'
                                                     ' --template-restricted-path C:/restricted'
                                                     ' --template-restricted-platform-relative-path some/folder'
                                                     ' => C:/restricted/<platform>/some/folder/<template_name>')
    create_from_template_subparser.add_argument('-kr', '--keep-restricted-in-instance', action='store_true',
                                                default=False,
                                                help='Should the instance keep the restricted platforms in the instance,'
                                                     ' or create the restricted files in the restricted folder, default'
                                                     ' is False')
    create_from_template_subparser.add_argument('-kl', '--keep-license-text', action='store_true',
                                                default=False,
                                                help='Should license in the template files text be kept in the instantiation,'
                                                     ' default is False, so will not keep license text by default.'
                                                     ' License text is defined as all lines of text starting on a line'
                                                     ' with {BEGIN_LICENSE} and ending line {END_LICENSE}.')
    create_from_template_subparser.add_argument('-r', '--replace', type=str, required=False,
                                                nargs='*',
                                                help='String that specifies A->B replacement pairs.'
                                                     ' Ex. --replace CoolThing ${the_thing} ${id} 1723905'
                                                     ' Note: <DestinationName> is the last component of destination_path'
                                                     ' Note: ${Name} is automatically <DestinationName>'
                                                     ' Note: ${NameLower} is automatically <destinationname>'
                                                     ' Note: ${NameUpper} is automatically <DESTINATIONNAME>')
    create_from_template_subparser.add_argument('-f', '--force', action='store_true', default=False,
                                      help='Copies over instantiated template directory even if it exist.')
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
    create_project_subparser.add_argument('-kr', '--keep-restricted-in-project', action='store_true',
                                          default=False,
                                          help='Should the new project keep the restricted platforms in the project, or'
                                               'create the restricted files in the restricted folder, default is False')
    create_project_subparser.add_argument('-kl', '--keep-license-text', action='store_true',
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
                                               ' Ex. --replace ${DATE} 1/1/2020 ${id} 1723905'
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
    create_project_subparser.add_argument('-f', '--force', action='store_true', default=False,
                                      help='Copies over instantiated template directory even if it exist.')
    create_project_subparser.add_argument('--no-register', action='store_true', default=False,
                                      help='If the project template is instantiated successfully, it will not register the'
                                           ' project with the global or engine manifest file.')
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
                                           ' Ex. --replace ${DATE} 1/1/2020 ${id} 1723905'
                                           ' Note: <GemName> is the last component of gem_path'
                                           ' Note: ${Name} is automatically <GemName>'
                                           ' Note: ${NameLower} is automatically <gemname>'
                                           ' Note: ${NameUpper} is automatically <GEMANME>')
    create_gem_subparser.add_argument('-kr', '--keep-restricted-in-gem', action='store_true',
                                      default=False,
                                      help='Should the new gem keep the restricted platforms in the project, or'
                                           'create the restricted files in the restricted folder, default is False')
    create_gem_subparser.add_argument('-kl', '--keep-license-text', action='store_true',
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
    create_gem_subparser.add_argument('-f', '--force', action='store_true', default=False,
                        help='Copies over instantiated template directory even if it exist.')
    create_gem_subparser.add_argument('--no-register', action='store_true', default=False,
                                      help='If the gem template is instantiated successfully, it will not register the'
                                           ' gem with the global, project or engine manifest file.')
    create_gem_subparser.set_defaults(func=_run_create_gem)


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
