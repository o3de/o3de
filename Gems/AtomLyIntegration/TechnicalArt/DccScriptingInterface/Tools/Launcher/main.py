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

##
# @file main.py
#
# @brief
# The Launcher (and main.py module) is an entrypoint for launching DCC-related productivity scripts within the DCCsi.
#
# @section description_main Description
# The Launcher is a tool for creating, storing, and accessing environments supporting the DCCsi. It provides an
# easy-to-use starting point for gaining access to DCCsi tools, and also offers a GUI-based interface for
# experienced coders and non-coders alike for accessing our growing collection of workflow enhancing tools. It also
# provides the means to manage DCC and O3DE-based project assets, create temporary overrides to assist in testing,
# and connects to help resources for new users.
#


import config
from dynaconf import settings
import logging
from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtCore import Slot
from Tools.Launcher.navigation import Navigation
from Tools.Launcher.sections import splash, tools, projects, output, setup, help, database
from box import Box


_LOGGER = logging.getLogger('Tools.Dev.Windows.Launcher.main')
_LOGGER.info('Launcher started')


class ContentContainer(QtWidgets.QWidget):
    """! This sets up the content window and navigation system

    All changes to window content move through this class, which is driven by user events.
    """
    def __init__(self, model):
        super(ContentContainer, self).__init__()

        self.app = QtWidgets.QApplication.instance()
        self.model = model
        self.sections = Box({})
        self.current_section = None
        self.main_layout = QtWidgets.QVBoxLayout(self)
        self.main_layout.setAlignment(QtCore.Qt.AlignTop)
        self.main_layout.setContentsMargins(0, 0, 0, 0)

        self.top_gutter = QtWidgets.QFrame(self)
        self.top_gutter.setGeometry(0, 0, 5000, 80)
        self.top_gutter.setStyleSheet('background-color:rgb(68, 68, 68);')

        self.navigation = Navigation(self.model)
        self.navigation.section_requested.connect(self.change_section)
        self.main_layout.addWidget(self.navigation)
        self.sections_layout = QtWidgets.QStackedLayout()
        self.main_layout.addLayout(self.sections_layout)
        self.build()

    def build(self):
        """! This function sets up the main content widget that contains all of the section content. Each
        section is a separate class (separated and stored inside the Launcher package's 'sections' directory),
        loaded into a stacked layout that serves up content based on main navigation clicks
        """
        self.sections['splash'] = [splash.Splash(self.model), 0]
        self.sections['tools'] = [tools.Tools(self.model), 1]
        self.sections['projects'] = [projects.Projects(self.model), 2]
        self.sections['output'] = [output.Output(self.model), 3]
        self.sections['setup'] = [setup.Setup(self.model), 4]
        self.sections['help'] = [help.Help(self.model), 5]
        self.sections['database'] = [database.Database(self.model), 6]
        for section in self.sections.values():
            self.sections_layout.addWidget(section[0])

        # Change this back to splash once you are done testing
        # self.change_section('tools')

    @Slot(str)
    def change_section(self, target_section: str):
        """! This is the central point for changing the content container based on a user clicking on a main
        navigation button.

        @param  target_section This is the string passed in the signal that corresponds to requested section
        """
        _LOGGER.info(f'Section change event:::  Switch to {target_section}')
        if self.current_section:
            self.sections[self.current_section][0].close_section()
        self.current_section = target_section
        self.sections_layout.setCurrentIndex(self.sections[self.current_section ][1])
        self.sections[self.current_section][0].open_section()


