"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import pathlib
from typing import List

"""
File Utils provide related functions to work with filesystem, e.g. get working directory
path, check file existence, etc
"""
logger = logging.getLogger(__name__)


def check_path_exists(file_path: str) -> bool:
    return pathlib.Path(file_path).exists()


def get_current_directory_path() -> str:
    return str(pathlib.Path.cwd())


def get_parent_directory_path(file_path: str) -> str:
    return pathlib.Path(file_path).parent


def find_files_with_suffix_under_directory(dir_path: str, suffix: str) -> List[str]:
    results: List[str] = []
    paths: List[pathlib.Path] = \
        pathlib.Path(dir_path).glob(f"*{suffix}")
    matched_path: pathlib.Path
    for matched_path in paths:
        logger.debug(f"Got matched path {str(matched_path)}")
        if matched_path.is_file():
            results.append(str(matched_path.name))
    return results


def normalize_file_path(file_path: str) -> str:
    if file_path:
        try:
            return str(pathlib.Path(file_path).resolve(True))
        except (FileNotFoundError, RuntimeError):
            logger.warning(f"Failed to normalize file path {file_path}, return empty string instead")
    return ""


def join_path(this_path: str, other_path: str) -> str:
    return str(pathlib.Path(this_path).joinpath(other_path))
