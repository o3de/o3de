"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import os
import platform
from typing import Dict

from utils import file_utils

"""
Environment Utils provide functions to setup python environment libs for resource mapping tool
"""
logger = logging.getLogger(__name__)

qt_binaries_linked: bool = False
old_os_env: Dict[str, str] = os.environ.copy()


def setup_qt_environment(bin_path: str) -> None:
    """
    Setup Qt binaries for O3DE python runtime environment
    :param bin_path: The path of Qt binaries
    """
    if is_qt_linked():
        logger.info("Qt binaries have already been linked, skip Qt setup")
        return
    global old_os_env
    old_os_env = os.environ.copy()
    binaries_path: str = file_utils.normalize_file_path(bin_path)

    # Python 3.8 DLL dependencies for extension modules and DLLs loaded with ctypes on Windows
    # are now resolved more securely. Only the system paths, the directory containing the DLL or PYD file,
    # and directories added with add_dll_directory() are searched for load-time dependencies.
    # Specifically, PATH and the current working directory are no longer used, and modifications to these
    # will no longer have any effect on normal DLL resolution.
    if platform.system() == 'Windows':
        os.add_dll_directory(binaries_path)

    os.environ["QT_PLUGIN_PATH"] = binaries_path

    path = os.environ['PATH']

    new_path = os.pathsep.join([binaries_path, path])
    os.environ['PATH'] = new_path

    # On Linux, we need to load pyside2 and related modules as well
    if platform.system() == 'Linux':
        import ctypes

        preload_shared_libs = [f'{bin_path}/libpyside2.abi3.so.5.14',
                               f'{bin_path}/libQt5Widgets.so.5']

        for preload_shared_lib in preload_shared_libs:
            if not os.path.exists(preload_shared_lib):
                logger.error(f"Cannot find required shared library at {preload_shared_lib}")
                return
            else:
                ctypes.CDLL(preload_shared_lib)

    global qt_binaries_linked
    qt_binaries_linked = True


def is_qt_linked() -> bool:
    """
    Check whether Qt binaries have been linked in o3de python runtime environment
    :return: True if Qt binaries have been linked; False if not
    """
    return qt_binaries_linked


def cleanup_qt_environment() -> None:
    """
    Clean up the linked Qt binaries in O3DE python runtime environment
    """
    if not is_qt_linked():
        logger.info("Qt binaries have not been linked, skip Qt uninstall")
        return
    global old_os_env
    if old_os_env.get("QT_PLUGIN_PATH"):
        old_os_env.pop("QT_PLUGIN_PATH")
    os.environ = old_os_env

    global qt_binaries_linked
    qt_binaries_linked = False
