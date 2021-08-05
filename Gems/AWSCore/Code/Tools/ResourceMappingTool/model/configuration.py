"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from __future__ import annotations
import logging
from typing import List

from model import error_messages
from utils import file_utils

logger = logging.getLogger(__name__)


class Configuration(object):
    """
    Data structure to store the global configuration for resource mapping tool
    """
    def __init__(self) -> None:
        self._config_directory: str = ""
        self._config_files: List[str] = []
        self._account_id: str = ""
        self._region: str = ""
    
    def __str__(self) -> str:
        return self._print_format()
    
    def __repr__(self) -> str:
        return self._print_format()
    
    def _print_format(self) -> str:
        return (f"ConfigFileDirectory:{self._config_directory},ConfigFiles:{self._config_files}"
                f"AccountId:{self._account_id},Region:{self._region}")
    
    @property
    def config_directory(self) -> str:
        return self._config_directory
    
    @config_directory.setter
    def config_directory(self, new_config_directory: str) -> None:
        if file_utils.check_path_exists(new_config_directory):
            self._config_directory = new_config_directory
        else:
            raise FileNotFoundError(error_messages.FILE_NOT_FOUND_ERROR_MESSAGE.format(new_config_directory))

    @property
    def config_files(self) -> List[str]:
        return self._config_files

    @config_files.setter
    def config_files(self, new_config_files: List[str]) -> None:
        self._config_files.clear()
        self._config_files.extend(new_config_files)
    
    @property
    def account_id(self) -> str:
        return self._account_id
    
    @account_id.setter
    def account_id(self, new_account_id: str) -> None:
        self._account_id = new_account_id
    
    @property
    def region(self) -> str:
        return self._region
    
    @region.setter
    def region(self, new_region: str) -> None:
        self._region = new_region


class ConfigurationBuilder(object):
    """
    The ConfigurationBuilder class provides a builder pattern for Configuration
    """
    def __init__(self) -> None:
        self._configuration: Configuration = Configuration()

    def build(self) -> Configuration:
        configuration: Configuration = self._configuration
        self.reset()
        return configuration

    def build_account_id(self, account_id_value: str) -> ConfigurationBuilder:
        self._configuration.account_id = account_id_value
        return self

    def build_config_files(self, config_files_value: List[str]) -> ConfigurationBuilder:
        self._configuration.config_files = config_files_value
        return self

    def build_config_directory(self, config_directory_value: str) -> ConfigurationBuilder:
        self._configuration.config_directory = config_directory_value
        return self

    def build_region(self, region_value: str) -> ConfigurationBuilder:
        self._configuration.region = region_value
        return self

    def reset(self) -> None:
        self._configuration = Configuration()
