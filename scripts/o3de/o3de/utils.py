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


def download_file(parsed_uri, download_path: pathlib.Path, force_overwrite: bool = False, download_progress_callback = None) -> int:
    """
    Download file
    :param parsed_uri: uniform resource identifier to zip file to download
    :param download_path: location path on disk to download file
    :param download_progress_callback: callback called with the download progress as a percentage, returns true to request to cancel the download
    """
    if download_path.is_file():
        if not force_overwrite:
            logger.error(f'File already downloaded to {download_path} and force_overwrite is not set.')
            return 1
        else:
            try:
                os.unlink(download_path)
            except OSError:
                logger.error(f'Could not remove existing download path {download_path}.')
                return 1

    if parsed_uri.scheme in ['http', 'https', 'ftp', 'ftps']:
        try:
            with urllib.request.urlopen(parsed_uri.geturl()) as s:
                download_file_size = 0
                try:
                    download_file_size = s.headers['content-length']
                except KeyError:
                    pass

                def download_progress(downloaded_bytes):
                    if download_progress_callback:
                        return download_progress_callback(int(downloaded_bytes), int(download_file_size))
                    return False

                with download_path.open('wb') as f:
                    download_cancelled = copyfileobj(s, f, download_progress)
                    if download_cancelled:
                        logger.info(f'Download of file to {download_path} cancelled.')
                        return 1
        except urllib.error.HTTPError as e:
            logger.error(f'HTTP Error {e.code} opening {parsed_uri.geturl()}')
            return 1
    else:
        origin_file = pathlib.Path(parsed_uri.geturl()).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_path)

    return 0


def download_zip_file(parsed_uri, download_zip_path: pathlib.Path, force_overwrite: bool, download_progress_callback = None) -> int:
    """
    :param parsed_uri: uniform resource identifier to zip file to download
    :param download_zip_path: path to output zip file
    """
    download_file_result = download_file(parsed_uri, download_zip_path, force_overwrite, download_progress_callback)
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
