#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
This file contains utility functions
"""

import uuid
import pathlib
import shutil
import urllib.request

def validate_identifier(identifier: str) -> bool:
    """
    Determine if the identifier supplied is valid.
    :param identifier: the name which needs to to checked
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


def sanitize_identifier_for_cpp(identifier: str) -> str:
    """
    Convert the provided identifier to a valid C++ identifier
    :param identifier: the name which needs to to sanitized
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
    Determine if the uuid supplied is valid.
    :param uuid_string: the uuid which needs to to checked
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


def download_file(parsed_uri, download_path: pathlib.Path) -> int:
    """
    :param parsed_uri: uniform resource identifier to zip file to download
    :param download_path: location path on disk to download file
    """
    if download_path.is_file():
        logger.warn(f'File already downloaded to {download_path}.')
    elif parsed_uri.scheme in ['http', 'https', 'ftp', 'ftps']:
        with urllib.request.urlopen(url) as s:
            with download_path.open('wb') as f:
                shutil.copyfileobj(s, f)
    else:
        origin_file = pathlib.Path(url).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_path)

    return 0


def download_zip_file(parsed_uri, download_zip_path: pathlib.Path) -> int:
    """
    :param parsed_uri: uniform resource identifier to zip file to download
    :param download_zip_path: path to output zip file
    """
    download_file_result = download_file(parsed_uri, download_zip_path)
    if download_file_result != 0:
        return download_file_result

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"File zip {download_zip_path} is invalid.")
        download_zip_path.unlink()
        return 1

    return 0