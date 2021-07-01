#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

from o3de import manifest, utils, validation

logger = logging.getLogger()
logging.basicConfig()


def process_add_o3de_repo(file_name: str or pathlib.Path,
                          repo_set: set) -> int:
    file_name = pathlib.Path(file_name).resolve()
    if not validation.valid_o3de_repo_json(file_name):
        return 1

    cache_folder = manifest.get_o3de_cache_folder()

    with file_name.open('r') as f:
        try:
            repo_data = json.load(f)
        except json.JSONDecodeError as e:
            logger.error(f'{file_name} failed to load: {str(e)}')
            return 1

        for o3de_object_uris, manifest_json in [(repo_data['engines'], 'engine.json'),
                                                (repo_data['projects'], 'project.json'),
                                                (repo_data['gems'], 'gem.json'),
                                                (repo_data['template'], 'template.json'),
                                                (repo_data['restricted'], 'restricted.json')]:
            for o3de_object_uri in o3de_object_uris:
                manifest_json_uri = f'{o3de_object_uri}/{manifest_json}'
                manifest_json_sha256 = hashlib.sha256(manifest_json_uri.encode())
                cache_file = cache_folder / str(manifest_json_sha256.hexdigest() + '.json')
                if not cache_file.is_file():
                    parsed_uri = urllib.parse.urlparse(manifest_json_uri)
                    download_file_result = utils.download_file(parsed_uri, cache_file)
                    if download_file_result != 0:
                        return download_file_result

        repo_set |= repo_data['repos']
    return 0


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

            repo_uri = f'{repo_uri}/repo.json'
            repo_sha256 = hashlib.sha256(repo_uri.encode())
            cache_file = cache_folder / str(repo_sha256.hexdigest() + '.json')
            if not cache_file.is_file():
                parsed_uri = urllib.parse.urlparse(repo_uri)
                download_file_result = utils.download_file(parsed_uri, cache_file)
                if download_file_result != 0:
                    return download_file_result

                if not validation.valid_o3de_repo_json(cache_file):
                    logger.error(f'Repo json {repo_uri} is not valid.')
                    cache_file.unlink()
                    return 1

                last_failure = process_add_o3de_repo(cache_file, repo_set)
                if last_failure:
                    result = last_failure

    return result


def search_repo(repo_json_data: dict,
                engine_name: str = None,
                project_name: str = None,
                gem_name: str = None,
                template_name: str = None,
                restricted_name: str = None) -> dict or None:

    if isinstance(engine_name, str) or isinstance(engine_name, pathlib.PurePath):
        o3de_object_uris = repo_json_data['engines']
        manifest_json = 'engine.json'
        json_key = 'engine_name'
        search_func = lambda: None if manifest_json_data.get(json_key, '') == engine_name else manifest_json_data
    elif isinstance(project_name, str) or isinstance(project_name, pathlib.PurePath):
        o3de_object_uris = repo_json_data['projects']
        manifest_json = 'project.json'
        json_key = 'project_name'
        search_func = lambda: None if manifest_json_data.get(json_key, '') == project_name else manifest_json_data
    elif isinstance(gem_name, str) or isinstance(gem_name, pathlib.PurePath):
        o3de_object_uris = repo_json_data['gems']
        manifest_json = 'gem.json'
        json_key = 'gem_name'
        search_func = lambda: None if manifest_json_data.get(json_key, '') == gem_name else manifest_json_data
    elif isinstance(template_name, str) or isinstance(template_name, pathlib.PurePath):
        o3de_object_uris = repo_json_data['template']
        manifest_json = 'template.json'
        json_key = 'template_name'
        search_func = lambda: None if manifest_json_data.get(json_key, '') == template_name_name else manifest_json_data
    elif isinstance(restricted_name, str) or isinstance(restricted_name, pathlib.PurePath):
        o3de_object_uris = repo_json_data['restricted']
        manifest_json = 'restricted.json'
        json_key = 'restricted_name'
        search_func = lambda: None if manifest_json_data.get(json_key, '') == restricted_name else manifest_json_data
    else:
        return None

    o3de_object =  search_o3de_object(manifest_json, o3de_object_uris, search_func)
    if o3de_object:
        return o3de_object

    # recurse into the repos object to search for the o3de object
    o3de_object_uris = repo_json_data['repos']
    manifest_json = 'repo.json'
    search_func = lambda: search_repo(manifest_json, engine_name, project_name, gem_name, template_name)
    return search_o3de_object(manifest_json, o3de_object_uris, search_func)


def search_o3de_object(manifest_json, o3de_object_uris, search_func):
    # Search for the o3de object based on the supplied object name in the current repo
    cache_folder = manifest.get_o3de_cache_folder()
    for o3de_object_uri in o3de_object_uris:
        manifest_json_uri = f'{o3de_object_uri}/{manifest_json}'
        manifest_json_sha256 = hashlib.sha256(manifest_json_uri.encode())
        cache_file = cache_folder / str(manifest_json_sha256.hexdigest() + '.json')
        if cache_file.is_file():
            with cache_file.open('r') as f:
                try:
                    manifest_json_data = json.load(f)
                except json.JSONDecodeError as e:
                    logger.warn(f'{cache_file} failed to load: {str(e)}')
                else:
                    result_json_data = search_func()
                    if result_json_data:
                        return result_json_data
    return None
