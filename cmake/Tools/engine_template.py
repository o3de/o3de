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
    'Xenia',
    'Provo',
    'Salem'
}


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

    if not keep_license_text:
        t_data = re.sub(r"(//|'''|#)\s*{BEGIN_LICENSE}((.|\n)*){END_LICENSE}\n", "", t_data, flags=re.DOTALL)

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
                           dev_root: str,
                           dst_path: str,
                           replacements: list,
                           keep_license_text: bool = False) -> None:
    """
    Internal function to instantiate a template by executing the templates json file data
    :param json_data: This is the data read directly from the template json
    :param dev_root: the abs dev root of the engine
    :param dst_path: the abs folder you want to put the instantiate template in, absolute or dev_root relative
    :param replacements: a list of tuples that will replace templates arguments with concrete names
    :param keep_license_text: whether or not you want ot keep the templates license text in your instance.
        template can have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE},
        this controls if you want to keep the license text from the template in the new instance. It is false by default
        because most customers will not want license text in there instances, but we may want to keep them.
    """
    # create dirs first
    # for each createDirectory entry, transform the folder name
    for create_directory in json_data['createDirectories']:
        # construct the new folder name
        new_dir = f"{dst_path}/{create_directory['outDir']}"
        # transform the folder name
        new_dir = _transform(new_dir, replacements, keep_license_text)
        # create the folder
        os.makedirs(new_dir, exist_ok=True)

    # for each copyFiles entry, _transformCopy the templated source file into a concrete instance file or
    # regular copy if not templated
    for copy_file in json_data['copyFiles']:
        # construct the input file name
        in_file = f"{dev_root}/{json_data['inputPath']}/{copy_file['inFile']}"
        # the file can be marked as optional, if it is and it does not exist skip
        if copy_file['isOptional'] and copy_file['isOptional'] == 'true':
            if not os.path.isfile(in_file):
                continue
        # construct the output file name
        out_file = f"{dst_path}/{copy_file['outFile']}"
        # transform the output file name
        out_file = _transform(out_file, replacements, keep_license_text)

        # if for some reason the output folder for this file was not created above do it now
        os.makedirs(os.path.dirname(out_file), exist_ok=True)

        # if templated _transformCopy the file, if not just copy it
        if copy_file['isTemplated']:
            _transform_copy(in_file, out_file, replacements, keep_license_text)
        else:
            shutil.copy(in_file, out_file)


def _instantiate_template(dev_root: str,
                          destination_path: str,
                          template_path: str, template_path_rel: str,
                          template_restricted_path: str,
                          replacements: list,
                          keep_license_text: bool = False) -> int:
    """
    Internal function to create a concrete instance from a template

    :param dev_root: the abs dev root of the engine
    :param destination_path: the folder you want to put the instantiate template in, absolute or dev_root relative
    :param template_path: the path of the template
    :param template_path_rel: the relative path of the template
    :param template_restricted_path: the path of the restricted template
    :param replacements: optional list of strings uses to make concrete names out of templated parameters. X->Y pairs
        Ex. ${Name},TestGem,${Player},TestGemPlayer
        This will cause all references to ${Name} be replaced by TestGem, and all ${Player} replaced by 'TestGemPlayer'
    :param keep_license_text: whether or not you want ot keep the templates license text in your instance.
        template can have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE},
        this controls if you want to keep the license text from the template in the new instance. It is false by default
        because most customers will not want license text in there instances, but we may want to keep them.
    :return: 0 for success or non 0 failure code
    """
    # make sure the template json is found
    template_json = f'{template_path}/Template.json'
    if not os.path.isfile(template_json):
        return 1

    # load the template json and execute it
    with open(template_json, 'r') as s:
        try:
            json_data = json.load(s)
        except Exception as e:
            logger.error(f'Failed to load {s}: ' + str(e))
            return 1
        _execute_template_json(json_data, dev_root, destination_path, replacements, keep_license_text)

    # execute restricted platform jsons if any
    if os.path.isdir(template_restricted_path):
        for restricted_platform in os.listdir(template_restricted_path):
            template_restricted_platform = f'{template_restricted_path}/{restricted_platform}'
            platform_json = f'{template_restricted_platform}/{template_path_rel}/Template.json'
            if os.path.isfile(platform_json):
                # load the template json and execute it
                with open(platform_json, 'r') as s:
                    try:
                        json_data = json.load(s)
                    except Exception as e:
                        logger.error(f'Failed to load {s}: ' + str(e))
                        return 1
                    restricted_dst_path = destination_path.replace(dev_root, template_restricted_platform)
                    _execute_template_json(json_data, dev_root, restricted_dst_path, replacements, keep_license_text)

    return 0


def create_template(dev_root: str,
                    source_path: str,
                    template_path: str,
                    source_restricted_path: str = 'restricted',
                    template_restricted_path: str = 'restricted',
                    replace: list = None,
                    keep_license_text: bool = False) -> int:
    """
    Create a generic template from a source directory using replacement

    :param dev_root: the path to dev root of the engine
    :param source_path: source folder, absolute or dev_root relative
    :param template_path: the path of the template to create, can be absolute or relative to dev root/Templates, if none
     then it will be the source_name
    :param source_restricted_path:
    :param template_restricted_path:
    :param replace: optional list of strings uses to make templated parameters out of concrete names. X->Y pairs
     Ex. TestGem,${Name},TestGemPlayer,${Player}
     This will cause all references to 'TestGem' be replaced by ${Name}, and all 'TestGemPlayer' replaced by ${Player}
     Note these replacements are executed in order, so if you have larger matches, do them first, i.e.
     TestGemPlayer,${Player},TestGem,${Name}
     TestGemPlayer will get matched first and become ${Player} and will not become ${Name}Player
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

    # source_path_rel is the dev root relative part of the source_path, if not in dev root then source_name
    if os.path.commonpath([dev_root, source_path]) == os.path.normpath(dev_root):
        source_path_rel = source_path.replace(f'{dev_root}/', '')
    else:
        source_path_rel = source_name

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

    # template_path_rel is the dev root relative part of the template_path, if not in dev root then template_name
    if os.path.commonpath([dev_root, template_path]) == os.path.normpath(dev_root):
        template_path_rel = template_path.replace(f'{dev_root}/', '')
    else:
        template_path_rel = template_name

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
        logger.error(f'Template restricted path {template_restricted_path} is not a folder.')
        return 1

    # template_restricted_rel is the dev root relative part of the template_restricted_path, if not
    # in dev root then template_name
    if os.path.commonpath([dev_root, template_restricted_path]) == os.path.normpath(dev_root):
        template_restricted_rel = template_restricted_path.replace(f'{dev_root}/', '')
    else:
        template_restricted_rel = template_name

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

        # we want to send back the transformed data and whether or not this file will
        # may require transformation when instantiated. So if the input data is not the
        # same as the output, then we transformed it which means there may be a transformation
        # needed to instance it. Or if we chose not to remove the license text then potentially it
        # may need to be transformed when instanced
        if s_data != t_data or '{BEGIN_LICENSE}' in t_data:
            return True, t_data
        else:
            return False, t_data

    def _transform_dir_into_copyfiles_and_createdirs(root_abs: str, path_abs: str = None) -> None:
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
            if '/Platform' in entry_abs:
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
                # with the platform name containing the this file
                if os.path.isfile(entry_abs) and found_platform not in restricted_platform_entries:
                    found_platform = ''

                # if we found a platform that is not yet in the restricted_platform_entries and it is a restricted
                # platform, then add add empty copyfiles and createDirs for this found restricted platform
                if found_platform not in restricted_platform_entries and found_platform in restricted_platforms:
                    restricted_platform_entries.update({found_platform: {'copyFiles': [], 'createDirs': []}})

            # Now if we found a platform and still have a found_platform which is a restricted platform
            # then transform the entry relative name into a dst relative entry name and dst abs entry.
            # if not then create a normal relative and abs dst entry name
            if platform and found_platform in restricted_platforms:
                _, destination_entry_rel = _transform_into_template(entry_rel)
                destination_entry_abs = f'{template_restricted_path}/{found_platform}/{template_path_rel}' \
                                f'/Template/{destination_entry_rel}'
            else:
                _, destination_entry_rel = _transform_into_template(entry_rel)
                destination_entry_abs = f'{template_path}/Template/{destination_entry_rel}'

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
                        "inFile": destination_entry_rel,
                        "outFile": destination_entry_rel,
                        "isTemplated": templated,
                        "isOptional": False
                    })
                else:
                    copy_files.append({
                        "inFile": destination_entry_rel,
                        "outFile": destination_entry_rel,
                        "isTemplated": templated,
                        "isOptional": False
                    })
            else:
                # if the folder was for a restricted platform add the entry to the restricted platform, otherwise add it
                # to the non restricted
                if platform and found_platform in restricted_platforms:
                    restricted_platform_entries[found_platform]['createDirs'].append({
                        "outDir": destination_entry_rel
                    })
                else:
                    create_dirs.append({
                        "outDir": destination_entry_rel
                    })

                # recurse using the same root and this folder
                _transform_dir_into_copyfiles_and_createdirs(root_abs, entry_abs)

    # when we run the transformation to create copyfiles, createdirs, any we find will go in here
    copy_files = []
    create_dirs = []

    # when we run the transformation any restricted platforms entries we find will go in here
    restricted_platform_entries = {}

    # Every project will have a unrestricted folder which is src_path_abs which MAY have restricted files in it, and
    # each project MAY have a any restricted folders which will only have restricted files in them. The process is the
    # same for all of them and the result will be a separation of all restricted files from unrestricted files. We do
    # this by running the transformation first over the src path abs and then on each restricted folder for this project
    # we find. This will effectively combine all sources then separates all the restricted.

    # run the transformation on the src, which may or may not have restricted files
    _transform_dir_into_copyfiles_and_createdirs(source_path)

    # every src may have a matching restricted folder per platform
    # run the transformation on each src restricted folder
    for restricted_platform in os.listdir(source_restricted_path):
        restricted_platform_src_path_abs = f'{source_restricted_path}/{restricted_platform}/{source_path_rel}'
        if os.path.isdir(restricted_platform_src_path_abs):
            _transform_dir_into_copyfiles_and_createdirs(restricted_platform_src_path_abs)

    # now we should have all our copyFiles and createDirs entries, so write out the Template json for the unrestricted
    # platforms all together
    input_path = f'{template_path_rel}/Template'
    json_data = {}
    json_data.update({'inputPath': input_path})
    json_data.update({'copyFiles': copy_files})
    json_data.update({'createDirectories': create_dirs})

    json_name = f'{template_path}/Template.json'

    # if the json file we are about to write already exists for some reason, delete it
    if os.path.isfile(json_name):
        os.unlink(json_name)
    with open(json_name, 'w') as s:
        s.write(json.dumps(json_data, indent=4))

    # now write out each restricted platform template json separately
    for restricted_platform in restricted_platform_entries:
        input_path = f'{template_restricted_rel}/{restricted_platform}/{template_path_rel}/Template'
        json_data = {}
        json_data.update({'inputPath': input_path})
        json_data.update({'copyFiles': restricted_platform_entries[restricted_platform]['copyFiles']})
        json_data.update({'createDirectories': restricted_platform_entries[restricted_platform]['createDirs']})

        json_name = f'{template_restricted_path}/{restricted_platform}/{template_path_rel}/Template.json'
        os.makedirs(os.path.dirname(json_name), exist_ok=True)

        # if the json file we are about to write already exists for some reason, delete it
        if os.path.isfile(json_name):
            os.unlink(json_name)
        with open(json_name, 'w') as s:
            s.write(json.dumps(json_data, indent=4))

    return 0


def create_from_template(dev_root: str,
                         destination_path: str,
                         template_path: str,
                         destination_restricted_path: str = 'restricted',
                         template_restricted_path: str = 'restricted',
                         keep_license_text: bool = False,
                         replace: list = None) -> int:
    """
    Generic template instantiation.
    :param dev_root: the path to the dev root of the engine
    :param destination_path: the folder you want to put the instantiate template in, absolute or dev_root relative
    :param template_path: the name of the template you want to instance
    :param destination_restricted_path:
    :param template_restricted_path:
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

    # template_path_rel is the dev root relative part of the template_path, if not in dev root then template_name
    if os.path.commonpath([dev_root, template_path]) == os.path.normpath(dev_root):
        template_path_rel = template_path.replace(f'{dev_root}/', '')
    else:
        template_path_rel = template_name

    # destination_restricted_path
    destination_restricted_path = destination_restricted_path.replace('\\', '/')
    if not os.path.isabs(destination_restricted_path):
        destination_restricted_path = f'{dev_root}/{destination_restricted_path}'
    if not os.path.isdir(destination_restricted_path):
        logger.error(f'Dst restricted path {destination_restricted_path} is not a folder.')
        return 1

    # template_restricted_path
    template_restricted_path = template_restricted_path.replace('\\', '/')
    if not os.path.isabs(template_restricted_path):
        template_restricted_path = f'{dev_root}/{template_restricted_path}'
    if not os.path.isdir(template_restricted_path):
        logger.error(f'Template restricted path {template_restricted_path} is not a folder.')
        return 1

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

    return _instantiate_template(dev_root,
                                 destination_path,
                                 template_path, template_path_rel,
                                 template_restricted_path,
                                 replacements,
                                 keep_license_text)


def create_project(dev_root: str,
                   project_path: str,
                   template_path: str = 'DefaultProject',
                   project_restricted_path: str = 'restricted',
                   template_restricted_path: str = 'restricted',
                   keep_license_text: bool = False,
                   replace: list = None,
                   system_component_class: str = None, system_component_class_id: str = None,
                   module_class: str = None, module_id: str = None) -> int:
    """
    Template instantiation that make all default assumptions for a Project template instantiation, reducing the effort
        needed in instancing a project
    :param dev_root: the path to the dev root of the engine
    :param project_path: the project path, can be absolute or dev_root relative
    :param template_path: the path to the template you want to instance, can be abs or relative,
     defaults to DefaultProject
    :param project_restricted_path:
    :param template_restricted_path:
    :param keep_license_text: whether or not you want ot keep the templates license text in your instance.
        template can have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE},
        this controls if you want to keep the license text from the template in the new instance. It is false by default
        because most customers will not want license text in there instances, but we may want to keep them.
    :param replace: optional list of strings uses to make concrete names out of templated parameters. X->Y pairs
        Ex. ${Name},TestGem,${Player},TestGemPlayer
        This will cause all references to ${Name} be replaced by TestGem, and all ${Player} replaced by 'TestGemPlayer'
    :param system_component_class: optionally give the system component a different name than project + SystemComponent
    :param system_component_class_id: optionally specify a uuid for the system component class, default is random uuid
    :param module_class: optionally give the module class a different name than project + Module
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

    # template_path_rel is the dev root relative part of the template_path, if not in dev root then template_name
    if os.path.commonpath([dev_root, template_path]) == os.path.normpath(dev_root):
        template_path_rel = template_path.replace(f'{dev_root}/', '')
    else:
        template_path_rel = template_name

    # project path
    if not project_path:
        logger.error('Project path cannot be empty.')
        return 1
    project_path = project_path.replace('\\', '/')
    if not os.path.isabs(project_path):
        project_path = f'{dev_root}/{project_path}'

    # project name is now the last component of the project_path
    project_name = os.path.basename(project_path)

    # project_restricted_path
    project_restricted_path = project_restricted_path.replace('\\', '/')
    if not os.path.isabs(project_restricted_path):
        project_restricted_path = f'{dev_root}/{project_restricted_path}'
    if not os.path.isdir(project_restricted_path):
        logger.error(f'Project restricted path {project_restricted_path} is not a folder.')
        return 1

    # template_restricted_path
    template_restricted_path = template_restricted_path.replace('\\', '/')
    if not os.path.isabs(template_restricted_path):
        template_restricted_path = f'{dev_root}/{template_restricted_path}'
    if not os.path.isdir(template_restricted_path):
        logger.error(f'Template restricted path {template_restricted_path} is not a folder.')
        return 1

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

    # gem summary
    replacements.append(("${GemSummary}", 'A short description of ' + project_name))

    # gem comment
    replacements.append(("${GemComment}", project_name))

    # gem namespace
    replacements.append(("${GemNamespace}", project_name))

    # gem bus
    replacements.append(("${GemBus}", project_name + 'Bus'))

    # gem requests
    replacements.append(("${GemRequests}", project_name + 'Requests'))

    # gem request bus
    replacements.append(("${GemRequestBus}", project_name + 'Bus'))

    # gem test
    replacements.append(("${GemTest}", project_name + 'Test'))

    # module class name
    if module_class:
        replacements.append(("${ModuleClass}", module_class))
    else:
        replacements.append(("${ModuleClass}", project_name + 'Module'))

    # module id is a uuid with { and -
    if module_id:
        replacements.append(("${ModuleClassId}", module_id))
    else:
        replacements.append(("${ModuleClassId}", '{' + str(uuid.uuid4()) + '}'))

    # system component class name
    if system_component_class:
        replacements.append(("${SysCompClass}", system_component_class))
    else:
        replacements.append(("${SysCompClass}", project_name + 'SystemComponent'))

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

    return _instantiate_template(dev_root,
                                 project_path,
                                 template_path, template_path_rel,
                                 template_restricted_path,
                                 replacements,
                                 keep_license_text)


def create_gem(dev_root: str,
               gem_path: str,
               template_path: str = 'DefaultGem',
               gem_restricted_path: str = 'restricted',
               template_restricted_path: str = 'restricted',
               keep_license_text: str = False,
               replace: list = None,
               gem_namespace: str = None, gem_bus: str = None,
               gem_requests: str = None, gem_requests_bus: str = None,
               gem_comment: str = None, gem_summary: str = None, gem_test: str = None,
               module_class: str = None, module_id: str = None) -> int:
    """
    Template instantiation that make all default assumptions for a Gem template instantiation, reducing the effort
        needed in instancing a gem
    :param dev_root: the path to the dev root of the engine
    :param gem_path: the gem path, can be absolute or dev_root/Gems relative
    :param gem_restricted_path: the gem restricted path, can be absolute or dev_root/Gems relative
    :param template_path: the template path you want to instance, abs or dev_root/Templates relative,
     defaults to DefaultGem
    :param template_restricted_path:
    :param keep_license_text: whether or not you want ot keep the templates license text in your instance. template can
     have license blocks starting with {BEGIN_LICENSE} and ending with {END_LICENSE}, this controls if you want to keep
      the license text from the template in the new instance. It is false by default because most customers will not
       want license text in there instances, but we may want to keep them.
    :param replace: optional list of strings uses to make concrete names out of templated parameters. X->Y pairs
        Ex. ${Name},TestGem,${Player},TestGemPlayer
        This will cause all references to ${Name} be replaced by TestGem, and all ${Player} replaced by 'TestGemPlayer'
    :param gem_namespace: optionally give the gem namespace a different name than project
    :param gem_bus: optionally give the system component a different name than project + Bus
    :param gem_requests: optionally give the system component a different name than project + Requests
    :param gem_requests_bus: optionally give the system component a different name than project + RequestsBus
    :param gem_comment: optionally give the system component a different name than project + Comment
    :param gem_summary: optionally give the system component a different name than project + Summary
    :param gem_test: optionally give the system component a different name than project + Test
    :param module_class: optionally give the module class a different name than project + Module
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

    # template_path_rel is the dev root relative part of the template_path, if not in dev root then template_name
    if os.path.commonpath([dev_root, template_path]) == os.path.normpath(dev_root):
        template_path_rel = template_path.replace(f'{dev_root}/', '')
    else:
        template_path_rel = template_name

    # gem_restricted_path
    gem_restricted_path = gem_restricted_path.replace('\\', '/')
    if not os.path.isabs(gem_restricted_path):
        gem_restricted_path = f'{dev_root}/{gem_restricted_path}'
    if not os.path.isdir(gem_restricted_path):
        logger.error(f'Gem restricted path {gem_restricted_path} is not a folder.')
        return 1

    # template_restricted_path
    template_restricted_path = template_restricted_path.replace('\\', '/')
    if not os.path.isabs(template_restricted_path):
        template_restricted_path = f'{dev_root}/{template_restricted_path}'
    if not os.path.isdir(template_restricted_path):
        logger.error(f'Template restricted path {template_restricted_path} is not a folder.')
        return 1

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

    # gem summary
    if gem_summary:
        replacements.append(("${GemSummary}", gem_summary))
    else:
        replacements.append(("${GemSummary}", 'A short description of ' + gem_name))

    # gem comment
    if gem_comment:
        replacements.append(("${GemComment}", gem_comment))
    else:
        replacements.append(("${GemComment}", gem_name))

    # gem namespace
    if gem_namespace:
        replacements.append(("${GemNamespace}", gem_namespace))
    else:
        replacements.append(("${GemNamespace}", gem_name))

    # gem bus
    if gem_bus:
        replacements.append(("${GemBus}", gem_bus))
    else:
        replacements.append(("${GemBus}", gem_name + 'Bus'))

    # gem requests
    if gem_requests:
        replacements.append(("${GemRequests}", gem_requests))
    else:
        replacements.append(("${GemRequests}", gem_name + 'Requests'))

    # gem request bus
    if gem_requests_bus:
        replacements.append(("${GemRequestBus}", gem_requests_bus))
    else:
        replacements.append(("${GemRequestBus}", gem_name + 'Bus'))

    # gem test
    if gem_test:
        replacements.append(("${GemTest}", gem_test))
    else:
        replacements.append(("${GemTest}", gem_name + 'Test'))

    # module class name
    if module_class:
        replacements.append(("${ModuleClass}", module_class))
    else:
        replacements.append(("${ModuleClass}", gem_name + 'Module'))

    # module id is a uuid with { and -
    if module_id:
        replacements.append(("${ModuleClassId}", module_id))
    else:
        replacements.append(("${ModuleClassId}", '{' + str(uuid.uuid4()) + '}'))

    return _instantiate_template(dev_root,
                                 gem_path,
                                 template_path, template_path_rel,
                                 template_restricted_path,
                                 replacements,
                                 keep_license_text)


def _run_create_template(args: argparse) -> int:
    return create_template(common.determine_dev_root(),
                           args.source_path,
                           args.template_path,
                           args.source_restricted_path,
                           args.template_restricted_path,
                           args.keep_license_text,
                           args.replace)


def _run_create_from_template(args: argparse) -> int:
    return create_from_template(common.determine_dev_root(),
                                args.destination_path,
                                args.template_path,
                                args.destination_restricted_path,
                                args.template_restricted_path,
                                args.keep_license_text, args.replace)


def _run_create_project(args: argparse) -> int:
    return create_project(common.determine_dev_root(),
                          args.project_path,
                          args.template_path,
                          args.project_restricted_path,
                          args.template_restricted_path,
                          args.keep_license_text,
                          args.replace,
                          args.system_component_class, args.system_component_class_id,
                          args.module_class, args.module_id)


def _run_create_gem(args: argparse) -> int:
    return create_gem(common.determine_dev_root(),
                      args.gem_path,
                      args.template_path,
                      args.gem_restricted_path,
                      args.template_restricted_path,
                      args.keep_license_text,
                      args.replace,
                      args.gem_namespace, args.gem_bus,
                      args.gem_requests, args.gem_requests_bus,
                      args.gem_comment, args.gem_summary, args.gem_test,
                      args.module_class, args.module_id)


def add_args(parser, subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or aggregated by a central python file.
    Ex. Directly run from this file alone with: python engine_template.py create_gem --gem-path TestGem
    OR
    lmbr.py can aggregate commands by importing engine_template,
    call add_args and execute: python lmbr.py create_gem --gem-path TestGem
    :param parser: the caller instantiates a parser and passes it in here
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    # turn a directory into a template
    create_template_subparser = subparsers.add_parser('create_template')
    create_template_subparser.add_argument('-sp', '--source-path', required=True,
                                           help='The path to the source that you want to make into a template,'
                                                ' can be absolute or dev root relative')
    create_template_subparser.add_argument('-srp', '--source-restricted-path', required=False,
                                           default='restricted',
                                           help='The path to the src restricted folder if any to read from, can be'
                                                ' absolute or dev root relative, default is the dev root/restricted.')
    create_template_subparser.add_argument('-tp', '--template-path', required=False,
                                           help='The path to the template you wish to create,'
                                                ' can be absolute or dev root/Templates relative, default is'
                                                ' source_name which is the last component of source path')
    create_template_subparser.add_argument('-trp', '--template-restricted-path', required=False,
                                           default='restricted',
                                           help='The path to where to put the templates restricted folders write to if'
                                                ' any, can be absolute or dev root relative, default is'
                                                ' dev root/restricted.')
    create_template_subparser.add_argument('-l', '--keep-license_text', required=False,
                                           default=False,
                                           help='Should license text be kept in the instantiation,'
                                                ' default is False')
    create_template_subparser.add_argument('-r', '--replace', required=False,
                                           nargs='*',
                                           help='String that specifies A->B replacement pairs.'
                                                ' Ex. --replace CoolThing ${the_thing} 1723905 ${id}'
                                                ' Note: <TemplateName> is the last component of template_path'
                                                ' Note: <TemplateName> is automatically ${Name}'
                                                ' Note: <templatename> is automatically ${NameLower}'
                                                ' Note: <TEMPLATENAME> is automatically ${NameUpper}')
    create_template_subparser.set_defaults(func=_run_create_template)

    # creation from a template
    create_from_template_subparser = subparsers.add_parser('create_from_template')
    create_from_template_subparser.add_argument('-dp', '--destination-path', required=True,
                                                help='The path to where you want the template instantiated,'
                                                     ' can be absolute or dev root relative.')
    create_from_template_subparser.add_argument('-drp', '--destination-restricted-path', required=False,
                                                default='restricted',
                                                help='The path to the dst restricted folder to read from if any, can be'
                                                     ' absolute or dev root relative. default is dev root/restricted')
    create_from_template_subparser.add_argument('-tp', '--template-path', required=True,
                                                help='The path to the template you want to instantiate, can be absolute'
                                                     ' or dev root/Templates relative.')
    create_from_template_subparser.add_argument('-trp', '--template-restricted-path', required=False,
                                                default='restricted',
                                                help='The path to the template restricted folder to write to if any,'
                                                     ' can be absolute or dev root relative, default is'
                                                     ' dev root/restricted')
    create_from_template_subparser.add_argument('-l', '--keep-license_text', required=False,
                                                default=False,
                                                help='Should license text be kept in the instantiation,'
                                                     ' default is False')
    create_from_template_subparser.add_argument('-r', '--replace', required=False,
                                                nargs='*',
                                                help='String that specifies A->B replacement pairs.'
                                                     ' Ex. --replace CoolThing ${the_thing} ${id} 1723905'
                                                     ' Note: <DestinationName> is the last component of destination_path'
                                                     ' Note: ${Name} is automatically <DestinationName>'
                                                     ' Note: ${NameLower} is automatically <destinationname>'
                                                     ' Note: ${NameUpper} is automatically <DESTINATIONNAME>')
    create_from_template_subparser.set_defaults(func=_run_create_from_template)

    # creation of a project from a template (like create from template but makes project assumptions)
    create_project_subparser = subparsers.add_parser('create_project')
    create_project_subparser.add_argument('-pp', '--project-path', required=True,
                                          help='The name of the project you wish to create from the template,'
                                               ' can be an absolute path or dev root relative.')
    create_project_subparser.add_argument('-prp', '--project-restricted-path', required=False,
                                          default='restricted',
                                          help='The path to the project restricted folder to write to if any, can be'
                                               ' absolute or dev root relative, default is dev root/restricted')
    create_project_subparser.add_argument('-tp', '--template-path', required=False,
                                          default='DefaultProject',
                                          help='The path to the template you want to instantiate, can be absolute'
                                               ' or dev root/Templates relative, defaults to the DefaultProject.')
    create_project_subparser.add_argument('-trp', '--template-restricted-path', required=False,
                                          default='restricted',
                                          help='The path to the template restricted folder to read from if any, can be'
                                               ' absolute or dev root relative, default is dev root/restricted')
    create_project_subparser.add_argument('-l', '--keep-license_text', required=False,
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
    create_project_subparser.add_argument('--system-component-class', required=False,
                                          help='The name you want to associate with the system class component, default'
                                               ' is project name + SystemComponent')
    create_project_subparser.add_argument('--system-component-class-id', required=False,
                                          help='The uuid you want to associate with the system class component, default'
                                               ' is a random uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}',
                                          type=utils.validate_uuid4)
    create_project_subparser.add_argument('--module-class', required=False,
                                          help='The name you want to associate with the module, default is a random'
                                               ' project name + Module')
    create_project_subparser.add_argument('--module-id', required=False,
                                          help='The uuid you want to associate with the module, default is a random'
                                               ' uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}',
                                          type=utils.validate_uuid4)
    create_project_subparser.set_defaults(func=_run_create_project)

    # creation of a gem from a template (like create from template but makes gem assumptions)
    create_gem_subparser = subparsers.add_parser('create_gem')
    create_gem_subparser.add_argument('-gp', '--gem-path', required=True,
                                      help='The path to the gem you want to create, can be absolute or dev root/Gems'
                                           ' relative.')
    create_gem_subparser.add_argument('-grp', '--gem-restricted-path', required=False,
                                      default='restricted',
                                      help='The path to the gem restricted to write to folder if any, can be'
                                           'absolute or dev root relative, default is dev root/restricted.')
    create_gem_subparser.add_argument('-tp', '--template-path', required=False,
                                      default='DefaultGem',
                                      help='The path to the template you want to instantiate, can be absolute or'
                                           ' relative to the dev root/Templates. Default is DefaultGem.')
    create_gem_subparser.add_argument('-trp', '--template-restricted-path', required=False,
                                      default='restricted',
                                      help='The path to the gem restricted folder to read from if any, can be'
                                           'absolute or dev root relative, default is dev root/restricted.')
    create_gem_subparser.add_argument('-r', '--replace', required=False,
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
    create_gem_subparser.add_argument('-l', '--keep-license_text', required=False,
                                      default=False,
                                      help='Should license text be kept in the instantiation, default is False')
    create_gem_subparser.add_argument('--gem-namespace', required=False,
                                      help='The namespace you want to associate with the gem, default is gem name.')
    create_gem_subparser.add_argument('--gem-bus', required=False,
                                      help='The name of the bus you want to associate with the gem,'
                                           ' default is gem name + Bus.')
    create_gem_subparser.add_argument('--gem-requests', required=False,
                                      help='The name of the requests you want to associate with the gem,'
                                           ' default is gem name + Requests.')
    create_gem_subparser.add_argument('--gem-requests_bus', required=False,
                                      help='The name of the requests bus you want to associate with the gem,'
                                           ' default is gem-requests + Bus')
    create_gem_subparser.add_argument('--gem-comment', required=False,
                                      help='The comment you want to associate with the gem,'
                                           ' default is gem name.')
    create_gem_subparser.add_argument('--gem-summary', required=False,
                                      help='The summary you want to associate with the gem,'
                                           ' default is "A short description of" + gem name.')
    create_gem_subparser.add_argument('--gem-test', required=False,
                                      help='The name you want to associate with the gem test,'
                                           ' default is gem name + Test.')
    create_gem_subparser.add_argument('--module-class', required=False,
                                      help='The name you want to associate with the gem module,'
                                           ' default is a gem name + Module.')
    create_gem_subparser.add_argument('--module-id', required=False,
                                      help='The uuid you want to associate with the gem module,'
                                           ' default is a random uuid Ex. {b60c92eb-3139-454b-a917-a9d3c5819594}',
                                      type=utils.validate_uuid4)
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
