"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Helper file for assisting in building workspaces and setting up LTT with the current Lumberyard environment.
"""

import os
import sys

import yaml

from common.exception import MetricsExn

config = {}


class ConfigExn(MetricsExn):
    pass


def load_config(local_path, override=None):
    global config

    if override is None:
        override = {}

    try:
        with open(os.path.join(sys.path[0], local_path), "r") as file:
            config = yaml.safe_load(file)
    except FileNotFoundError:
        raise ConfigExn(f"Failed to load configuration from local path '{local_path}'") from None

    for path, value in override:
        path_elements = path.split(".")
        target = config

        while len(path_elements) > 0:
            key = path_elements.pop(0)

            if isinstance(target, dict) and key in target:
                if len(path_elements) == 0:
                    if isinstance(target[key], (int, float, str, bool)):
                        try:
                            target[key] = type(target[key])(value)
                        except ValueError:
                            raise ConfigExn(
                                f"Failed to parse value '{value}' of override configuration parameter '{path}'"
                            ) from None
                else:
                    target = target[key]
            else:
                raise ConfigExn(f"Invalid override configuration parameter '{path}'")


def read_config(path, source=None):
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
            except KeyError:
                raise ConfigExn(f"Invalid configuration path '{path}' - '{key}' is not a key") from None
        else:
            raise ConfigExn(f"Invalid configuration path '{path}' - path too long")

    return source
