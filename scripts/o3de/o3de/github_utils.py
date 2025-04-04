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
from urllib.parse import ParseResult
import urllib.request
import subprocess
import requests
import getpass
from o3de import gitproviderinterface, utils

LOG_FORMAT = '[%(levelname)s] %(name)s: %(message)s'

logger = logging.getLogger('o3de.github_utils')
logging.basicConfig(format=LOG_FORMAT)

class GitHubProvider(gitproviderinterface.GitProviderInterface):
    def get_specific_file_uri(self, parsed_uri: ParseResult):
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

    def clone_from_git(self, uri: ParseResult, download_path: pathlib.Path, force_overwrite: bool = False, ref: str = None) -> int:
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
        """
        Uploads a release asset to a GitHub repository. 
        It enables users to specify the GitHub release tag for the asset.
        :param repo_uri (str): The URL of the GitHub repository (e.g., "https://github.com/owner/repo.git").
        :param zip_path (pathlib.Path): The path to the ZIP file that needs to be uploaded.
        :param archive_filename (str): The filename to be used for the uploaded asset in the GitHub release.
        :param git_release_tag (str): The tag associated with the GitHub release.
        """
        access_token = getpass.getpass("Provide your GitHub access token (Must have content - read and write permission)\n"
                                        "Enter your GitHub Token (right click mouse to paste):")
        if len(access_token) == 0:
            logger.error('No input received! To paste your token into a terminal use mouse right click.')
            return 1
        tag_name = git_release_tag
        # Get GitHub credentials
        headers = {
            'Authorization': f'Token {access_token}',
            'Accept': 'application/vnd.github.v3+json',
            "Content-Type": "application/zip"
        }
        
        # Get owner (required)
        owner = response = requests.get('https://api.github.com/user', headers=headers)
        if response.status_code == 200:
            user_data = response.json()
            owner = user_data['login']
        else:
            logger.error(f'Failed to retrieve github user name. Status code: {response.status_code}')
            return 1
        # Get repo (required) repo_uri: https://github.com/<owner>/<repo>.git
        repo_uri = repo_uri.geturl().split("/")
        repo = repo_uri[-1].split(".git")[0]
        
        tag_check_url = f"https://api.github.com/repos/{owner}/{repo}/releases/tags/{tag_name}"
        response = requests.get(tag_check_url, headers=headers)
        
        release_payload = {
            'tag_name': tag_name
        }
        # Valid release
        if response.status_code == 200:
            # Get the release end point to retrieve release id
            get_release_by_tag = f"https://api.github.com/repos/{owner}/{repo}/releases/tags/{tag_name}"
            response = requests.get(get_release_by_tag, headers=headers, json=release_payload)
            release_id = response.json().get('id')
        
            if response.status_code == 200:
                upload_url = f'https://uploads.github.com/repos/{owner}/{repo}/releases/{release_id}/assets?name={archive_filename}'

                with open(zip_path, 'rb') as file:
                    response = requests.post(upload_url, headers=headers, data=file)
                    if not response.status_code == 201:
                        logger.error(f'Failed to upload asset to existing release. Status code: {response.status_code}')
                        return 1
            else:
                logger.error(f'Failed to retrieve release. Status code: {response.status_code}')
                return 1
            
        # Release doesn't exist; create a new release endpoint with the given tag_name and upload assests  
        elif response.status_code == 404:
            release_url = f"https://api.github.com/repos/{owner}/{repo}/releases"
            response = requests.post(release_url, headers=headers, json=release_payload)

            if response.status_code == 201:
                release_id = response.json().get('id')
                upload_url = f'https://uploads.github.com/repos/{owner}/{repo}/releases/{release_id}/assets?name={archive_filename}'

                with open(zip_path, 'rb') as file:
                    response = requests.post(upload_url, headers=headers, data=file)
                    if not response.status_code == 201:
                        logger.error(f'Failed to upload asset. Status code: {response.status_code}')
                        return 1
            else:
                logger.error(f'Failed to create release. Status code: {response.status_code}')
                return 1
        else:
            logger.error(f'Error checking tag existence. Status code: {response.status_code}')
            return 1

def get_github_provider(parsed_uri: ParseResult) -> GitHubProvider or None:
    if 'github.com' in parsed_uri.netloc:
        components = parsed_uri.path.split('/')
        components = [ele for ele in components if ele.strip()]

        if len(components) < 2:
            return None

        if components[1].endswith(".git"):
            return GitHubProvider()

    return None
