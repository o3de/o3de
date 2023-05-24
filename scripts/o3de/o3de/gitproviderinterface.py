#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
This file contains the interface for the Git providers
"""

import pathlib
from abc import ABC, abstractmethod
from urllib.parse import ParseResult

class GitProviderInterface(ABC):
    @abstractmethod
    def get_specific_file_uri(self, parsed_uri: ParseResult):
        """
        Gets a uri that can be used to download a resource if the provide allows it, otherwise returns the unmodified uri
        :param uri: uniform resource identifier to get a downloadable link for
        :return: uri that can be used to download the given resource
        """
        pass

    @abstractmethod
    def clone_from_git(self, uri: ParseResult, download_path: pathlib.Path, force_overwrite: bool = False, ref: str = None) -> int:
        """
        Clones a git repository from a uri into a given folder
        :param uri: uniform resource identifier
        :param download_path: location path on disk to download file
        :param force_overwrite: whether to force overwrite the contents that already exist on disk or not
        :param ref: optional source control reference which can be a commit hash, branch, tag or other reference type
        :return: return code, 0 on success
        """
        pass
