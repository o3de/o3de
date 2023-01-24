"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Config file utilities for metrics scripts
"""

import os
import sys

import yaml

from common.exception import MetricsExn

config = {}


class ConfigExn(MetricsExn):
    pass


def load_config(local_path: str, override: list[list] = None):
    """
    Loads configuration file with provided override values.
    :param local_path: Path to the yaml configuration file as a string
    :param override: A list of lists where each list is a key/value pair to override
    :return: None
    """
    global config

    override = {} if override is None else override

    try:
        with open(os.path.join(sys.path[0], local_path), "r") as file:
            config = yaml.safe_load(file)
    except Exception as exc:
        raise ConfigExn(f"Failed to load configuration from local path '{local_path}'") from exc

    for path, value in override:
        # Keys are separated with periods i.e. (key1.key2.key3)
        path_elements = path.split(".")
        target = config

        while len(path_elements) > 0:
            key = path_elements.pop(0)

            if isinstance(target, dict) and key in target:
                if len(path_elements) == 0:
                    if isinstance(target[key], (int, float, str, bool)):
                        try:
                            target[key] = type(target[key])(value)
                        except ValueError as exc:
                            raise ConfigExn(
                                f"Failed to parse value '{value}' of override configuration parameter '{path}'"
                            ) from exc
                else:
                    target = target[key]
            else:
                raise ConfigExn(f"Invalid override configuration parameter '{path}'")


def read_config(path: str, source: dict = None):
    """
    Reads the value for the given path from the yaml config file
    :param path: The path as a string (i.e. key1.key2)
    :param source: A source override as a dict
    :return: The value for the given path
    """
    if source is None:
        source = config

    path_elements = path.split(".")

    while len(path_elements) > 0:
        if isinstance(source, dict):
            key = path_elements.pop(0)

            # Allow empty paths and paths ending in a dot.
            if len(key) == len(path_elements) == 0:
                break

            try:
                source = source[key]
            except KeyError as exc:
                raise ConfigExn(f"Invalid configuration path '{path}' - '{key}' is not a key") from exc
        else:
            raise ConfigExn(f"Invalid configuration path '{path}' - path too long")

    return source
