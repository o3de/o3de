"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Small library of functions to support autotests for asset processor

"""

import os
import shutil
import re
import ly_test_tools.environment.file_system as fs
import hashlib
import shutil
import logging

logger = logging.getLogger(__name__)

def compare_assets_with_cache(assets, assets_cache_path):
    """
    Given a list of assets names, will try to find them (disrespecting file extensions) from project's Cache folder with test assets
    :param assets: A list of assets to be compared with Cache
    :param assets_cache_path: A path to cache test assets folder
    :return: A tuple with two lists - first is missing in cache assets, second is existing in cache assets
    """
    missing_assets = []
    existing_assets = []
    if os.path.exists(assets_cache_path):
        files_in_cache = list(map(fs.remove_path_and_extension, os.listdir(assets_cache_path)))
        for asset in assets:
            file_without_ext = fs.remove_path_and_extension(asset).lower()
            if file_without_ext in files_in_cache:
                existing_assets.append(file_without_ext)
                files_in_cache.remove(file_without_ext)
            else:
                missing_assets.append(file_without_ext)
    else:
        missing_assets = assets
    return missing_assets, existing_assets


def copy_assets_to_project(assets, source_directory, target_asset_dir):
    """
    Given a list of asset names and a directory, copy those assets into the target project directory
    :param assets: A list of asset names to be copied
    :param source_directory: A path string where assets are located
    :param target_asset_dir: A path to project tests assets directory where assets will be copied over to
    :return: None
    """
    for asset in assets:
        full_name = os.path.join(source_directory, asset)
        shutil.copy(full_name, target_asset_dir)


def prepare_test_assets(assets_path, function_name, project_test_assets_dir):
    """
    Given function name and assets cache path, will clear cache and copy test assets assigned to function name to project's folder
    :param assets_path: Path to tests assets folder
    :param function_name: Name of a function that corresponds to folder with assets
    :param project_test_assets_dir: A path to project directory with test assets
    :return: Returning path to copied assets folder
    """
    test_assets_folder = os.path.join(assets_path, 'assets', function_name)
    copy_assets_to_project(os.listdir(test_assets_folder), test_assets_folder, project_test_assets_dir)
    return test_assets_folder


def find_joblog_file(joblogs_path, regexp):
    """
    Given path to joblogs files and asset name in form of regexp, will try to find joblog file for provided asset; if multiple - will return first occurrence
    :param joblogs_path: Path to a folder with joblogs files to look for needed file
    :param regexp: Python Regexp containing name of the asset that was processed, for which we're looking joblog file for
    :return: Full path to joblog file, empty string if not found
    """
    for file_name in os.listdir(joblogs_path):
        if re.match(regexp, file_name):
            return os.path.join(joblogs_path, file_name)
    return ''


def find_missing_lines_in_joblog(joblog_location, strings_to_verify):
    """
    Given joblog file full path and list of strings to verify, will find all missing strings in the file
    :param joblog_location: Full path to joblog file
    :param strings_to_verify: List of string to look for in joblog file
    :return: Subset of original strings list, that were not found in the file
    """
    lines_not_found = []
    with open(joblog_location, 'r') as f:
        read_data = f.read()
    for line in strings_to_verify:
        if line not in read_data:
            lines_not_found.append(line)
    return lines_not_found


def clear_project_test_assets_dir(test_assets_dir):
    """
    On call - deletes test assets dir if it exists and creates new empty one
    :param test_assets_dir: A path to tests assets dir
    :return: None
    """
    if os.path.exists(test_assets_dir):
        fs.delete([test_assets_dir], True, True)
    os.mkdir(test_assets_dir)


def get_files_hashsum(path_to_files_dir):
    """
    On call - calculates md5 hashsums for filecontents.
    :param path_to_files_dir: A path to files directory
    :return: Returns a dict with initial filenames from path_to_files_dir as keys and their contents hashsums as values
    """
    checksum_dict = {}
    try:
        for fname in os.listdir(path_to_files_dir):
            with open(os.path.join(path_to_files_dir, fname), 'rb') as fopen:
                checksum_dict[fname] = hashlib.sha256(fopen.read()).digest()
    except IOError:
        logger.error('An error occured trying to read file')
    return checksum_dict


def append_to_filename(file_name, path_to_file, append_text, ignore_extension):
    """
    Function for appending text to file and folder names
    :param file_name: Name of a file or folder
    :param path_to_file: Path to file or folder
    :param append_text: Text to append
    :param ignore_extension: True or False for ignoring extensions
    :return: None
    """
    new_name = ''
    if not ignore_extension:
        (name, extension) = file_name.split('.')
        new_name = name + append_text + '.' + extension
    else:
        new_name = file_name + append_text
    os.rename(os.path.join(path_to_file, file_name), os.path.join(path_to_file, new_name))


def create_asset_processor_backup_directories(backup_root_directory, test_backup_directory):
    """
    Function for creating the asset processor logs backup directory structure
    :param backup_root_directory: The location where logs should be stored
    :param test_backup_directory: The directory for the specific test being ran
    :return: None
    """
    if not os.path.exists(os.path.join(backup_root_directory, test_backup_directory)):
        os.makedirs(os.path.join(backup_root_directory, test_backup_directory))


def backup_asset_processor_logs(bin_directory, backup_directory):
    """
    Function for backing up the logs created by asset processor to designated backup directory
    :param bin_directory: The bin directory created by the lumberyard build process
    :param backup_directory: The location where asset processor logs should be backed up to
    :return: None
    """
    ap_logs = os.path.join(bin_directory, 'logs')

    if os.path.exists(ap_logs):
        destination = os.path.join(backup_directory, 'logs')
        shutil.copytree(ap_logs, destination)
