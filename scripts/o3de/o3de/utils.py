#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
This file contains utility functions
"""
import argparse
import sys
import uuid
import os
import pathlib
import shutil
import urllib.request
import logging
import zipfile
import re
from packaging.version import Version
from packaging.specifiers import SpecifierSet

from o3de import gitproviderinterface, github_utils

LOG_FORMAT = '[%(levelname)s] %(name)s: %(message)s'

logger = logging.getLogger('o3de.utils')
logging.basicConfig(format=LOG_FORMAT)

COPY_BUFSIZE = 64 * 1024


class VerbosityAction(argparse.Action):
    def __init__(self,
                 option_strings,
                 dest,
                 default=None,
                 required=False,
                 help=None):
        super().__init__(
            option_strings=option_strings,
            dest=dest,
            nargs=0,
            default=default,
            required=required,
            help=help,
        )

    def __call__(self, parser, namespace, values, option_string=None):
        count = getattr(namespace, self.dest, None)
        if count is None:
            count = self.default
        count += 1
        setattr(namespace, self.dest, count)
        # get the parent logger instance
        log = logging.getLogger('o3de')
        if count >= 2:
            log.setLevel(logging.DEBUG)
        elif count == 1:
            log.setLevel(logging.INFO)


def add_verbosity_arg(parser: argparse.ArgumentParser) -> None:
    """
    Add a consistent/common verbosity option to an arg parser
    :param parser: The ArgumentParser to modify
    :return: None
    """
    parser.add_argument('-v', dest='verbosity', action=VerbosityAction, default=0,
                        help='Additional logging verbosity, can be -v or -vv')


def copyfileobj(fsrc, fdst, callback, length=0):
    # This is functionally the same as the python shutil copyfileobj but
    # allows for a callback to return the download progress in blocks and allows
    # to early out to cancel the copy.
    if not length:
        length = COPY_BUFSIZE

    fsrc_read = fsrc.read
    fdst_write = fdst.write

    copied = 0
    while True:
        buf = fsrc_read(length)
        if not buf:
            break
        fdst_write(buf)
        copied += len(buf)
        if callback(copied):
            return 1
    return 0


def validate_identifier(identifier: str) -> bool:
    """
    Determine if the identifier supplied is valid
    :param identifier: the name which needs to be checked
    :return: bool: if the identifier is valid or not
    """
    if not identifier:
        return False
    elif len(identifier) > 64:
        return False
    elif not identifier[0].isalpha():
        return False
    else:
        for character in identifier:
            if not (character.isalnum() or character == '_' or character == '-'):
                return False
    return True


def validate_version_specifier(version_specifier:str) -> bool:
    try:
        get_object_name_and_version_specifier(version_specifier)
    except (InvalidObjectNameException, InvalidVersionSpecifierException):
        return False
    return True 


def validate_version_specifier_list(version_specifiers:str or list) -> bool:
    version_specifier_list = version_specifiers.split() if isinstance(version_specifiers, str) else version_specifiers
    if not isinstance(version_specifier_list, list):
        logger.error(f'Version specifiers must be in the format <name><version specifiers>. e.g. name==1.2.3 \n {version_specifiers}')
        return False

    for version_specifier in version_specifier_list:
        if not validate_version_specifier(version_specifier):
            logger.error(f'Version specifiers must be in the format <name><version specifiers>. e.g. name==1.2.3 \n {version_specifier}')
            return False
    return True


def sanitize_identifier_for_cpp(identifier: str) -> str:
    """
    Convert the provided identifier to a valid C++ identifier
    :param identifier: the name which needs to be sanitized
    :return: str: sanitized identifier
    """
    if not identifier:
        return ''
    
    sanitized_identifier = list(identifier)
    for index, character in enumerate(sanitized_identifier):
        if not (character.isalnum() or character == '_'):
            sanitized_identifier[index] = '_'
            
    return "".join(sanitized_identifier)


def validate_uuid4(uuid_string: str) -> bool:
    """
    Determine if the uuid supplied is valid
    :param uuid_string: the uuid which needs to be checked
    :return: bool: if the uuid is valid or not
    """
    try:
        val = uuid.UUID(uuid_string, version=4)
    except ValueError:
        return False
    return str(val) == uuid_string


def backup_file(file_name: str or pathlib.Path) -> None:
    index = 0
    renamed = False
    while not renamed:
        backup_file_name = pathlib.Path(str(file_name) + '.bak' + str(index)).resolve()
        index += 1
        if not backup_file_name.is_file():
            file_name = pathlib.Path(file_name).resolve()
            file_name.rename(backup_file_name)
            if backup_file_name.is_file():
                renamed = True


def backup_folder(folder: str or pathlib.Path) -> None:
    index = 0
    renamed = False
    while not renamed:
        backup_folder_name = pathlib.Path(str(folder) + '.bak' + str(index)).resolve()
        index += 1
        if not backup_folder_name.is_dir():
            folder = pathlib.Path(folder).resolve()
            folder.rename(backup_folder_name)
            if backup_folder_name.is_dir():
                renamed = True


def get_git_provider(parsed_uri):
    """
    Returns a git provider if one exists given the passed uri
    :param parsed_uri: uniform resource identifier of a possible git repository
    :return: A git provider implementation providing functions to get infomration about or clone a repository, see gitproviderinterface
    """
    return github_utils.get_github_provider(parsed_uri)


def download_file(parsed_uri, download_path: pathlib.Path, force_overwrite: bool = False, object_name: str = "", download_progress_callback = None) -> int:
    """
    Download file
    :param parsed_uri: uniform resource identifier to zip file to download
    :param download_path: location path on disk to download file
    :param force_overwrite: force overwrites the local file if one exists
    :param object_name: name of the object being downloaded
    :param download_progress_callback: callback called with the download progress as a percentage, returns true to request to cancel the download
    """
    file_exists = False
    if download_path.is_file():
        if not force_overwrite:
            file_exists = True
        else:
            try:
                os.unlink(download_path)
            except OSError:
                logger.error(f'Could not remove existing download path {download_path}.')
                return 1

    if parsed_uri.scheme in ['http', 'https', 'ftp', 'ftps']:
        try:
            current_request = urllib.request.Request(parsed_uri.geturl())
            resume_position = 0
            if not force_overwrite:
                if file_exists:
                    resume_position = os.path.getsize(download_path)
                    current_request.add_header("If-Range", "bytes=%d-" % resume_position)
            with urllib.request.urlopen(current_request) as s:
                download_file_size = 0
                try:
                    download_file_size = s.headers['content-length']
                except KeyError:
                    pass

                # if the server does not return a content length we also have to assume we would be replacing a complete file
                if file_exists and (resume_position == int(download_file_size) or int(download_file_size) == 0) and not force_overwrite:
                    logger.error(f'File already downloaded to {download_path} and force_overwrite is not set.')
                    return 1

                if s.getcode() == 206: # partial content
                    file_mode = 'ab'
                elif s.getcode() == 200:
                    file_mode = 'wb'
                else:
                    logger.error(f'HTTP status {e.code} opening {parsed_uri.geturl()}')
                    return 1

                def print_progress(downloaded, total_size):
                    end_ch = '\r'
                    if total_size == 0 or downloaded > total_size:
                        print(f'Downloading {object_name} - {downloaded} bytes')
                    else:
                        if downloaded == total_size:
                            end_ch = '\n'
                        print(f'Downloading {object_name} - {downloaded} of {total_size} bytes - {(downloaded/total_size)*100:.2f}%', end=end_ch)

                if download_progress_callback == None:
                    download_progress_callback = print_progress

                def download_progress(downloaded_bytes):
                    if download_progress_callback:
                        return download_progress_callback(int(downloaded_bytes), int(download_file_size))
                    return False

                with download_path.open(file_mode) as f:
                    download_cancelled = copyfileobj(s, f, download_progress)
                    if download_cancelled:
                        logger.info(f'Download of file to {download_path} cancelled.')
                        return 1
        except urllib.error.HTTPError as e:
            logger.error(f'HTTP Error {e.code} opening {parsed_uri.geturl()}')
            return 1
        except urllib.error.URLError as e:
            logger.error(f'URL Error {e.reason} opening {parsed_uri.geturl()}')
            return 1
    else:
        origin_file = pathlib.Path(parsed_uri.geturl()).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_path)

    return 0


def download_zip_file(parsed_uri, download_zip_path: pathlib.Path, force_overwrite: bool, object_name: str, download_progress_callback = None) -> int:
    """
    :param parsed_uri: uniform resource identifier to zip file to download
    :param download_zip_path: path to output zip file
    :param force_overwrite: force overwrites the local file if one exists
    :param object_name: name of the object being downloaded
    :param download_progress_callback: callback called with the download progress as a percentage, returns true to request to cancel the download
    """
    download_file_result = download_file(parsed_uri, download_zip_path, force_overwrite, object_name, download_progress_callback)
    if download_file_result != 0:
        return download_file_result

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"File zip {download_zip_path} is invalid. Try re-downloading the file.")
        download_zip_path.unlink()
        return 1

    return 0


def find_ancestor_file(target_file_name: pathlib.PurePath, start_path: pathlib.Path,
                       max_scan_up_range: int=0) -> pathlib.Path or None:
    """
    Find a file with the given name in the ancestor directories by walking up the starting path until the file is found
    :param target_file_name: Name of the file to find
    :param start_path: path to start looking for the file
    :param max_scan_up_range: maximum number of directories to scan upwards when searching for target file
           if the value is 0, then there is no max
    :return: Path to the file or None if not found
    """
    current_path = pathlib.Path(start_path)
    candidate_path = current_path / target_file_name

    max_scan_up_range = max_scan_up_range if max_scan_up_range else sys.maxsize

    # Limit the number of directories to traverse, to avoid infinite loop in path cycles
    for _ in range(max_scan_up_range):
        if candidate_path.exists():
            # Found the file we wanted
            break

        parent_path = current_path.parent
        if parent_path == current_path:
            # Only true when we are at the directory root, can't keep searching
            break
        candidate_path = parent_path / target_file_name
        current_path = parent_path

    return candidate_path if candidate_path.exists() else None


def find_ancestor_dir_containing_file(target_file_name: pathlib.PurePath, start_path: pathlib.Path,
                                      max_scan_up_range: int=0) -> pathlib.Path or None:
    """
    Find the nearest ancestor directory that contains the file with the given name by walking up
    from the starting path
    :param target_file_name: Name of the file to find
    :param start_path: path to start looking for the file
    :param max_scan_up_range: maximum number of directories to scan upwards when searching for target file
           if the value is 0, then there is no max
    :return: Path to the directory containing file or None if not found
    """
    ancestor_file = find_ancestor_file(target_file_name, start_path, max_scan_up_range)
    return ancestor_file.parent if ancestor_file else None


def get_gem_names_set(gems: list) -> set:
    """
    For working with the 'gem_names' lists in project.json
    Returns a set of gem names in a list of gems
    :param gems: The original list of gems, strings or small dicts (json objects)
    :return: A set of gem name strings
    """
    return set([gem['name'] if isinstance(gem, dict) else gem for gem in gems])


def remove_gem_duplicates(gems: list) -> list:
    """
    For working with the 'gem_names' lists in project.json
    Adds names to a dict, and when a collision occurs, eject the existing one in favor of the new one.
    This is because when adding gems the list is extended, so the override will come last.
    :param gems: The original list of gems, strings or small dicts (json objects)
    :return: A new list with duplicate gem entries removed
    """
    new_list = []
    names = {}
    for gem in gems:
        if not (isinstance(gem, dict) or isinstance(gem, str)):
            continue
        gem_name = gem.get('name', '') if isinstance(gem, dict) else gem
        if gem_name:
            if gem_name not in names:
                names[gem_name] = len(new_list)
                new_list.append(gem)
            else:
                new_list[names[gem_name]] = gem
    return new_list


def update_keys_and_values_in_dict(existing_values: dict, new_values: list or str, remove_values: list or str,
                      replace_values: list or str):
    """
    Updates values within a dictionary by replacing all values or by appending values in the new_values list and 
    removing values in the remove_values
    :param existing_values dict to modify
    :param new_values list of key=value pairs to add to the existing dictionary 
    :param remove_values list with keys to remove from the existing dictionary 
    :param replace_values list with key=value pairs to replace existing dictionary with

    returns updated existing dictionary
    """
    if replace_values != None:
        replace_values = replace_values.split() if isinstance(replace_values, str) else replace_values
        return dict(entry.split('=') for entry in replace_values if '=' in entry)
    
    if new_values:
        new_values = new_values.split() if isinstance(new_values, str) else new_values
        new_values = dict(entry.split('=') for entry in new_values if '=' in entry)
        if new_values:
            existing_values.update(new_values)
    
    if remove_values:
        remove_values = remove_values.split() if isinstance(remove_values, str) else remove_values
        [existing_values.pop(key) for key in remove_values]
    
    return existing_values


def update_values_in_key_list(existing_values: list, new_values: list or str, remove_values: list or str,
                      replace_values: list or str):
    """
    Updates values within a list by replacing all values or by appending values in the new_values list, 
    removing values in the remove_values and then removing duplicates.
    :param existing_values list with existing values to modify
    :param new_values list with values to add to the existing value list
    :param remove_values list with values to remove from the existing value list
    :param replace_values list with values to replace in the existing value list

    returns updated existing value list
    """
    if replace_values != None:
        replace_values = replace_values.split() if isinstance(replace_values, str) else replace_values
        return list(dict.fromkeys(replace_values))

    if new_values:
        new_values = new_values.split() if isinstance(new_values, str) else new_values
        existing_values.extend(new_values)
    if remove_values:
        remove_values = remove_values.split() if isinstance(remove_values, str) else remove_values
        existing_values = list(filter(lambda value: value not in remove_values, existing_values))

    # replace duplicate values
    return list(dict.fromkeys(existing_values))


class InvalidVersionSpecifierException(Exception):
    pass


class InvalidObjectNameException(Exception):
    pass


def get_object_name_and_version_specifier(input:str) -> (str, str) or None:
    """
    Get the object name and version specifier from a string in the form <name><version specifier(s)>
    Valid input examples:
        o3de>=1.2.3
        o3de-sdk==1.2.3,~=2.3.4
    :param input a string in the form <name><PEP 440 version specifier(s)> where the version specifier includes the relational operator such as ==, >=, ~=

    return an engine name and a version specifier or raises an exception if input is invalid
    """

    regex_str = r"(?P<object_name>(.*?))(?P<version_specifier>((~=|==|!=|<=|>=|<|>|===)(\s*\S+)+))"
    regex = re.compile(regex_str, re.IGNORECASE)
    match = regex.fullmatch(input.strip())

    if not match:
        raise InvalidVersionSpecifierException(f"Invalid name and/or version specifier {input}, expected <name><version specifiers> e.g. o3de==1.2.3")

    if not match.group("object_name"):
        raise InvalidObjectNameException(f"Invalid or missing name {input}, expected <name><version specifiers> e.g. o3de==1.2.3")

    # SpecifierSet will raise an exception if invalid
    if not SpecifierSet(match.group("version_specifier")):
        return None
    
    return match.group("object_name").strip(), match.group("version_specifier").strip()


def get_object_name_and_optional_version_specifier(input:str):
    """
    Returns an object name and optional version specifier 
    :param input: The input string
    """
    try:
        return get_object_name_and_version_specifier(input)
    except (InvalidObjectNameException, InvalidVersionSpecifierException):
        return input, None


def replace_dict_keys_with_value_key(input:dict, value_key:str, replaced_key_name:str = None):
    """
    Takes a dictionary of dictionaries and replaces the keys with the value of 
    a specific value key.
    For example, if you have a dictionary of gem_paths->gem_json_data, this function can be used
    to convert the dictionary so the keys are gem names instead of paths (gem_name->gem_json_data)
    :param input: A dictionary of key->value pairs where every value is a dictionary that has a value_key
    :param value_key: The value's key to replace the current key with
    :param replaced_key_name: (Optional) A key name under which to store the replaced key in value
    """

    # we cannot iterate over the dict while deleting entries
    # so we iterate over a copy of the keys
    keys = list(input.keys())
    for key in keys:
        value = input[key]

        # if the value is invalid just remove it
        if value == None:
            del input[key]
            continue

        # include the key we're removing if replaced_key_name provided
        if replaced_key_name:
            value[replaced_key_name] = key

        # remove the current entry 
        del input[key]

        # replace with an entry keyed on value_key's value
        input[value[value_key]] = value
