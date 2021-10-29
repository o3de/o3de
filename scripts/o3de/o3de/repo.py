#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import json
import logging
import pathlib
import shutil
import urllib.parse
import urllib.request
import hashlib
from datetime import datetime
from o3de import manifest, utils, validation

logger = logging.getLogger()
logging.basicConfig()


def process_add_o3de_repo(file_name: str or pathlib.Path,
                          repo_set: set) -> int:
    file_name = pathlib.Path(file_name).resolve()
    if not validation.valid_o3de_repo_json(file_name):
        return 1
    cache_folder = manifest.get_o3de_cache_folder()

    repo_data = {}
    with file_name.open('r') as f:
        try:
            repo_data = json.load(f)
        except json.JSONDecodeError as e:
            logger.error(f'{file_name} failed to load: {str(e)}')
            return 1

    with file_name.open('w') as f:
        try:
            time_now = datetime.now()
            # Convert to lower case because AM/PM is capitalized
            time_str = time_now.strftime('%d/%m/%Y %I:%M%p').lower()
            repo_data.update({'last_updated': time_str})
            f.write(json.dumps(repo_data, indent=4) + '\n')
        except Exception as e:
            logger.error(f'{file_name} failed to save: {str(e)}')
            return 1

    # A repo may not contain all types of object.
    manifest_download_list = []
    try:
        manifest_download_list.append((repo_data['engines'], 'engine.json'))
    except KeyError:
        pass
    try:
        manifest_download_list.append((repo_data['projects'], 'project.json'))
    except KeyError:
        pass
    try:
        manifest_download_list.append((repo_data['gems'], 'gem.json'))
    except KeyError:
        pass
    try:
        manifest_download_list.append((repo_data['templates'], 'template.json'))
    except KeyError:
        pass
    try:
        manifest_download_list.append((repo_data['restricted'], 'restricted.json'))
    except KeyError:
        pass

    for o3de_object_uris, manifest_json in manifest_download_list:
        for o3de_object_uri in o3de_object_uris:
            manifest_json_uri = f'{o3de_object_uri}/{manifest_json}'
            manifest_json_sha256 = hashlib.sha256(manifest_json_uri.encode())
            cache_file = cache_folder / str(manifest_json_sha256.hexdigest() + '.json')
            if not cache_file.is_file():
                parsed_uri = urllib.parse.urlparse(manifest_json_uri)
                download_file_result = utils.download_file(parsed_uri, cache_file)
                if download_file_result != 0:
                    return download_file_result

    # Having a repo is also optional
    repo_list = []
    try:
        repo_list.append(repo_data['repos'])
    except KeyError:
        pass

    for repo in repo_list:
        if repo not in repo_set:
            repo_set.add(repo)
            for o3de_object_uri in o3de_object_uris:
                parsed_uri = urllib.parse.urlparse(f'{repo}/repo.json')
                manifest_json_sha256 = hashlib.sha256(parsed_uri.geturl().encode())
                cache_file = cache_folder / str(manifest_json_sha256.hexdigest() + '.json')
                if cache_file.is_file():
                    cache_file.unlink()
                download_file_result = utils.download_file(parsed_uri, cache_file)
                if download_file_result != 0:
                    return download_file_result

                return process_add_o3de_repo(parsed_uri.geturl(), repo_set)
    return 0


def get_gem_json_paths_from_cached_repo(repo_uri: str) -> set:
    url = f'{repo_uri}/repo.json'
    repo_sha256 = hashlib.sha256(url.encode())
    cache_folder = manifest.get_o3de_cache_folder()
    cache_filename = cache_folder / str(repo_sha256.hexdigest() + '.json')

    gem_set = set()

    file_name = pathlib.Path(cache_filename).resolve()
    if not file_name.is_file():
        logger.error(f'Could not find cached repo json file for {repo_uri}')
        return gem_set

    with file_name.open('r') as f:
        try:
            repo_data = json.load(f)
        except json.JSONDecodeError as e:
            logger.error(f'{file_name} failed to load: {str(e)}')
            return gem_set

        # Get list of gems, then add all json paths to the list if they exist in the cache
        repo_gems = []
        try:
            repo_gems.append((repo_data['gems'], 'gem.json'))
        except KeyError:
            pass

        for o3de_object_uris, manifest_json in repo_gems:
            for o3de_object_uri in o3de_object_uris:
                manifest_json_uri = f'{o3de_object_uri}/{manifest_json}'
                manifest_json_sha256 = hashlib.sha256(manifest_json_uri.encode())
                cache_gem_json_filepath = cache_folder / str(manifest_json_sha256.hexdigest() + '.json')
                if cache_gem_json_filepath.is_file():
                    gem_set.add(cache_gem_json_filepath)
                else:
                    logger.warn(f'Could not find cached gem json file {cache_gem_json_filepath} for {o3de_object_uri} in repo {repo_uri}')

    return gem_set

def get_gem_json_paths_from_all_cached_repos() -> set:
    json_data = manifest.load_o3de_manifest()
    gem_set = set()

    for repo_uri in json_data['repos']:
        gem_set.update(get_gem_json_paths_from_cached_repo(repo_uri)) 

    return gem_set
    

def refresh_repo(repo_uri: str,
                 cache_folder: str = None,
                 repo_set: set = None) -> int:
    if not cache_folder:
        cache_folder = manifest.get_o3de_cache_folder()
    if not repo_set:
        repo_set = set()

    parsed_uri = urllib.parse.urlparse(f'{repo_uri}/repo.json')
    repo_sha256 = hashlib.sha256(parsed_uri.geturl().encode())
    cache_file = cache_folder / str(repo_sha256.hexdigest() + '.json')

    download_file_result = utils.download_file(parsed_uri, cache_file)
    if download_file_result != 0:
        return download_file_result

    if not validation.valid_o3de_repo_json(cache_file):
        logger.error(f'Repo json {repo_uri} is not valid.')
        cache_file.unlink()
        return 1

    return process_add_o3de_repo(cache_file, repo_set)

def refresh_repos() -> int:
    json_data = manifest.load_o3de_manifest()

    # clear the cache
    cache_folder = manifest.get_o3de_cache_folder()
    shutil.rmtree(cache_folder)
    cache_folder = manifest.get_o3de_cache_folder()  # will recreate it

    result = 0

    # set will stop circular references
    repo_set = set()

    for repo_uri in json_data['repos']:
        if repo_uri not in repo_set:
            repo_set.add(repo_uri)

            last_failure = refresh_repo(repo_uri, cache_folder, repo_set)
            if last_failure:
                result = last_failure

    return result


def search_repo(manifest_json_data: dict,
                engine_name: str = None,
                project_name: str = None,
                gem_name: str = None,
                template_name: str = None,
                restricted_name: str = None) -> dict or None:
    if isinstance(engine_name, str) or isinstance(engine_name, pathlib.PurePath):
        o3de_object_uris = manifest_json_data['engines']
        manifest_json = 'engine.json'
        json_key = 'engine_name'
        search_func = lambda manifest_json_data: manifest_json_data if manifest_json_data.get(json_key, '') == engine_name else None
    elif isinstance(project_name, str) or isinstance(project_name, pathlib.PurePath):
        o3de_object_uris = manifest_json_data['projects']
        manifest_json = 'project.json'
        json_key = 'project_name'
        search_func = lambda manifest_json_data: manifest_json_data if manifest_json_data.get(json_key, '') == project_name else None
    elif isinstance(gem_name, str) or isinstance(gem_name, pathlib.PurePath):
        o3de_object_uris = manifest_json_data['gems']
        manifest_json = 'gem.json'
        json_key = 'gem_name'
        search_func = lambda manifest_json_data: manifest_json_data if manifest_json_data.get(json_key, '') == gem_name else None
    elif isinstance(template_name, str) or isinstance(template_name, pathlib.PurePath):
        o3de_object_uris = manifest_json_data['template']
        manifest_json = 'template.json'
        json_key = 'template_name'
        search_func = lambda manifest_json_data: manifest_json_data if manifest_json_data.get(json_key, '') == template_name_name else None
    elif isinstance(restricted_name, str) or isinstance(restricted_name, pathlib.PurePath):
        o3de_object_uris = manifest_json_data['restricted']
        manifest_json = 'restricted.json'
        json_key = 'restricted_name'
        search_func = lambda manifest_json_data: manifest_json_data if manifest_json_data.get(json_key, '') == restricted_name else None
    else:
        return None
    o3de_object = search_o3de_object(manifest_json, o3de_object_uris, search_func)
    if o3de_object:
        o3de_object['repo_name'] = manifest_json_data['repo_name']
        return o3de_object

    # recurse into the repos object to search for the o3de object
    o3de_object_uris = []
    try:
        o3de_object_uris = manifest_json_data['repos']
    except KeyError:
        pass

    manifest_json = 'repo.json'
    search_func = lambda manifest_json_data: search_repo(manifest_json_data, engine_name, project_name, gem_name, template_name)
    return search_o3de_object(manifest_json, o3de_object_uris, search_func)


def search_o3de_object(manifest_json, o3de_object_uris, search_func):
    # Search for the o3de object based on the supplied object name in the current repo
    cache_folder = manifest.get_o3de_cache_folder()
    for o3de_object_uri in o3de_object_uris:
        parsed_uri = urllib.parse.urlparse(f'{o3de_object_uri}/{manifest_json}')
        manifest_json_sha256 = hashlib.sha256(parsed_uri.geturl().encode())
        cache_file = cache_folder / str(manifest_json_sha256.hexdigest() + '.json')
        if cache_file.is_file():
            with cache_file.open('r') as f:
                try:
                    manifest_json_data = json.load(f)
                except json.JSONDecodeError as e:
                    logger.warn(f'{cache_file} failed to load: {str(e)}')
                else:
                    result_json_data = search_func(manifest_json_data)
                    if result_json_data:
                        return result_json_data
    return None
