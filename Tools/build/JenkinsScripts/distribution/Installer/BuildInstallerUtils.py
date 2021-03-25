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

import json
import os
import re
import shutil
import uuid
from urllib.parse import urlparse


download_url_base = "http://gamedev.amazon.com/lumberyard/releases/"


def set_download_url_base(url):
    global download_url_base
    download_url_base = url


def is_url(potential_url):
    return urlparse(potential_url)[0] == 'https'


def get_package_name(package_path):
    path_to_file = package_path
    if is_url(package_path):
        # index 2 is everything that isn't a parameter in the URL after the high level domain
        #   see https://docs.python.org/2/library/urlparse.html#urlparse.urlparse for more info
        path_to_file = urlparse(package_path)[2]
    return os.path.basename(path_to_file)


def get_ly_version_from_package(args, unpacked_location):
    version = None
    path_to_default_settings = os.path.join(unpacked_location, 'dev/_WAF_/default_settings.json')
    verbose_print(args.verbose, 'Searching for Lumberyard version in {}'.format(path_to_default_settings))
    with open(path_to_default_settings) as default_settings_file:
        default_settings_data = json.load(default_settings_file)
        default_settings_file.close()
        for build_option in default_settings_data['Build Options']:
            if 'attribute' in build_option and 'default_value' in build_option:
                if build_option['attribute'] == 'version':
                    version = build_option['default_value']

    if version is None:
        verbose_print(args.verbose, 'Version not found in package')
        raise Exception('Version was not available in default settings.json')

    return version


def append_trailing_slash_to_url(url):
    if not url.endswith(tuple(['/', '\\'])):
        url += '/'
    return url


def generate_target_url(base_target_url, version, build_id, suppress_version_in_path, append_build_id):
    output_url = base_target_url
    if append_build_id:
        output_url = append_trailing_slash_to_url(output_url)
        output_url += build_id
    if not suppress_version_in_path:
        output_url = append_trailing_slash_to_url(output_url)
        output_url += '{}/installer'.format(version)
    return output_url


# PRODUCT & UPGRADE/PATCH GUID CREATION
def create_id(name, seed, version, build_id):
    """
    Generate the Product GUID using the name, the version, and a changelist value.
    @param name - Name of the product.
    @param seed - String used to create a unique GUID.
    @param version - The version of the product in the form "PRODUCT.MAJOR.MINOR.PATCH".
    @param build_id - An optional identifier for the build to be used in GUID generation.
    @return - A GUID for this version of the product.
    """
    # Temporary. Replace the download_url_base with wherever we pass in to the public host or host url.
    uuid_seed = download_url_base + seed + name + version
    if build_id is not None:
        uuid_seed += build_id
    return str(uuid.uuid3(uuid.NAMESPACE_URL, uuid_seed)).upper()
# END PRODUCT & UPGRADE/PATCH GUID CREATION


def replace_leading_numbers(source_string):
    pattern = re.compile(r'^[0-9]')
    return pattern.sub('N', source_string)


def strip_special_characters(source_string):
    """
    Remove all non-alphanumeric characters from the given sourceString.
    @return - A new string that is a copy of the original, containing only
        letters and numbers.
    """
    if source_string.isalnum():
        return source_string

    pattern = re.compile(r'[\W_]+')
    return pattern.sub('', source_string)


def check_for_empty_subfolders(package_root, allowed_empty_folders):
    # find empty folders
    empty_folders_found = []
    for root, dirs, files in os.walk(package_root):
        if dirs == [] and files == []:
            empty_folders_found.append(os.path.normpath(os.path.relpath(root, package_root)))
    if allowed_empty_folders is not None:
        whitelist_root = 'Whitelist'
        whitelist_folders = []
        assert (os.path.exists(allowed_empty_folders)), 'The whitelist file specified at {} does not exist.'.format(allowed_empty_folders)
        with open(allowed_empty_folders, 'r') as source:
            json_whitelist = json.load(source)
            try:
                for allowed_folder in json_whitelist[whitelist_root]:
                    whitelist_folders.append(os.path.normpath(allowed_folder))
            except KeyError:
                print('Unknown json root {}, please check the json root specified.'.format(whitelist_root))
                exit(1)
        assert (len(whitelist_folders) > 0), 'The whitelist in the file specified at {} is empty. Either populate the list or omit the argument.'.format(allowed_empty_folders)
        for folder in empty_folders_found:
            assert (folder in whitelist_folders), 'The empty folder {} could not be found in the whitelist of empty folders.'.format(folder)


def get_immediate_subdirectories(root_dir):
    """
    Create a list of all directories that exist in the given directory, without
        recursing through the subdirectories' children.
    @param root_dir - The directory to search for subdirectories.
    @return - A list of subdirectories in this directory, excluding their children.
    """
    directories = []
    for directory in os.listdir(root_dir):
        if os.path.isdir(os.path.join(root_dir, directory)):
            directories.append(directory)

    return directories


def get_file_names_in_directory(directory, file_extension=None):
    """
    Create a list of all files in a directory. If given a file extension, it
        will list all files with that extension.
    @param directory - The directory to gather files from.
    @param file_extension - (Optional) The type of files to find. Must be a
        string in the ".extension" format. (Default None)
    @retun - A list of names of all files in this directory (that match the given
        extension if provided).
    """
    file_list = []
    if file_extension:
        for file in os.listdir(directory):
            if file.endswith(file_extension):
                file_list.append(os.path.basename(file))
    else:
        for file in os.listdir(directory):
            file_list.append(os.path.basename(file))

    return file_list


# VERBOSE RELATED FUNCTIONS

def verbose_print(isVerbose, message):
    if isVerbose:
        print(message)


def find_file_in_package(packageRoot, fileToFind, pathFilters=None):
    """
    Searchs the package path for a file.
    @param packageRoot: Path to the root of content.
    @param fileToFind: Name of the file to find.
    @param pathFilters: Path filter to apply to find.
    @return: The full path to the file if found, otherwise None.
    """
    for root, dirs, files in os.walk(packageRoot):
        if fileToFind in files:
            if pathFilters is None or any(pathFilter in root for pathFilter in pathFilters):
                return os.path.join(root, fileToFind)
    return None


def safe_shutil_file_copy(src, dst):
    # need to remove the old version of dst if it already exists due to a bug
    #   in shutil.copy that causes both the src and dst files to be zeroed out
    #   if they are identical files.
    if os.path.exists(dst):
        os.remove(dst)
    shutil.copy(src, dst)


def create_version_file(buildId, installerPath):
    with open(os.path.join(installerPath, 'version.txt'), 'w') as versionFile:
        versionFile.write(buildId)
