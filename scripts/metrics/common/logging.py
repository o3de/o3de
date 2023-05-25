"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Standardized logging util file for metrics scripts
"""

import enum
import logging
import sys


class Level(enum.IntEnum):
    """
    Enum reference for default log values
    """
    CRITICAL = 50
    ERROR = 40
    WARNING = 30
    INFO = 20
    DEBUG = 10
    NOTSET = 0


class Formatter(logging.Formatter):
    """
    Standardized log formatting for scripts.
    """
    def __init__(self):
        super().__init__("%(asctime)s  %(name)-40s  %(message)s")

    def format(self, record):
        return "{} {}".format(Formatter.get_level_acronym(record.levelno), super().format(record))

    @classmethod
    def get_level_acronym(cls, level: int):
        """
        Returns the corresponding log letter for formatting.
        :param level: The log level enum
        :return: The letter of the log level
        """
        if level == Level.CRITICAL:
            return "C"
        elif level == Level.ERROR:
            return "E"
        elif level == Level.WARNING:
            return "W"
        elif level == Level.INFO:
            return "I"
        elif level == Level.DEBUG:
            return "D"
        else:
            return f"({level})"


def get_logger(name=None):
    """
    Wrapper function
    """
    return logging.getLogger(name)


def setup_logger(logger, min_level=Level.INFO):
    """
    Standardized logging setup.
    :param logger: The logger object
    :param min_level: Minimum level of logging to be set
    :return: None
    """
    formatter = Formatter()

    out_handler = logging.StreamHandler(sys.stdout)
    out_handler.setLevel(Level.DEBUG)
    out_handler.addFilter(lambda record: record.levelno < Level.ERROR)
    out_handler.setFormatter(formatter)

    err_handler = logging.StreamHandler(sys.stderr)
    err_handler.setLevel(Level.ERROR)
    err_handler.setFormatter(formatter)

    logger.setLevel(min_level)
    logger.addHandler(out_handler)
    logger.addHandler(err_handler)
