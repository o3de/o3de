#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! @brief This module contains several common utilities/operations for Python when creating tools for the DCCsi. """

##
# @file general_utilities.py
#
# @brief This module contains several helper functions useful when creating Python tool scripts.
#
# @section General Utilities Description
# Python module containing many commonly needed helper functions for Python tool development in the DCCsi.
# DCCsi (Digital Content Creation Scripting Interface)
#
# @section QT Utilities Notes
# - Comments are Doxygen compatible

# standard imports
import sqlite3
from sqlite3 import Error
from pathlib import Path
from box import Box
import logging as _logging
import json
import re
import os

# module global scope
from DccScriptingInterface.azpy import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.general_utilities'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


def get_incremented_filename(name):
    last_number = re.compile(r'(?:[^\d]*(\d+)[^\d]*)+')
    number_found = last_number.search(name)
    if number_found:
        next_number = str(int(number_found.group(1)) + 1)
        start, end = number_found.span(1)
        name = name[:max(end - len(next_number), start)] + next_number + name[end:]
    return name


##############
# VALIDATION #
##############


def validate_markdown(target_path: Path) -> bool:
    with open(target_path) as markdown_file:
        for line in markdown_file:
            if '### File Under' in line:
                return True
    return False


def validate_path(target_path: Path) -> Path:
    if target_path.is_file():
        return target_path
    return Path('')


#######################
# GETTERS AND SETTERS #
#######################


def get_json_data(target_path: Path) -> dict:
    try:
        with open(target_path) as json_file:
            return json.load(json_file)
    except IOError:
        raise Exception(f'File [{target_path}] not found.')


def get_commented_json_data(target_path: Path) -> dict:
    try:
        with open(target_path) as json_file:
            data = json_file.read()
            data = re.sub("//.*", "", data, flags=re.MULTILINE)
            return json.loads(data)
    except IOError:
        raise Exception(f'File [{target_path}] not found.')


def get_markdown_information(target_path: Path) -> dict:
    markdown = {}
    current_section = ''
    content_string = ''
    with open(target_path) as markdown_file:
        for line in markdown_file:
            heading = re.match(r'[#]+[ ]{1,}[\w\s]+', line)
            if heading:
                heading_parts = heading.group(0).split(' ')
                section = ' '.join(heading_parts[1:])
                if current_section:
                    markdown[current_section] = content_string
                    content_string = ''
                current_section = section.rstrip()
            else:
                content_string += line.rstrip()
    markdown[current_section] = content_string
    return markdown


def get_clean_path(target_path) -> str:
    return target_path.replace('\\', '/')


def get_files_by_name(base_directory: Path, target_file: str) -> list:
    file_list = []
    for (root, dirs, files) in os.walk(base_directory, topdown=True):
        root = Path(root)
        for file in files:
            if file == target_file:
                file_list.append(root / file)
    return file_list


def set_json_data(target_path: Path, json_data: Box):
    with open(str(target_path), 'w') as json_file:
        json.dump(json_data, json_file, indent=4)


###########################
# SQLITE DATABASE HELPERS #
###########################


def get_database(database_path: str) -> sqlite3.Connection:
    try:
        db = sqlite3.connect(database_path)
        _LOGGER.info(f'DB Connection Made: {sqlite3.version}')
        return db
    except Error as e:
        _LOGGER.info(f'Database connection failed. Error: {e}')
        return None


def create_table(db: sqlite3.Connection, cursor: sqlite3.Cursor, table_name: str, headers: list):
    header_string = ', '.join(headers)
    _LOGGER.info(f'CreatingTable [{table_name}]: {header_string}')
    execute_db_command(db, cursor, f"""CREATE TABLE IF NOT EXISTS {table_name} ({header_string})""")


def remove_table(db: sqlite3.Connection, cursor: sqlite3.Cursor, table_name: str):
    execute_db_command(db, cursor, f"""DROP TABLE {table_name}""")


def create_table_entry(db: sqlite3.Connection, cursor: sqlite3.Cursor, table_name: str, values: list):
    values_string = ', '.join(values)
    execute_db_command(db, cursor, f"""INSERT INTO {table_name} VALUES ({values_string})""")


def remove_table_entry(db: sqlite3.Connection, cursor: sqlite3.Cursor, table_name: str, row_id: int):
    execute_db_command(db, cursor, f"""DELETE from {table_name} where id = {row_id}""")


def execute_db_command(db: sqlite3.Connection, cursor: sqlite3.Cursor, command: str):
    _LOGGER.info(f'Executing: {command}')
    cursor.execute(command)
    db.commit()


def get_table_values(cursor: sqlite3.Cursor, table: str) -> dict:
    table_dict = {}
    cursor.execute(f"""SELECT * from {table}""")
    values = cursor.fetchall()
    for value in values:
        table_dict[value[0]] = value[1]
    return table_dict


def get_database_values(cursor: sqlite3.Cursor, tables: list) -> dict:
    database_values = {}
    for table in tables:
        cursor.execute(f"""SELECT * from {table}""")
        table_values = cursor.fetchall()
        database_values[table] = table_values
    return database_values


