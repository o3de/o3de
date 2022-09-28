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

import json
import logging
import pathlib
import urllib.parse
import urllib.request
import subprocess

from o3de import gitproviderinterface

LOG_FORMAT = '[%(levelname)s] %(name)s: %(message)s'

logger = logging.getLogger('o3de.github_utils')
logging.basicConfig(format=LOG_FORMAT)

class GitHubProvider(gitproviderinterface.GitProviderInterface):
    def get_specific_file_uri(parsed_uri):
        components = parsed_uri.path.split('/')
        components = [ele for ele in components if ele.strip()]

        if len(components) < 3:
            return parsed_uri

        user = components[0]
        repository = components[1].replace('.git','')
        filepath = '/'.join(components[2:len(components)])
        api_url = f'http://api.github.com/repos/{user}/{repository}/contents/{filepath}'

        try:
            with urllib.request.urlopen(api_url) as url:
                json_data = json.loads(url.read().decode())
                download_url = json_data['download_url']
                parsed_uri = urllib.parse.urlparse(download_url)
        except urllib.error.HTTPError as e:
            logger.error(f'HTTP Error {e.code} opening {api_url.geturl()}')
        except urllib.error.URLError as e:
            logger.error(f'URL Error {e.reason} opening {api_url.geturl()}')

        return parsed_uri

    def clone_from_git(uri, download_path: pathlib.Path) -> int:
        """
        :param uri: uniform resource identifier
        :param download_path: location path on disk to download file
        """
        params = ["git", "clone", uri, download_path.as_posix()]
        try:
            with subprocess.Popen(params, stdout=subprocess.PIPE) as proc:
                print(proc.stdout.read())
        except Exception as e:
            logger.error(str(e))
            return 1

        return proc.returncode

def get_github_provider(parsed_uri) -> GitHubProvider or None:
    if 'github.com' in parsed_uri.netloc:
        components = parsed_uri.path.split('/')
        components = [ele for ele in components if ele.strip()]

        if len(components) < 2:
            return None

        if components[1].endswith(".git"):
            return GitHubProvider

    return None
