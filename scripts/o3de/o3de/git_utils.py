#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
This file contains utility functions for using GitHub
"""

import logging
import pathlib
from urllib.parse import ParseResult
import subprocess

from o3de import gitproviderinterface, utils

LOG_FORMAT = '[%(levelname)s] %(name)s: %(message)s'

logger = logging.getLogger('o3de.git_utils')
logging.basicConfig(format=LOG_FORMAT)

class GenericGitProvider(gitproviderinterface.GitProviderInterface):
    def get_specific_file_uri(self, parsed_uri: ParseResult):
        logger.warning(f"GenericGitProvider does not yet support retrieving individual files")
        return parsed_uri

    def clone_from_git(self, uri, download_path: pathlib.Path, force_overwrite: bool = False, ref: str = None) -> int:
        """
        :param uri: uniform resource identifier
        :param download_path: location path on disk to download file
        :param ref: optional source control reference (commit/branch/tag) 
        """
        if download_path.exists():
            # check if the path is not empty
            if any(download_path.iterdir()):
                if not force_overwrite:
                    logger.error(f'Cannot clone into non-empty folder {download_path}')
                    return 1
                else:
                    try:
                        utils.remove_dir_path(download_path)
                    except OSError:
                        logger.error(f'Could not remove existing download path {download_path}')
                        return 1

        params = ["git", "clone", uri, download_path.as_posix()]
        try:
            with subprocess.Popen(params, stdout=subprocess.PIPE) as proc:
                print(proc.stdout.read())
        except Exception as e:
            logger.error(str(e))
            return 1

        if proc.returncode == 0 and ref:
            params = ["git", "-C", download_path.as_posix(), "reset", "--hard", ref]
            try:
                with subprocess.Popen(params, stdout=subprocess.PIPE) as proc:
                    print(proc.stdout.read())
            except Exception as e:
                logger.error(str(e))
                return 1

        return proc.returncode

    def upload_release(self, repo_uri: ParseResult, zip_path: pathlib.Path, archive_filename: str, git_release_tag: str) -> int:
        logger.warning("GenericGitProvider does not yet support uploading a release to Git platforms other than GitHub")
        return 1

def get_generic_git_provider(parsed_uri: ParseResult) -> GenericGitProvider or None:
    # the only requirement we have is one of the path components ends in .git
    # this could be relaxed further 
    if parsed_uri.netloc and parsed_uri.scheme and parsed_uri.path:
        for element in parsed_uri.path.split('/'):
            if element.strip().endswith(".git"):
                return GenericGitProvider()
    return None
