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
This file contains utility functions
"""

import uuid


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