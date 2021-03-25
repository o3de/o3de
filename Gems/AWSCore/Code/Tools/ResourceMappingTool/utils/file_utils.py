"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import logging
import pathlib
from typing import List

"""
File Utils provide related functions to work with filesystem, e.g. get working directory
path, check file existence, etc
"""
logger = logging.getLogger(__name__)


def check_path_exists(full_path: str) -> bool:
    return pathlib.Path(full_path).exists()


def get_current_directory_path() -> str:
    return str(pathlib.Path.cwd())


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
    
    
def join_path(dir_path: str, file_name: str) -> str:
    # TODO: expand usage to support Mac and Linux
    return str(pathlib.WindowsPath(dir_path).joinpath(file_name))
