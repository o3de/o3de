# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

import enum
import logging
import sys


class Level(enum.IntEnum):
    CRITICAL = 50
    ERROR = 40
    WARNING = 30
    INFO = 20
    DEBUG = 10
    NOTSET = 0


class Formatter(logging.Formatter):
    def __init__(self):
        super().__init__("%(asctime)s  %(name)-40s  %(message)s")

    def format(self, record):
        return "{} {}".format(Formatter.get_level_acronym(record.levelno), super().format(record))

    @classmethod
    def get_level_acronym(cls, level):
        if level >= Level.CRITICAL:
            return "C"
        elif level >= Level.ERROR:
            return "E"
        elif level >= Level.WARNING:
            return "W"
        elif level >= Level.INFO:
            return "I"
        elif level >= Level.DEBUG:
            return "D"
        else:
            return "?"


def get_logger(name=None):
    return logging.getLogger(name)


def setup_logger(logger, min_level=Level.INFO):
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
