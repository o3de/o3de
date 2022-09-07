# -*- coding: utf-8 -*-
# !/usr/bin/python
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

"""! @brief This module contains several common utilities/operations for Python when creating tools for the DCCsi. """

##
# @file model.py
#
# @brief This module collects and manages data for the DCCsi Launcher application.
#
# @section model_description Description
# Python module containing many commonly needed helper functions for Python tool development in the DCCsi.
#
# @section Launcher Model Notes
from dynaconf import settings
from SDK.Python import general_utilities as helpers
from azpy.o3de.utils import markdown_conversion
from Tools.Launcher.data import project_constants as constants
from pathlib import Path
import box
from box import Box
import logging
import os
import re

_LOGGER = logging.getLogger('Launcher.model')


class LauncherModel:
    def __init__(self):
        self.launcher_base_path = Path(settings.get('PATH_DCCSI_TOOLS')) / 'Launcher'
        self.launcher_sections = ['Splash', 'Tools', 'Projects', 'Output', 'Setup', 'Help']
        self.db_path = (self.launcher_base_path / 'data/launcher.db').as_posix()
        self.database_path = None
        self.icon_path = None
        self.db = None
        self.dcc_paths = {}
        self.cursor = None
        self.tools = Box({})
        self.projects = Box({})
        self.applications = Box({})
        self.tables = {
            'tools':        self.tools,
            'projects':     self.projects,
            'applications': self.applications
        }
        self.initialize_db_values()

    def initialize_db_values(self):
        _LOGGER.info('Initializing DB Values...')
        if self.connect_to_launcher_db():
            db_tables = helpers.get_tables(self.db)
            _LOGGER.info(f'Existing Tables in DB: {db_tables}')
            self.extract_db_values()
            self.get_projects()
            self.get_tools()
            self.get_applications()
        else:
            _LOGGER.info('Database connection error occurred')

    def connect_to_launcher_db(self):
        self.db = helpers.get_database(self.db_path)
        if self.db:
            self.cursor = self.db.cursor()
            return True
        return False

    def extract_db_values(self):
        database_values = helpers.get_database_values(self.cursor, [*self.tables])
        for key, values in database_values.items():
            categories = constants.TOOLS_CATEGORIES if key == 'tools' else constants.PROJECTS_CATEGORIES
            self.tables[key] = {item: {} for item in categories}
            for entry in values:
                temp_dict = {}
                for index, attribute_value in enumerate(entry):
                    temp_dict[constants.TOOLS_HEADERS[index]] = attribute_value
                self.tables[key][temp_dict['toolSection']][temp_dict['toolName']] = temp_dict
        self.close_database()

    def convert_markdown(self, markdown_file):
        markdown = markdown_conversion.convert_markdown_file(markdown_file)
        return markdown

    def create_tool_table(self):
        table_data = [
            'toolId integer PRIMARY KEY AUTOINCREMENT NOT NULL',
            'toolSection text NOT NULL',
            'toolPlacement text NOT NULL',
            'toolName text NOT NULL',
            'toolDescription varchar(20)',
            'toolStartFile varchar(20) NOT NULL',
            'toolCategory varchar(20) NOT NULL',
            'toolMarkdown varchar(20)',
            'toolDocumentation varchar(20)'
        ]
        helpers.create_table(self.db, 'tools', table_data)

    def set_tool(self, data):
        if self.connect_to_launcher_db():
            existing_tables = helpers.get_tables(self.db)
            if 'tools' not in existing_tables:
                self.create_tool_table()
            _LOGGER.info(f'Adding tool data: {data}')
            helpers.create_table_entry(self.db, 'tools', tuple(constants.TOOLS_HEADERS), tuple(data))
            return True
        return False

    def set_project(self):
        pass

    def get_projects(self):
        for category in constants.PROJECTS_CATEGORIES:
            self.tables['projects'][category] = settings.get(category)

    def get_tools(self):
        for tool_type in constants.TOOLS_CATEGORIES:
            try:
                self.tools[tool_type] = self.tables['tools'][tool_type]
            except box.exceptions.BoxKeyError:
                self.tools[tool_type] = {}

    def get_applications(self):
        application_list = {
            'PyCharm Professional': {'location': 'PYCHARM_EXE', 'exe': 'pycharm64.exe'},
            'WingIDE': {'location': 'WING_EXE', 'exe': 'wingdb.exe'},
            'Visual Studio Code': {'location': 'VSCODE_EXE', 'exe': 'code.exe'},
            'Maya': {'location': 'MAYA_EXE', 'exe': 'maya.exe'},
            'Blender': {'location': 'BLENDER_EXE', 'exe': 'blender.exe'},
            'Substance Designer': {'location': 'DESIGNER_EXE', 'exe': 'Adobe Substance 3D Designer.exe'},
            'Substance Painter': {'location': 'PAINTER_EXE', 'exe': 'Adobe Substance 3D Painter.exe'},
            'SubstanceAutomationToolkit': {'location': 'PATH_PYSBS', 'exe': None},
            'Houdini': {'location': 'HOUDINI.EXE', 'exe': 'houdini.exe'}
        }

        stored_values = self.get_stored_values('applications')

        for application_key, application_values in application_list.items():
            self.applications[application_key] = application_values
            try:
                self.applications[application_key]['location'] = stored_values[application_key]['location']
            except KeyError:
                target_key = application_list[application_key]['location']
                self.applications[application_key]['location'] = settings.get(target_key)
            finally:
                if not self.applications[application_key]['location']:
                    self.applications[application_key]['location'] = 'Not found'

    def get_application_path(self, application: str) -> Path:
        if application in self.applications.keys():
            return self.applications[application]
        return Path('')
    
    def get_table_names(self) -> list:
        return [*self.tables]

    def get_stored_values(self, target_table) -> dict:
        stored_tables = helpers.get_tables(self.db)
        if stored_tables and target_table in stored_tables:
            return helpers.get_table_values(self.cursor, target_table)
        return {}

    def get_sections(self) -> list:
        return self.launcher_sections

    def close_database(self):
        if self.db:
            self.db.close()
            self.db = None
            self.cursor = None

    @staticmethod
    def format_for_query(value):
        return f"\'{value}\'"

    @staticmethod
    def get_tools_categories() -> list:
        return constants.TOOLS_CATEGORIES

