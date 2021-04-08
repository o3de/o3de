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

import os
import logging

from pathlib import Path

logger = logging.getLogger()

pyside_initialized = False
old_env = os.environ.copy()

# Helper to extend OS PATH for pyside to locate our QT binaries based on our build folder
def add_pyside_environment(bin_path):
    if is_pyside_ready():
        # No need to reinitialize currently
        logger.info("Pyside environment already initialized")
        return
    global old_env
    old_env = os.environ.copy()
    binaries_path = Path(os.path.normpath(bin_path))
    platforms_path = binaries_path.joinpath("platforms")
    logger.info(f'Adding binaries path {binaries_path}')
    os.environ["QT_QPA_PLATFORM_PLUGIN_PATH"] = str(platforms_path)

    path = os.environ['PATH']

    new_path = os.pathsep.join([str(binaries_path), str(platforms_path), path])
    os.environ['PATH'] = new_path

    global pyside_initialized
    pyside_initialized = True


def is_pyside_ready():
    return pyside_initialized


def is_configuration_valid(workspace):
    return os.path.basename(workspace.paths.build_directory()) != "debug"


def uninstall_env():
    if not is_pyside_ready():
        logger.warning("Pyside not initialized")
        return os.environ

    global old_env
    if old_env.get("QT_QPA_PLATFORM_PLUGIN_PATH"):
        old_env.pop("QT_QPA_PLATFORM_PLUGIN_PATH")
    os.environ = old_env
    return old_env
