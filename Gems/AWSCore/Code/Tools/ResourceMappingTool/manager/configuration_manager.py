"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from __future__ import annotations
import logging

from model import (constants, error_messages)
from model.configuration import Configuration
from utils import aws_utils
from utils import file_utils

logger = logging.getLogger(__name__)


class ConfigurationManager(object):
    """
    Configuration manager maintains all global setups for this tool
    """
    __instance: ConfigurationManager = None
    
    @staticmethod
    def get_instance() -> ConfigurationManager:
        if ConfigurationManager.__instance is None:
            ConfigurationManager()
        return ConfigurationManager.__instance
    
    def __init__(self) -> None:
        if ConfigurationManager.__instance is None:
            self._configuration: Configuration = Configuration()
            
            ConfigurationManager.__instance = self
        else:
            raise AssertionError(error_messages.SINGLETON_OBJECT_ERROR_MESSAGE.format("ConfigurationManager"))
    
    @property
    def configuration(self) -> Configuration:
        return self._configuration
    
    @configuration.setter
    def configuration(self, new_configuration: ConfigurationManager) -> None:
        self._configuration = new_configuration

    def setup(self) -> None:
        logger.info("Setting up default configuration ...")
        # TODO: remove config directory and files default setup once integrating with user input
        try:
            self._configuration.config_directory = file_utils.get_current_directory_path()
            self._configuration.config_files = \
                file_utils.find_files_with_suffix_under_directory(self._configuration.config_directory,
                                                                  constants.RESOURCE_MAPPING_CONFIG_FILE_NAME_SUFFIX)

            self._configuration.account_id = aws_utils.get_default_account_id()
            self._configuration.region = aws_utils.get_default_region()
        except (RuntimeError, FileNotFoundError) as e:
            logger.exception(e)
        logger.debug(self._configuration)
