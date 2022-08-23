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

"""! @brief Baselogger establishes the logging configuration supporting the DCCsi. """

##
# @file baselogger.py
#
# @brief This serves as the starting point for logging in the DCCsi. All work in the DCCsi necessarily begins by
# importing the config.py file from the DCCsi base directory. It begins by instantiating the default configuration
# unless overridden by a stored file configuration, and can be modified at any point subsequently. The default
# configuration is basic. For more complicated configurations, the best approach would be to use the
# load_configuration_file function below. To set up a persistent startup file configuration, add the file location to
# the settings.json file in the root of the dccsi after "DCCSI_LOGGING_FILE_CFG" under the default environment settings
#
# @section Launcher Notes
# - Comments are Doxygen compatible

from dynaconf import settings
from azpy.constants import FRMT_LOG_LONG
from pathlib import Path
import logging
from logging import config
import sys
import os


for handler in logging.root.handlers[:]:
    logging.root.removeHandler(handler)


class BaseLogger:
    def __init__(self, dccsi_base_directory):
        self.dccsi_base_directory = dccsi_base_directory
        self.log_file = Path(self.dccsi_base_directory, '.temp\logs\dccsi.log')
        self.settings_file = Path(self.dccsi_base_directory) / 'settings.json'
        self.color_formatting = True
        self.log_configuration_directory = None
        self.file_handler = None
        self.stream_handler = None
        self.log_level = logging.INFO
        self.file_log_level = logging.ERROR
        self.get_dynaconf_configuration()

    def set_initial_values(self):
        settings.set('DCCSI_BASE_DIRECTORY', str(self.dccsi_base_directory))
        settings.set('DCCSI_LOG_LEVEL', 20)
        settings.set('DCCSI_FILE_LOG_LEVEL', 40)

    def get_dynaconf_configuration(self):
        if settings.get('FORCE_RESET'):
            self.set_initial_values()
        self.set_logging_configuration(settings.get('DCCSI_LOG_LEVEL'),
                                       settings.get('DCCSI_LOGGING_FILE_CFG'),
                                       settings.get('DCCSI_FILE_LOG_LEVEL'))

    def set_logging_configuration(self, log_level, file_config, file_log_level):
        if file_config:
            config.fileConfig(file_config)
        else:
            try:
                os.makedirs(os.path.dirname(self.log_file), exist_ok=True)
                self.log_level = self.log_level if not isinstance(log_level, int) else log_level
                self.file_log_level = self.file_log_level if not isinstance(file_log_level, int) else file_log_level
                formatter = ColorFormatter() if self.color_formatting else FRMT_LOG_LONG
                self.stream_handler = logging.StreamHandler(sys.stdout)
                self.stream_handler.setFormatter(formatter)
                self.file_handler = logging.FileHandler(filename=self.log_file)
                handlers = [self.file_handler, self.stream_handler]
                logging.basicConfig(level=self.log_level,
                                    datefmt='%m-%d %H:%M',
                                    handlers=handlers)
            except AttributeError as e:
                sys.exit(f'Logging configuration failed: {e}')

    @staticmethod
    def get_logger(target_module):
        logger = logging.getLogger(target_module)
        return logger

    def load_configuration_file(self, config_file='file.conf'):
        config.fileConfig(self.log_configuration_directory / config_file)
        _LOGGER = logging.getLogger('DCCsi.baselogger')
        _LOGGER.info(f'Logging config file loaded: {config_file}')


class ColorFormatter(logging.Formatter):
    grey = "\x1b[38;20m"
    yellow = "\x1b[33;20m"
    red = "\x1b[31;20m"
    bold_red = "\x1b[31;1m"
    reset = "\x1b[0m"
    format = FRMT_LOG_LONG

    FORMATS = {
        logging.DEBUG:    grey + format + reset,
        logging.INFO:     grey + format + reset,
        logging.WARNING:  yellow + format + reset,
        logging.ERROR:    red + format + reset,
        logging.CRITICAL: bold_red + format + reset
    }

    def format(self, record):
        log_fmt = self.FORMATS.get(record.levelno)
        formatter = logging.Formatter(log_fmt)
        return formatter.format(record)
