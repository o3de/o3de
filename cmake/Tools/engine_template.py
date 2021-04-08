#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
"""
This file contains all the code that has to do with creating and instantiate engine templates
"""
import argparse
import logging
import os
import shutil
import sys
import json
import uuid
import re

from cmake.Tools import utils
from cmake.Tools import common

logger = logging.getLogger()
logging.basicConfig()

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
    t_data = s_data
    for replacement in replacements:
        t_data = t_data.replace(replacement[0], replacement[1])

    # if someone hand edits the template to have ${Random_Uuid} then replace it with a randomly generated uuid
    while '${Random_Uuid}' in t_data:
        t_data = t_data.replace('${Random_Uuid}', str(uuid.uuid4()), 1)

    ##################################################################
    # For some reason the re.sub call here gets into some kind of infinite
    # loop and never returns on some files consistently.
    # Until I figure out why we can use the string replacement method
    # if not keep_license_text:
    #    t_data = re.sub(r"^(//|'''|#)\s*{BEGIN_LICENSE}((.|\n)*){END_LICENSE}\n", "", t_data, flags=re.DOTALL)

    if not keep_license_text:
        while '{BEGIN_LICENSE}' in t_data:
            start = t_data.find('{BEGIN_LICENSE}')
            if start != -1:
                line_start = t_data.rfind('\n', 0, start)
                if line_start == -1:
                    line_start = 0
                end = t_data.find('{END_LICENSE}')
                end = t_data.find('\n', end)
                if end != -1:
                    t_data = t_data[:line_start] + t_data[end+1:]
    ###################################################################
    return t_data


def _transform_copy(source_file: str,
                    destination_file: str,
                    replacements: list,
                    keep_license_text: bool = False) -> None:
    """
    Internal function called to transform and copy a source file into templated destination file
    :param source_file: the source file to be transformed
    :param destination_file: the destination file, this is the transformed file
    :param replacements: list of transformation pairs A->B
    :param keep_restricted_in_instance: whether or not you want ot keep the templates restricted files in your instance
     or separate them out into the restricted folder
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


def _execute_template_json(json_data: str,
                           destination_path: str,
                           template_path: str,
                           replacements: list,
                           keep_license_text: bool = False) -> None:
    # create dirs first
    # for each createDirectory entry, transform the folder name
    for create_directory in json_data['createDirectories']:
        # construct the new folder name
        new_dir = f"{destination_path}/{create_directory['dir']}"

        # transform the folder name
        new_dir = _transform(new_dir, replacements, keep_license_text)

        # create the folder
        os.makedirs(new_dir, exist_ok=True)

    # for each copyFiles entry, _transformCopy the templated source file into a concrete instance file or
    # regular copy if not templated
    for copy_file in json_data['copyFiles']:
        # construct the input file name
        in_file = f"{template_path}/Template/{copy_file['file']}"

        # the file can be marked as optional, if it is and it does not exist skip
        if copy_file['isOptional'] and copy_file['isOptional'] == 'true':
            if not os.path.isfile(in_file):
                continue

        # construct the output file name
        out_file = f"{destination_path}/{copy_file['file']}"

        # transform the output file name
        out_file = _transform(out_file, replacements, keep_license_text)

        # if for some reason the output folder for this file was not created above do it now
        os.makedirs(os.path.dirname(out_file), exist_ok=True)

        # if templated _transformCopy the file, if not just copy it
        if copy_file['isTemplated']:
            _transform_copy(in_file, out_file, replacements, keep_license_text)
        else:
            shutil.copy(in_file, out_file)


def _execute_restricted_template_json(json_data: str,
                                      restricted_platform: str,
                                      destination_name,
                                      template_name,
                                      destination_path: str,
                                      destination_restricted_path: str,
                                      template_restricted_path: str,
                                      destination_restricted_platform_relative_path: str,
                                      template_restricted_platform_relative_path: str,
                                      replacements: list,
                                      keep_restricted_in_instance: bool = False,
                                      keep_license_text: bool = False) -> None:
    # create dirs first
    # for each createDirectory entry, transform the folder name
    for create_directory in json_data['createDirectories']:
        # construct the new folder name
        new_dir = f"{destination_restricted_path}/{restricted_platform}/{destination_restricted_platform_relative_path}/{destination_name}/{create_directory['dir']}".replace('//', '/')
        if keep_restricted_in_instance:
            new_dir = f"{destination_path}/{create_directory['origin']}".replace('//', '/')

        # transform the folder name
        new_dir = _transform(new_dir, replacements, keep_license_text)

        # create the folder
        os.makedirs(new_dir, exist_ok=True)

    # for each copyFiles entry, _transformCopy the templated source file into a concrete instance file or
    # regular copy if not templated
    for copy_file in json_data['copyFiles']:
        # construct the input file name
        in_file = f"{template_restricted_path}/{restricted_platform}/{template_restricted_platform_relative_path}/{template_name}/Template/{copy_file['file']}".replace('//', '/')

        # the file can be marked as optional, if it is and it does not exist skip
        if copy_file['isOptional'] and copy_file['isOptional'] == 'true':
            if not os.path.isfile(in_file):
                continue

        # construct the output file name
        out_file = f"{destination_restricted_path}/{restricted_platform}/{destination_restricted_platform_relative_path}/{destination_name}/{copy_file['file']}".replace('//', '/')
        if keep_restricted_in_instance:
            out_file = f"{destination_path}/{copy_file['origin']}".replace('//', '/')

        # transform the output file name
        out_file = _transform(out_file, replacements, keep_license_text)

        # if for some reason the output folder for this file was not created above do it now
        os.makedirs(os.path.dirname(out_file), exist_ok=True)

        # if templated _transformCopy the file, if not just copy it
        if copy_file['isTemplated']:
            _transform_copy(in_file, out_file, replacements, keep_license_text)
        else:
            shutil.copy(in_file, out_file)


def _instantiate_template(destination_name: str,
                          template_name: str,
                          destination_path: str,
                          template_path: str,
                          destination_restricted_path: str,
                          template_restricted_path: str,
                          destination_restricted_platform_relative_path: str,
                          template_restricted_platform_relative_path: str,
                          replacements: list,
                          keep_restricted_in_instance: bool = False,
                          keep_license_text: bool = False) -> int:
    """
    Internal function to create a concrete instance from a template

    :param destination_path: the folder you want to instantiate the template in, absolute or dev_root relative
    :param template_path: the path of the template
    :param template_path_rel: the relative path of the template
    :param template_restricted_path: the path of the restricted template
    :param replacements: optional list of strings uses to make concrete names out of templated parameters. X->Y pairs
        Ex. ${Name},TestGem,${Player},TestGemPlayer
        This will cause all references to ${Name} be replaced by TestGem, and all ${Player} replaced by 'TestGemPlayer'
    :param keep_restricted_in_instance: whether or not you want ot keep the templates restricted files in your instance
     or separate them out into the restricted folder
    :param keep_license_text: whether or not you want ot keep the templates license text in your instance.
        template can have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE},
        this controls if you want to keep the license text from the template in the new instance. It is false by default
        because most customers will not want license text in there instances, but we may want to keep them.
    :return: 0 for success or non 0 failure code
    """
    # make sure the template json is found
    template_json = f'{template_path}/{template_file_name}'
    if not os.path.isfile(template_json):
        logger.error(f'Template json {template_json} not found.')
        return 1

    # load the template json and execute it
    with open(template_json, 'r') as s:
        try:
            json_data = json.load(s)
        except Exception as e:
            logger.error(f'Failed to load {template_json}: ' + str(e))
            return 1
        _execute_template_json(json_data,
                               destination_path,
                               template_path,
                               replacements,
                               keep_license_text)

    # execute restricted platform jsons if any
    if os.path.isdir(template_restricted_path):
        for restricted_platform in os.listdir(template_restricted_path):
            template_restricted_platform = f'{template_restricted_path}/{restricted_platform}'
            template_restricted_platform_path_rel = f'{template_restricted_platform}/{template_restricted_platform_relative_path}/{template_name}'
            platform_json = f'{template_restricted_platform_path_rel}/{template_file_name}'.replace('//', '/')
            if os.path.isfile(platform_json):
                # load the template json and execute it
                with open(platform_json, 'r') as s:
                    try:
                        json_data = json.load(s)
                    except Exception as e:
                        logger.error(f'Failed to load {platform_json}: ' + str(e))
                        return 1
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


def create_template(dev_root: str,
                    source_path: str,
                    template_path: str,
                    source_restricted_path: str,
                    template_restricted_path: str,
                    source_restricted_platform_relative_path: str,
                    template_restricted_platform_relative_path: str,
                    keep_restricted_in_template: bool = False,
                    keep_license_text: bool = False,
                    replace: list = None) -> int:
    """
    Create a generic template from a source directory using replacement

    :param dev_root: the path to dev root of the engine
    :param source_path: source folder, absolute or dev_root relative
    :param template_path: the path of the template to create, can be absolute or relative to dev root/Templates, if none
     then it will be the source_name
    :param source_restricted_path: path to the projects restricted folder
    :param template_restricted_path: path to the templates restricted folder
    :param replace: optional list of strings uses to make templated parameters out of concrete names. X->Y pairs
     Ex. TestGem,${Name},TestGemPlayer,${Player}
     This will cause all references to 'TestGem' be replaced by ${Name}, and all 'TestGemPlayer' replaced by ${Player}
     Note these replacements are executed in order, so if you have larger matches, do them first, i.e.
     TestGemPlayer,${Player},TestGem,${Name}
     TestGemPlayer will get matched first and become ${Player} and will not become ${Name}Player
    :param keep_restricted_in_template: whether or not you want ot keep the templates restricted in your template.
    :param keep_license_text: whether or not you want ot keep the templates license text in your instance.
     template can have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE},
     this controls if you want to keep the license text from the template in the new instance. It is false by default
     because most customers will not want license text in there instances, but we may want to keep them.
    :return: 0 for success or non 0 failure code
    """
    # if no dev root error
    if not dev_root:
        logger.error('Dev root cannot be empty.')
        return 1
    dev_root = dev_root.replace('\\', '/')
    if not os.path.isabs(dev_root):
        dev_root = os.path.abspath(dev_root)
    if not os.path.isdir(dev_root):
        logger.error(f'Dev root {dev_root} is not a folder.')
        return 1

    # if no src path error
    if not source_path:
        logger.error('Src path cannot be empty.')
        return 1
    source_path = source_path.replace('\\', '/')
    if not os.path.isabs(source_path):
        source_path = f'{dev_root}/{source_path}'
    if not os.path.isdir(source_path):
        logger.error(f'Src path {source_path} is not a folder.')
        return 1

    # source_name is now the last component of the source_path
    source_name = os.path.basename(source_path)

    # if no new template path error
    if not template_path:
        logger.warning('Template path empty. Using source name')
        template_path = source_name
    template_path = template_path.replace('\\', '/')
    if not os.path.isabs(template_path):
        template_path = f'{dev_root}/Templates/{template_path}'
    if os.path.isdir(template_path):
        logger.error(f'Template path {template_path} is already exists.')
        return 1

    # template name is now the last component of the template_path
    template_name = os.path.basename(template_path)

    # template name cannot be the same as a restricted platform name
    if template_name in restricted_platforms:
        logger.error(f'Template path cannot be a restricted name. {template_name}')
        return 1

    # source_restricted_path
    source_restricted_path = source_restricted_path.replace('\\', '/')
    if not os.path.isabs(source_restricted_path):
        source_restricted_path = f'{dev_root}/{source_restricted_path}'
    if not os.path.isdir(source_restricted_path):
        logger.error(f'Src restricted path {source_restricted_path} is not a folder.')
        return 1

    # template_restricted_path
    template_restricted_path = template_restricted_path.replace('\\', '/')
    if not os.path.isabs(template_restricted_path):
        template_restricted_path = f'{dev_root}/{template_restricted_path}'
    if not os.path.isdir(template_restricted_path):
        os.makedirs(template_restricted_path)

    # source restricted relative
    source_restricted_platform_relative_path = source_restricted_platform_relative_path.replace('\\', '/')

    # template restricted relative
    template_restricted_platform_relative_path = template_restricted_platform_relative_path.replace('\\', '/')

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

    def _transform_into_template(s_data: object) -> (bool, object):
        """
        Internal function to transform any data into templated data
        :param source_data: the input data, this could be file data or file name data
        :return: bool: whether or not the returned data MAY need to be transformed to instantiate it
                 t_data: potentially transformed data 0 for success or non 0 failure code
        """
        # copy the src data to the transformed data, then operate only on transformed data
        t_data = s_data

        # run all the replacements
        for replacement in replacements:
            t_data = t_data.replace(replacement[0], replacement[1])

        if not keep_license_text:
            t_data = re.sub(r"(//|'''|#)\s*{BEGIN_LICENSE}((.|\n)*){END_LICENSE}\n", "", t_data, flags=re.DOTALL)

        # See if this file has the ModuleClassId
        try:
            pattern = r'.*AZ_RTTI\(\$\{Name\}Module, \"(?P<ModuleClassId>\{.*-.*-.*-.*-.*\})\", AZ::Module'
            module_class_id = re.search(pattern, t_data).group('ModuleClassId')
            replacements.append((module_class_id, '${ModuleClassId}'))
            t_data = t_data.replace(module_class_id, '${ModuleClassId}')
        except Exception as e:
            pass

        # See if this file has the SysCompClassId
        try:
            pattern = r'.*AZ_COMPONENT\(\$\{Name\}SystemComponent, \"(?P<SysCompClassId>\{.*-.*-.*-.*-.*\})\"'
            sys_comp_class_id = re.search(pattern, t_data).group('SysCompClassId')
            replacements.append((sys_comp_class_id, '${SysCompClassId}'))
            t_data = t_data.replace(sys_comp_class_id, '${SysCompClassId}')
        except Exception as e:
            pass

        # See if this file has the EditorSysCompClassId
        try:
            pattern = r'.*AZ_COMPONENT\(\$\{Name\}EditorSystemComponent, \"(?P<EditorSysCompClassId>\{.*-.*-.*-.*-.*\})\"'
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
        :param source_data: the input data, this could be file data or file name data
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

    def _transform_restricted_into_copyfiles_and_createdirs(source_path: str,
                                                            restricted_platform: str,
                                                            root_abs: str,
                                                            path_abs: str = None) -> None:
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
            entry_abs = f'{path_abs}/{entry}'

            # create the relative entry by removing the root_abs
            entry_rel = entry_abs.replace(root_abs + '/', '')

            # report what file we are processing so we have a good idea if it breaks on what file it broke on
            logger.info(f'Processing file: {entry_abs}')

            # this is a restricted file, so we need to transform it, unpalify it
            # restricted/<platform>/<source_path_rel>/some/folders/<file> ->
            # <source_path_rel>/some/folders/Platform/<platform>/<file>
            #
            # C:/repo/Lumberyard/restricted/Jasper/TestDP/CMakeLists.txt ->
            # C:/repo/Lumberyard/TestDP/Platform/Jasper/CMakeLists.txt
            #
            _, origin_entry_rel = _transform_into_template(entry_rel)
            components = origin_entry_rel.split('/')
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
                for x in range(0, num_components-1):
                    relative += f'{components[x]}/'
                    if os.path.isdir(f'{source_path}/{relative}'):
                        before.append(components[x])
                    else:
                        after.append(components[x])

                after.append(components[num_components-1])

            before.append("Platform")
            warn_if_not_platform = f'{source_path}/{"/".join(before)}'
            before.append(restricted_platform)
            before.extend(after)

            origin_entry_rel = '/'.join(before)

            if not os.path.isdir(warn_if_not_platform):
                logger.warning(f'{entry_abs} -> {origin_entry_rel}: Other Platforms not found in {warn_if_not_platform}')

            destination_entry_rel = origin_entry_rel
            destination_entry_abs = f'{template_path}/Template/{origin_entry_rel}'

            # clean up any collapsed folders
            origin_entry_rel = origin_entry_rel.replace('//', '/')
            destination_entry_abs = destination_entry_abs.replace('//', '/')
            destination_entry_rel = destination_entry_rel.replace('//', '/')

            # clean up any relative leading slashes
            while origin_entry_rel.startswith('/'):
                origin_entry_rel = origin_entry_rel[1:]
            while destination_entry_rel.startswith('/'):
                destination_entry_rel = destination_entry_rel[1:]

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
                            templated, source_data = _transform_into_template(source_data)

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
                _transform_restricted_into_copyfiles_and_createdirs(source_path, restricted_platform, root_abs, entry_abs)

    def _transform_dir_into_copyfiles_and_createdirs(root_abs: str,
                                                     path_abs: str = None) -> None:
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
            entry_abs = f'{path_abs}/{entry}'

            # create the relative entry by removing the root_abs
            entry_rel = entry_abs.replace(root_abs + '/', '')

            # report what file we are processing so we have a good idea if it breaks on what file it broke on
            logger.info(f'Processing file: {entry_abs}')

            # see if the entry is a platform file, if it is then we save its copyfile data in a platform specific list
            # then at the end we can save the restricted ones separately
            found_platform = ''
            platform = False
            if not keep_restricted_in_template and '/Platform' in entry_abs:
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
            _, origin_entry_rel = _transform_into_template(entry_rel)
            if platform and found_platform in restricted_platforms:
                _, destination_entry_rel = _transform_into_template_restricted_filename(entry_rel, found_platform)
                destination_entry_abs = f'{template_restricted_path}/{found_platform}/{template_restricted_platform_relative_path}/{template_name}/Template/{destination_entry_rel}'
            else:
                destination_entry_rel = origin_entry_rel
                destination_entry_abs = f'{template_path}/Template/{destination_entry_rel}'

            # clean up any collapsed folders
            origin_entry_rel = origin_entry_rel.replace('//', '/')
            destination_entry_abs = destination_entry_abs.replace('//', '/')
            destination_entry_rel = destination_entry_rel.replace('//', '/')

            # clean up any relative leading slashes
            while origin_entry_rel.startswith('/'):
                origin_entry_rel = origin_entry_rel[1:]
            while destination_entry_rel.startswith('/'):
                destination_entry_rel = destination_entry_rel[1:]

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
                            templated, source_data = _transform_into_template(source_data)

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
    for restricted_platform in os.listdir(source_restricted_path):
        restricted_platform_src_path_abs = f'{source_restricted_path}/{restricted_platform}/{source_restricted_platform_relative_path}/{source_name}'.replace('//', '/')
        if os.path.isdir(restricted_platform_src_path_abs):
            _transform_restricted_into_copyfiles_and_createdirs(source_path, restricted_platform, restricted_platform_src_path_abs)

    # sort
    copy_files.sort(key=lambda x: x['file'])
    create_dirs.sort(key=lambda x: x['dir'])

    # now we should have all our copyFiles and createDirs entries, so write out the Template json for the unrestricted
    # platforms all together
    json_data = {}
    json_data.update({'template_name': template_name})
    json_data.update({'origin': f'The primary repo for {template_name} goes here: i.e. http://www.mydomain.com'})
    json_data.update({'license': f'What license {template_name} uses goes here: i.e. https://opensource.org/licenses/MIT'})
    json_data.update({'display_name': template_name})
    json_data.update({'summary': f"A short description of {template_name}."})
    json_data.update({'canonical_tags': []})
    json_data.update({'user_tags': [f"{template_name}"]})
    json_data.update({'icon_path': "preview.png"})
    json_data.update({'copyFiles': copy_files})
    json_data.update({'createDirectories': create_dirs})

    json_name = f'{template_path}/{template_file_name}'

    # if the json file we are about to write already exists for some reason, delete it
    if os.path.isfile(json_name):
        os.unlink(json_name)
    with open(json_name, 'w') as s:
        s.write(json.dumps(json_data, indent=4))

    #copy the default preview.png
    this_script_parent = os.path.dirname(os.path.realpath(__file__))
    preview_png_src = f'{this_script_parent}/preview.png'
    preview_png_dst = f'{template_path}/Template/preview.png'
    if not os.path.isfile(preview_png_dst):
        shutil.copy(preview_png_src, preview_png_dst)

    # now write out each restricted platform template json separately
    for restricted_platform in restricted_platform_entries:
        restricted_template_path = f'{template_restricted_path}/{restricted_platform}/{template_restricted_platform_relative_path}/{template_name}'.replace('//', '/')

        #sort
        restricted_platform_entries[restricted_platform]['copyFiles'].sort(key=lambda x: x['file'])
        restricted_platform_entries[restricted_platform]['createDirs'].sort(key=lambda x: x['dir'])

        json_data = {}
        json_data.update({'template_name': template_name})
        json_data.update({'origin': f'The primary repo for {template_name} goes here: i.e. http://www.mydomain.com'})
        json_data.update({'license': f'What license {template_name} uses goes here: i.e. https://opensource.org/licenses/MIT'})
        json_data.update({'display_name': template_name})
        json_data.update({'summary': f"A short description of {template_name}."})
        json_data.update({'canonical_tags': []})
        json_data.update({'user_tags': [f'{template_name}']})
        json_data.update({'icon_path': "preview.png"})
        json_data.update({'copyFiles': restricted_platform_entries[restricted_platform]['copyFiles']})
        json_data.update({'createDirectories': restricted_platform_entries[restricted_platform]['createDirs']})

        json_name = f'{restricted_template_path}/{template_file_name}'
        os.makedirs(os.path.dirname(json_name), exist_ok=True)

        # if the json file we are about to write already exists for some reason, delete it
        if os.path.isfile(json_name):
            os.unlink(json_name)
        with open(json_name, 'w') as s:
            s.write(json.dumps(json_data, indent=4))

        preview_png_dst = f'{restricted_template_path}/Template/preview.png'
        if not os.path.isfile(preview_png_dst):
            shutil.copy(preview_png_src, preview_png_dst)

    return 0


def find_all_gem_templates(template_folder_list: list) -> list:
    """
    Find all subfolders in the given list of folders which appear to be gem templates
    :param template_folder_list: List of folders to search
    :return: list of tuples of TemplateName, AbsolutePath
    """
    return find_all_templates(template_folder_list, 'gem.json')


def find_all_project_templates(template_folder_list: list) -> list:
    """
    Find all subfolders in the given list of folders which appear to be project templates
    :param template_folder_list: List of folders to search
    :return: list of tuples of TemplateName, AbsolutePath
    """
    return find_all_templates(template_folder_list, 'project.json')


def find_all_templates(template_folder_list: list, template_marker_file: str) -> list:
    """
    Find all subfolders in the given list of folders which appear to be templates
    :param template_folder_list: List of folders to search
    :param template_marker_file: file expected in the template folder
    :return: list of tuples of TemplateName, AbsolutePath
    """
    templates_found = []
    for folder in template_folder_list:
        for root, dirs, files in os.walk(folder):
            for dir in dirs:
                if os.path.isfile(os.path.join(root, dir, template_file_name)) and \
                        os.path.isfile(os.path.join(root, dir, 'Template', template_marker_file)):
                    templates_found.append((dir, os.path.join(root, dir)))
            break # We only want root folders containing templates, do not recurse
    return templates_found


def create_from_template(dev_root: str,
                         destination_path: str,
                         template_path: str,
                         destination_restricted_path: str,
                         template_restricted_path: str,
                         destination_restricted_platform_relative_path: str,
                         template_restricted_platform_relative_path: str,
                         keep_restricted_in_instance: bool = False,
                         keep_license_text: bool = False,
                         replace: list = None) -> int:
    """
    Generic template instantiation.
    :param dev_root: the path to the dev root of the engine
    :param destination_path: the folder you want to put the instantiate template in, absolute or dev_root relative
    :param template_path: the name of the template you want to instance
    :param destination_restricted_path: path to the projects restricted folder
    :param template_restricted_path: path to the templates restricted folder
    :param keep_restricted_in_instance: whether or not you want ot keep the templates restricted files in your instance
     or separate them out into the restricted folder
    :param keep_license_text: whether or not you want ot keep the templates license text in your instance.
        template can have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE},
        this controls if you want to keep the license text from the template in the new instance. It is false by default
        because most customers will not want license text in there instances, but we may want to keep them.
    :param replace: optional list of strings uses to make concrete names out of templated parameters. X->Y pairs
        Ex. ${Name},TestGem,${Player},TestGemPlayer
        This will cause all references to ${Name} be replaced by TestGem, and all ${Player} replaced by 'TestGemPlayer'
    :return: 0 for success or non 0 failure code
    """
    # if no dev root error
    if not dev_root:
        logger.error('Dev root cannot be empty.')
        return 1
    dev_root = dev_root.replace('\\', '/')
    if not os.path.isabs(dev_root):
        dev_root = os.path.abspath(dev_root)
    if not os.path.isdir(dev_root):
        logger.error(f'Dev root {dev_root} is not a folder.')
        return 1

    # if no destination_path error
    if not destination_path:
        logger.error('Dst path cannot be empty.')
        return 1
    destination_path = destination_path.replace('\\', '/')
    if not os.path.isabs(destination_path):
        destination_path = f'{dev_root}/{destination_path}'

    # dst name is now the last component of the destination_path
    destination_name = os.path.basename(destination_path)

    # destination name cannot be the same as a restricted platform name
    if destination_name in restricted_platforms:
        logger.error(f'Destination path cannot be a restricted name. {destination_name}')
        return 1

    # if no template_path error
    if not template_path:
        logger.error('Template path cannot be empty.')
        return 1
    template_path = template_path.replace('\\', '/')
    if not os.path.isabs(template_path):
        template_path = f'{dev_root}/Templates/{template_path}'
    if not os.path.isdir(template_path):
        logger.error(f'Could not find the template {template_path}')
        return 1

    # template name is now the last component of the template_path
    template_name = os.path.basename(template_path)

    # destination_restricted_path
    destination_restricted_path = destination_restricted_path.replace('\\', '/')
    if not os.path.isabs(destination_restricted_path):
        destination_restricted_path = f'{dev_root}/{destination_restricted_path}'
    if not os.path.isdir(destination_restricted_path):
        os.makedirs(destination_restricted_path)

    # template_restricted_path
    template_restricted_path = template_restricted_path.replace('\\', '/')
    if not os.path.isabs(template_restricted_path):
        template_restricted_path = f'{dev_root}/{template_restricted_path}'
    if not os.path.isdir(template_restricted_path):
        logger.error(f'Template restricted path {template_restricted_path} is not a folder.')
        return 1

    # destination restricted relative
    destination_restricted_platform_relative_path = destination_restricted_platform_relative_path.replace('\\', '/')

    # template restricted relative
    template_restricted_platform_relative_path = template_restricted_platform_relative_path.replace('\\', '/')

    # any user supplied replacements
    replacements = list()
    while replace:
        replace_this = replace.pop(0)
        with_this = replace.pop(0)
        replacements.append((replace_this, with_this))

    # dst name is Name
    replacements.append(("${Name}", destination_name))
    replacements.append(("${NameUpper}", destination_name.upper()))
    replacements.append(("${NameLower}", destination_name.lower()))

    return _instantiate_template(destination_name,
                                 template_name,
                                 destination_path,
                                 template_path,
                                 template_restricted_path,
                                 replacements,
                                 keep_restricted_in_instance,
                                 keep_license_text)


def create_project(dev_root: str,
                   project_path: str,
                   template_path: str,
                   project_restricted_path: str = "restricted",
                   template_restricted_path: str = "restricted",
                   project_restricted_platform_relative_path: str or None = '',
                   template_restricted_platform_relative_path: str = "Templates",
                   keep_restricted_in_project: bool = False,
                   keep_license_text: bool = False,
                   replace: list = None,
                   system_component_class_id: str = None,
                   editor_system_component_class_id: str = None,
                   module_id: str = None) -> int:
    """
    Template instantiation that make all default assumptions for a Project template instantiation, reducing the effort
        needed in instancing a project
    :param dev_root: the path to the dev root of the engine
    :param project_path: the project path, can be absolute or dev_root relative
    :param template_path: the path to the template you want to instance, can be abs or relative,
     defaults to DefaultProject
    :param project_restricted_path: path to the projects restricted folder
    :param template_restricted_path: path to the templates restricted folder
    :param project_restricted_platform_relative_path: path to append to the project-restricted-path
    :param template_restricted_platform_relative_path: path to append to the template_restricted_path
    :param keep_restricted_in_project: whether or not you want ot keep the templates restricted files in your project or
     separate them out into the restricted folder
    :param keep_license_text: whether or not you want ot keep the templates license text in your instance.
        template can have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE},
        this controls if you want to keep the license text from the template in the new instance. It is false by default
        because most customers will not want license text in there instances, but we may want to keep them.
    :param replace: optional list of strings uses to make concrete names out of templated parameters. X->Y pairs
        Ex. ${Name},TestGem,${Player},TestGemPlayer
        This will cause all references to ${Name} be replaced by TestGem, and all ${Player} replaced by 'TestGemPlayer'
    :param system_component_class_id: optionally specify a uuid for the system component class, default is random uuid
    :param editor_system_component_class_id: optionally specify a uuid for the editor system component class, default is random uuid
    :param module_id: optionally specify a uuid for the module class, default is random uuid
    :return: 0 for success or non 0 failure code
    """
    # if no dev root error
    if not dev_root:
        logger.error('Dev root cannot be empty.')
        return 1
    dev_root = dev_root.replace('\\', '/')
    if not os.path.isabs(dev_root):
        dev_root = os.path.abspath(dev_root)
    if not os.path.isdir(dev_root):
        logger.error(f'Dev root {dev_root} is not a folder.')
        return 1

    if not template_path:
        logger.error('Template path cannot be empty.')
        return 1
    template_path = template_path.replace('\\', '/')
    if not os.path.isabs(template_path):
        template_path = f'{dev_root}/Templates/{template_path}'
    if not os.path.isdir(template_path):
        logger.error(f'Could not find the template {template_path}')
        return 1

    # template name is now the last component of the template_path
    template_name = os.path.basename(template_path)

    # project path
    if not project_path:
        logger.error('Project path cannot be empty.')
        return 1
    project_path = project_path.replace('\\', '/')
    if not os.path.isabs(project_path):
        project_path = f'{dev_root}/{project_path}'

    # project name is now the last component of the project_path
    project_name = os.path.basename(project_path)

    # project name cannot be the same as a restricted platform name
    if project_name in restricted_platforms:
        logger.error(f'Project path cannot be a restricted name. {project_name}')
        return 1

    # project_restricted_path
    project_restricted_path = project_restricted_path.replace('\\', '/')
    if not os.path.isabs(project_restricted_path):
        project_restricted_path = f'{dev_root}/{project_restricted_path}'
    if not os.path.isdir(project_restricted_path):
        os.makedirs(project_restricted_path)

    # template_restricted_path
    template_restricted_path = template_restricted_path.replace('\\', '/')
    if not os.path.isabs(template_restricted_path):
        template_restricted_path = f'{dev_root}/{template_restricted_path}'
    if not os.path.isdir(template_restricted_path):
        logger.error(f'Template restricted path {template_restricted_path} is not a folder.')
        return 1

    # project restricted relative
    project_restricted_platform_relative_path = project_restricted_platform_relative_path.replace('\\', '/')

    # template restricted relative
    template_restricted_platform_relative_path = template_restricted_platform_relative_path.replace('\\', '/')

    # any user supplied replacements
    replacements = list()
    while replace:
        replace_this = replace.pop(0)
        with_this = replace.pop(0)
        replacements.append((replace_this, with_this))

    # project name
    replacements.append(("${Name}", project_name))
    replacements.append(("${NameUpper}", project_name.upper()))
    replacements.append(("${NameLower}", project_name.lower()))

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
                f'Editor System component class id {editor_system_component_class_id} is malformed. Should look like Ex.' +
                '{b60c92eb-3139-454b-a917-a9d3c5819594}')
            return 1
        replacements.append(("${EditorSysCompClassId}", editor_system_component_class_id))
    else:
        replacements.append(("${EditorSysCompClassId}", '{' + str(uuid.uuid4()) + '}'))

    if _instantiate_template(project_name,
                             template_name,
                             project_path,
                             template_path,
                             project_restricted_path,
                             template_restricted_path,
                             project_restricted_platform_relative_path,
                             template_restricted_platform_relative_path,
                             replacements,
                             keep_restricted_in_project,
                             keep_license_text) == 0:

        # we created the project, now do anything extra that a project requires

        # If we do not keep the restricted folders in the project then we have to make sure
        # the restricted project folder has a CMakeLists.txt file in it so that when the restricted
        # folders CMakeLists.txt is executed it will find a CMakeLists.txt in the restricted project root
        if not keep_restricted_in_project:
            for restricted_platform in restricted_platforms:
                restricted_project = f'{project_restricted_path}/{restricted_platform}/{project_name}'
                os.makedirs(restricted_project, exist_ok=True)
                cmakelists_file_name = f'{restricted_project}/CMakeLists.txt'
                if not os.path.isfile(cmakelists_file_name):
                    with open(cmakelists_file_name, 'w') as d:
                        if keep_license_text:
                            d.write('# {BEGIN_LICENSE}\n')
                            d.write('# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or\n')
                            d.write('# its licensors.\n')
                            d.write('#\n')
                            d.write('# For complete copyright and license terms please see the LICENSE at the root of this\n')
                            d.write('# distribution (the "License"). All use of this software is governed by the License,\n')
                            d.write('# or, if provided, by the license below or the license accompanying this file. Do not\n')
                            d.write('# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,\n')
                            d.write('# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n')
                            d.write('# {END_LICENSE}\n')
    return 0


def create_gem(dev_root: str,
               gem_path: str,
               template_path: str,
               gem_restricted_path: str,
               template_restricted_path: str,
               gem_restricted_platform_relative_path: str or None,
               template_restricted_platform_relative_path: str,
               keep_restricted_in_gem: bool = False,
               keep_license_text: bool = False,
               replace: list = None,
               system_component_class_id: str = None,
               editor_system_component_class_id: str = None,
               module_id: str = None) -> int:
    """
    Template instantiation that make all default assumptions for a Gem template instantiation, reducing the effort
        needed in instancing a gem
    :param dev_root: the path to the dev root of the engine
    :param gem_path: the gem path, can be absolute or dev_root/Gems relative
    :param gem_restricted_path: the gem restricted path, can be absolute or dev_root/Gems relative
    :param template_path: the template path you want to instance, abs or dev_root/Templates relative,
     defaults to DefaultGem
    :param template_restricted_path: path to the templates restricted folder
    :param keep_restricted_in_gem: whether or not you want ot keep the templates restricted files in your instance or
     seperate them out into the restricted folder
    :param keep_license_text: whether or not you want ot keep the templates license text in your instance. template can
     have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE}, this controls if you want to keep
      the license text from the template in the new instance. It is false by default because most customers will not
       want license text in there instances, but we may want to keep them.
    :param replace: optional list of strings uses to make concrete names out of templated parameters. X->Y pairs
        Ex. ${Name},TestGem,${Player},TestGemPlayer
        This will cause all references to ${Name} be replaced by TestGem, and all ${Player} replaced by 'TestGemPlayer'
    :param system_component_class_id: optionally specify a uuid for the system component class, default is random uuid
    :param editor_system_component_class_id: optionally specify a uuid for the editor system component class, default is random uuid
    :param module_id: optionally specify a uuid for the module class, default is random uuid
    :return: 0 for success or non 0 failure code
    """
    # if no dev root error
    if not dev_root:
        logger.error('Dev root cannot be empty.')
        return 1
    dev_root = dev_root.replace('\\', '/')
    if not os.path.isabs(dev_root):
        dev_root = os.path.abspath(dev_root)
    if not os.path.isdir(dev_root):
        logger.error(f'Dev root {dev_root} is not a folder.')
        return 1

    # if no destination_path error
    if not gem_path:
        logger.error('Gem path cannot be empty.')
        return 1
    gem_path = gem_path.replace('\\', '/')
    if not os.path.isabs(gem_path):
        gem_path = f'{dev_root}/Gems/{gem_path}'

    # gem name is now the last component of the gem_path
    gem_name = os.path.basename(gem_path)

    # gem name cannot be the same as a restricted platform name
    if gem_name in restricted_platforms:
        logger.error(f'Gem path cannot be a restricted name. {gem_name}')
        return 1

    if not template_path:
        logger.error('Template path cannot be empty.')
        return 1
    template_path = template_path.replace('\\', '/')
    if not os.path.isabs(template_path):
        template_path = f'{dev_root}/Templates/{template_path}'
    if not os.path.isdir(template_path):
        logger.error(f'Could not find the template {template_path}')
        return 1

    # template name is now the last component of the template_path
    template_name = os.path.basename(template_path)

    # gem_restricted_path
    gem_restricted_path = gem_restricted_path.replace('\\', '/')
    if not os.path.isabs(gem_restricted_path):
        gem_restricted_path = f'{dev_root}/{gem_restricted_path}'
    if not os.path.isdir(gem_restricted_path):
        os.makedirs(gem_restricted_path)

    # template_restricted_path
    template_restricted_path = template_restricted_path.replace('\\', '/')
    if not os.path.isabs(template_restricted_path):
        template_restricted_path = f'{dev_root}/{template_restricted_path}'
    if not os.path.isdir(template_restricted_path):
        logger.error(f'Template restricted path {template_restricted_path} is not a folder.')
        return 1

    # gem restricted relative
    gem_restricted_platform_relative_path = gem_restricted_platform_relative_path.replace('\\', '/')

    # template restricted relative
    template_restricted_platform_relative_path = template_restricted_platform_relative_path.replace('\\', '/')

    # any user supplied replacements
    replacements = list()
    while replace:
        replace_this = replace.pop(0)
        with_this = replace.pop(0)
        replacements.append((replace_this, with_this))

    # gem name
    replacements.append(("${Name}", gem_name))
    replacements.append(("${NameUpper}", gem_name.upper()))
    replacements.append(("${NameLower}", gem_name.lower()))

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
                f'Editor System component class id {editor_system_component_class_id} is malformed. Should look like Ex.' +
                '{b60c92eb-3139-454b-a917-a9d3c5819594}')
            return 1
        replacements.append(("${EditorSysCompClassId}", editor_system_component_class_id))
    else:
        replacements.append(("${EditorSysCompClassId}", '{' + str(uuid.uuid4()) + '}'))

    return _instantiate_template(gem_name,
                                 template_name,
                                 gem_path,
                                 template_path,
                                 gem_restricted_path,
                                 template_restricted_path,
                                 gem_restricted_platform_relative_path,
                                 template_restricted_platform_relative_path,
                                 replacements,
                                 keep_restricted_in_gem,
                                 keep_license_text)


def _run_create_template(args: argparse) -> int:
    return create_template(common.determine_engine_root(),
                           args.source_path,
                           args.template_path,
                           args.source_restricted_path,
                           args.template_restricted_path,
                           args.source_restricted_platform_relative_path,
                           args.template_restricted_platform_relative_path,
                           args.keep_restricted_in_template,
                           args.keep_license_text,
                           args.replace)


def _run_create_from_template(args: argparse) -> int:
    return create_from_template(common.determine_engine_root(),
                                args.destination_path,
                                args.template_path,
                                args.destination_restricted_path,
                                args.template_restricted_path,
                                args.destination_restricted_platform_relative_path,
                                args.template_restricted_platform_relative_path,
                                args.keep_restricted_in_instance,
                                args.keep_license_text,
                                args.replace)


def _run_create_project(args: argparse) -> int:
    return create_project(common.determine_engine_root(),
                          args.project_path,
                          args.template_path,
                          args.project_restricted_path,
                          args.template_restricted_path,
                          args.project_restricted_platform_relative_path,
                          args.template_restricted_platform_relative_path,
                          args.keep_restricted_in_project,
                          args.keep_license_text,
                          args.replace,
                          args.system_component_class_id,
                          args.editor_system_component_class_id,
                          args.module_id)


def _run_create_gem(args: argparse) -> int:
    return create_gem(common.determine_engine_root(),
                      args.gem_path,
                      args.template_path,
                      args.gem_restricted_path,
                      args.template_restricted_path,
                      args.gem_restricted_platform_relative_path,
                      args.template_restricted_platform_relative_path,
                      args.keep_restricted_in_gem,
                      args.keep_license_text,
                      args.replace,
                      args.system_component_class_id,
                      args.editor_system_component_class_id,
                      args.module_id)


def add_args(parser, subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or aggregated by a central python file.
    Ex. Directly run from this file alone with: python engine_template.py create_gem --gem-path TestGem
    OR
    o3de.py can aggregate commands by importing engine_template,
    call add_args and execute: python o3de.py create_gem --gem-path TestGem
    :param parser: the caller instantiates a parser and passes it in here
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    # turn a directory into a template
    create_template_subparser = subparsers.add_parser('create-template')
    create_template_subparser.add_argument('-sp', '--source-path', type=str, required=True,
                                           help='The path to the source that you want to make into a template,'
                                                ' can be absolute or dev root relative.'
                                                'Ex. C:/o3de/Test'
                                                'Test = <source_name>')
    create_template_subparser.add_argument('-srp', '--source-restricted-path', type=str, required=False,
                                           default='restricted',
                                           help='The path to the src restricted folder if any to read from, can be'
                                                ' absolute or dev root relative, default is the dev root/restricted.')
    create_template_subparser.add_argument('-srprp', '--source-restricted-platform-relative-path', type=str, required=False,
                                           default='',
                                           help='Any path to append to the --source-restricted-path/<platform>'
                                                ' to where the restricted source is.'
                                                ' --source-restricted-path C:/restricted'
                                                ' --source-restricted-platform-relative-path some/folder'
                                                ' => C:/restricted/<platform>/some/folder/<source_name>')
    create_template_subparser.add_argument('-tp', '--template-path', type=str, required=False,
                                           help='The path to the template you wish to create,'
                                                ' can be absolute or dev root/Templates relative, default is'
                                                ' source_name which is the last component of source path'
                                                ' Ex. C:/o3de/TestTemplate'
                                                ' Test = <template_name>')
    create_template_subparser.add_argument('-trp', '--template-restricted-path', type=str, required=False,
                                           default='restricted',
                                           help='The path to where to put the templates restricted folders write to if'
                                                ' any, can be absolute or dev root relative, default is'
                                                ' dev root/restricted.')
    create_template_subparser.add_argument('-trprp', '--template-restricted-platform-relative-path', type=str,
                                            required=False,
                                            default='Templates',
                                            help='Any path to append to the --template-restricted-path/<platform>'
                                                 ' to where the restricted template source is.'
                                                 ' --template-restricted-path C:/restricted'
                                                 ' --template-restricted-platform-relative-path some/folder'
                                                 ' => C:/restricted/<platform>/some/folder/<template_name>')
    create_template_subparser.add_argument('-kr', '--keep-restricted-in-template', action='store_true',
                                           default=False,
                                           help='Should the template keep the restricted platforms in the template, or'
                                                ' create the restricted files in the restricted folder, default is'
                                                ' False')
    create_template_subparser.add_argument('-kl', '--keep-license-text', action='store_true',
                                           default=False,
                                           help='Should license text be kept in the instantiation,'
                                                ' default is False')
    create_template_subparser.add_argument('-r', '--replace', type=str, required=False,
                                           nargs='*',
                                           help='String that specifies A->B replacement pairs.'
                                                ' Ex. --replace CoolThing ${the_thing} 1723905 ${id}'
                                                ' Note: <TemplateName> is the last component of template_path'
                                                ' Note: <TemplateName> is automatically ${Name}'
                                                ' Note: <templatename> is automatically ${NameLower}'
                                                ' Note: <TEMPLATENAME> is automatically ${NameUpper}')
    create_template_subparser.set_defaults(func=_run_create_template)

    # creation from a template
    create_from_template_subparser = subparsers.add_parser('create-from-template')
    create_from_template_subparser.add_argument('-dp', '--destination-path', type=str, required=True,
                                                help='The path to where you want the template instantiated,'
                                                     ' can be absolute or dev root relative.'
                                                     'Ex. C:/o3de/Test'
                                                     'Test = <destination_name>')
    create_from_template_subparser.add_argument('-drp', '--destination-restricted-path', type=str, required=False,
                                                default='restricted',
                                                help='The path to the dst restricted folder to read from if any, can be'
                                                     ' absolute or dev root relative. default is dev root/restricted')
    create_from_template_subparser.add_argument('-drprp', '--destination-restricted-platform-relative-path', type=str, required=False,
                                                default='',
                                                help='Any path to append to the --destination-restricted-path/<platform>'
                                                    ' to where the restricted destination is.'
                                                    ' --destination-restricted-path C:/instance'
                                                    ' --destination-restricted-platform-relative-path some/folder'
                                                    ' => C:/instance/<platform>/some/folder/<destination_name>')
    create_from_template_subparser.add_argument('-tp', '--template-path', type=str, required=True,
                                                help='The path to the template you want to instantiate, can be absolute'
                                                     ' or dev root/Templates relative.'
                                                     'Ex. C:/o3de/Template/TestTemplate'
                                                     'TestTemplate = <template_name>')
    create_from_template_subparser.add_argument('-trp', '--template-restricted-path', type=str, required=False,
                                                default='restricted',
                                                help='The path to the template restricted folder to write to if any,'
                                                     ' can be absolute or dev root relative, default is'
                                                     ' dev root/restricted')
    create_from_template_subparser.add_argument('-trprp', '--template-restricted-platform-relative-path', type=str,
                                                required=False,
                                                default='Templates',
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
                                                help='Should license text be kept in the instantiation,'
                                                     ' default is False')
    create_from_template_subparser.add_argument('-r', '--replace', type=str, required=False,
                                                nargs='*',
                                                help='String that specifies A->B replacement pairs.'
                                                     ' Ex. --replace CoolThing ${the_thing} ${id} 1723905'
                                                     ' Note: <DestinationName> is the last component of destination_path'
                                                     ' Note: ${Name} is automatically <DestinationName>'
                                                     ' Note: ${NameLower} is automatically <destinationname>'
                                                     ' Note: ${NameUpper} is automatically <DESTINATIONNAME>')
    create_from_template_subparser.set_defaults(func=_run_create_from_template)

    # creation of a project from a template (like create from template but makes project assumptions)
    create_project_subparser = subparsers.add_parser('create-project')
    create_project_subparser.add_argument('-pp', '--project-path', type=str, required=True,
                                          help='The name of the project you wish to create from the template,'
                                               ' can be an absolute path or dev root relative.'
                                               ' Ex. C:/o3de/TestProject'
                                               ' TestProject = <project_name>')
    create_project_subparser.add_argument('-prp', '--project-restricted-path', type=str, required=False,
                                          default='restricted',
                                          help='The path to the project restricted folder to write to if any, can be'
                                               ' absolute or dev root relative, default is dev root/restricted')
    create_project_subparser.add_argument('-prprp', '--project-restricted-platform-relative-path', type=str,
                                          required=False,
                                          default='',
                                          help='Any path to append to the --project-restricted-path/<platform>'
                                               ' to where the restricted project is.'
                                               ' --project-restricted-path C:/restricted'
                                               ' --project-restricted-platform-relative-path some/folder'
                                               ' => C:/restricted/<platform>/some/folder/<project_name>')
    create_project_subparser.add_argument('-tp', '--template-path', type=str, required=False,
                                          default='DefaultProject',
                                          help='The path to the template you want to instantiate, can be absolute'
                                               ' or dev root/Templates relative, defaults to the DefaultProject.'
                                               ' Ex. C:/o3de/Template/TestTemplate'
                                               ' TestTemplate = <template_name>')
    create_project_subparser.add_argument('-trp', '--template-restricted-path', type=str, required=False,
                                          default='restricted',
                                          help='The path to the template restricted folder to read from if any, can be'
                                               ' absolute or dev root relative, default is dev root/restricted')
    create_project_subparser.add_argument('-trprp', '--template-restricted-platform-relative-path', type=str,
                                          required=False,
                                          default='Templates',
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
                                          help='Should license text be kept in the instantiation, default is False')
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
    create_project_subparser.add_argument('--system-component-class-id', type=utils.validate_uuid4, required=False,
                                          help='The uuid you want to associate with the system class component, default'
                                               ' is a random uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_project_subparser.add_argument('--editor-system-component-class-id', type=utils.validate_uuid4, required=False,
                                          help='The uuid you want to associate with the editor system class component, default'
                                               ' is a random uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_project_subparser.add_argument('--module-id', type=utils.validate_uuid4, required=False,
                                          help='The uuid you want to associate with the module, default is a random'
                                               ' uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_project_subparser.set_defaults(func=_run_create_project)

    # creation of a gem from a template (like create from template but makes gem assumptions)
    create_gem_subparser = subparsers.add_parser('create-gem')
    create_gem_subparser.add_argument('-gp', '--gem-path', type=str, required=True,
                                      help='The path to the gem you want to create, can be absolute or dev root/Gems'
                                           ' relative.'
                                           ' Ex. C:/o3de/TestGem'
                                           ' TestGem = <gem_name>')
    create_gem_subparser.add_argument('-grp', '--gem-restricted-path', type=str, required=False,
                                      default='restricted',
                                      help='The path to the gem restricted to write to folder if any, can be'
                                           'absolute or dev root relative, default is dev root/restricted.')
    create_gem_subparser.add_argument('-grprp', '--gem-restricted-platform-relative-path', type=str,
                                      required=False,
                                      default='Gems',
                                      help='Any path to append to the --gem-restricted-path/<platform>'
                                           ' to where the restricted template is.'
                                           ' --gem-restricted-path C:/restricted'
                                           ' --gem-restricted-platform-relative-path some/folder'
                                           ' => C:/restricted/<platform>/some/folder/<gem_name>')
    create_gem_subparser.add_argument('-tp', '--template-path', type=str, required=False,
                                      default='DefaultGem',
                                      help='The path to the template you want to instantiate, can be absolute or'
                                           ' relative to the dev root/Templates. Default is DefaultGem.')
    create_gem_subparser.add_argument('-trp', '--template-restricted-path', type=str, required=False,
                                      default='restricted',
                                      help='The path to the gem restricted folder to read from if any, can be'
                                           'absolute or dev root relative, default is dev root/restricted.')
    create_gem_subparser.add_argument('-trprp', '--template-restricted-platform-relative-path', type=str,
                                      required=False,
                                      default='Templates',
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
                                      help='Should license text be kept in the instantiation, default is False')
    create_gem_subparser.add_argument('--system-component-class-id', type=utils.validate_uuid4, required=False,
                                      help='The uuid you want to associate with the system class component, default'
                                           ' is a random uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_gem_subparser.add_argument('--editor-system-component-class-id', type=utils.validate_uuid4,
                                      required=False,
                                      help='The uuid you want to associate with the editor system class component, default'
                                           ' is a random uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_gem_subparser.add_argument('--module-id', type=utils.validate_uuid4, required=False,
                                      help='The uuid you want to associate with the gem module,'
                                           ' default is a random uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}')
    create_gem_subparser.set_defaults(func=_run_create_gem)


if __name__ == "__main__":
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers
    the_subparsers = the_parser.add_subparsers(help='sub-command help')

    # add args to the parser
    add_args(the_parser, the_subparsers)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args)

    # return
    sys.exit(ret)
