"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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

    def setup(self, profile_name: str, config_path: str) -> bool:
        result: bool = True
        logger.debug("Setting up default configuration ...")
        try:
            logger.debug("Setting up boto3 default session ...")
            aws_utils.setup_default_session(profile_name)

            logger.debug("Setting up config directory and files ...")
            normalized_config_path: str = file_utils.normalize_file_path(config_path)
            if normalized_config_path:
                self._configuration.config_directory = normalized_config_path
            else:
                self._configuration.config_directory = file_utils.get_current_directory_path()
            self._configuration.config_files = \
                file_utils.find_files_with_suffix_under_directory(self._configuration.config_directory,
                                                                  constants.RESOURCE_MAPPING_CONFIG_FILE_NAME_SUFFIX)

            logger.debug("Setting up aws account id and region ...")
            self._configuration.account_id = aws_utils.get_default_account_id()
            self._configuration.region = aws_utils.get_default_region()
        except (RuntimeError, FileNotFoundError) as e:
            logger.error(e)
            result = False
        logger.debug(self._configuration)
        return result
