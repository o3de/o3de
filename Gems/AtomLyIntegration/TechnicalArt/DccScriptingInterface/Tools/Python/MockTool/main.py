# # coding:utf-8
# #!/usr/bin/python
# #
# # All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# # its licensors.
# #
# # For complete copyright and license terms please see the LICENSE at the root of this
# # distribution (the "License"). All use of this software is governed by the License,
# # or, if provided, by the license below or the license accompanying this file. Do not
# # remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# #

"""! @brief MockTool is a collection of template files that can be used to start a new tool within the DCCsi. """

##
# @file main.py
#
# @brief It is difficult to predict in what form tools developed in the DCCsi will take, but a few elements will
# remain constant when creating a new tool. Environment will usually play a significant role in tools creation. The
# DCCsi is supported with Dynaconf for setting environments- main settings are created and maintained within both the
# config.py and settings.json files in the base directory of the DCCsi, but per tool settings can be created by way of
# and ".env" file. A sample of this file has been included in the sample MockTool directory.
#
# Logging is also an important requirement for developing tools and workflows to assist in debugging. The formatting
# is very easy to follow- see below for an example of the setup. By changing the "_MODULENAME" path to correspond with
# a tool's module path is all that is needed- by adding logging statements (also included below), formatted logging
# output will be enabled.
#
# The last important element to point out is that each tool should include the three imports listed below:
#  - The "config" module ensures that Python has access to O3DE-utilizied third party libraries, and establishes the
#    Dynaconf settings
#  - Logging must be imported in order to access the main logging system that supports the DCCsi. No new configurations
#    are necessary. Users can leverage the default settings, or adjust the existing configuration with their own
#    desired settings when developing tools.
#  - The last import of "settings" connects the established Dynaconf environment. The best access of environment
#    settings can be achieved by using a combination of settings established in the .env file, as well as leveraging
#    DCCsi default settings, as well as targeted DCCsi environment settings that exist specific to development needs.
#
#  Examples of each of the elements above can be found below, and tailored to the needs of a project.
#
# @section Launcher Notes
# - Comments are Doxygen compatible

# Required Imports
import config
import logging
from dynaconf import settings
from PySide2 import QtWidgets

# Logging Formatting
_MODULENAME = 'Tools.Python.MockTool.main'
_LOGGER = logging.getLogger(_MODULENAME)


class MockTool(QtWidgets.QWidget):
    def __init__(self, *args, **kwargs):
        super(MockTool, self).__init__()
        _LOGGER.info('MockTool Added')
        self.main_container = QtWidgets.QVBoxLayout(self)
        self.test_button = QtWidgets.QPushButton('Hello')
        self.main_container.addWidget(self.test_button)

    @staticmethod
    def get_environment_values(target_environment=None):
        """ Gather environment variables stored in Dynaconf """
        for key, value in settings.items():
            if target_environment:
                if key in settings.from_env(target_environment):
                    _LOGGER.info(f"Key: {key}  Value: {value}")
            else:
                _LOGGER.info(f"Key: {key}  Value: {value}")

    @staticmethod
    def get_env_value():
        """ Access variables from the .env file """

        # Get Settings Value
        _LOGGER.info(f"Access the DYNACONF_ICON_PATH value from the .env file: {settings.get('ICON_PATH')}")
        _LOGGER.info(f"Access the DYNACONF_DB_PATH value from the .env file: {settings.get('DB_PATH')}")

        # Check if Settings Value is present - if it isn't will return the second value (in this case "404")
        _LOGGER.info(f"Accessing Dynaconf Base Directory Variable: {settings.get('NO_KEY_AVAILABLE', 404)}")


def get_tool(*args, **kwargs):
    _LOGGER.info('Get tool fired')
    return MockTool(*args, **kwargs)


