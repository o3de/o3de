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

import json
import logging
import pathlib
import shutil
import urllib.parse
import urllib.request

from o3de import manifest, validation

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
        except Exception as e:
            logger.error(f'{file_name} failed to load: {str(e)}')
            return 1

        for engine_uri in repo_data['engines']:
            engine_uri = f'{engine_uri}/engine.json'
            engine_sha256 = hashlib.sha256(engine_uri.encode())
            cache_file = cache_folder / str(engine_sha256.hexdigest() + '.json')
            if not cache_file.is_file():
                parsed_uri = urllib.parse.urlparse(engine_uri)
                if parsed_uri.scheme == 'http' or \
                        parsed_uri.scheme == 'https' or \
                        parsed_uri.scheme == 'ftp' or \
                        parsed_uri.scheme == 'ftps':
                    with urllib.request.urlopen(engine_uri) as s:
                        with cache_file.open('wb') as f:
                            shutil.copyfileobj(s, f)
                else:
                    engine_json = pathlib.Path(engine_uri).resolve()
                    if not engine_json.is_file():
                        return 1
                    shutil.copy(engine_json, cache_file)

        for project_uri in repo_data['projects']:
            project_uri = f'{project_uri}/project.json'
            project_sha256 = hashlib.sha256(project_uri.encode())
            cache_file = cache_folder / str(project_sha256.hexdigest() + '.json')
            if not cache_file.is_file():
                parsed_uri = urllib.parse.urlparse(project_uri)
                if parsed_uri.scheme == 'http' or \
                        parsed_uri.scheme == 'https' or \
                        parsed_uri.scheme == 'ftp' or \
                        parsed_uri.scheme == 'ftps':
                    with urllib.request.urlopen(project_uri) as s:
                        with cache_file.open('wb') as f:
                            shutil.copyfileobj(s, f)
                else:
                    project_json = pathlib.Path(project_uri).resolve()
                    if not project_json.is_file():
                        return 1
                    shutil.copy(project_json, cache_file)

        for gem_uri in repo_data['gems']:
            gem_uri = f'{gem_uri}/gem.json'
            gem_sha256 = hashlib.sha256(gem_uri.encode())
            cache_file = cache_folder / str(gem_sha256.hexdigest() + '.json')
            if not cache_file.is_file():
                parsed_uri = urllib.parse.urlparse(gem_uri)
                if parsed_uri.scheme == 'http' or \
                        parsed_uri.scheme == 'https' or \
                        parsed_uri.scheme == 'ftp' or \
                        parsed_uri.scheme == 'ftps':
                    with urllib.request.urlopen(gem_uri) as s:
                        with cache_file.open('wb') as f:
                            shutil.copyfileobj(s, f)
                else:
                    gem_json = pathlib.Path(gem_uri).resolve()
                    if not gem_json.is_file():
                        return 1
                    shutil.copy(gem_json, cache_file)

        for template_uri in repo_data['templates']:
            template_uri = f'{template_uri}/template.json'
            template_sha256 = hashlib.sha256(template_uri.encode())
            cache_file = cache_folder / str(template_sha256.hexdigest() + '.json')
            if not cache_file.is_file():
                parsed_uri = urllib.parse.urlparse(template_uri)
                if parsed_uri.scheme == 'http' or \
                        parsed_uri.scheme == 'https' or \
                        parsed_uri.scheme == 'ftp' or \
                        parsed_uri.scheme == 'ftps':
                    with urllib.request.urlopen(template_uri) as s:
                        with cache_file.open('wb') as f:
                            shutil.copyfileobj(s, f)
                else:
                    template_json = pathlib.Path(template_uri).resolve()
                    if not template_json.is_file():
                        return 1
                    shutil.copy(template_json, cache_file)

        for repo_uri in repo_data['repos']:
            if repo_uri not in repo_set:
                repo_set.add(repo_uri)
                repo_uri = f'{repo_uri}/repo.json'
                repo_sha256 = hashlib.sha256(repo_uri.encode())
                cache_file = cache_folder / str(repo_sha256.hexdigest() + '.json')
                if not cache_file.is_file():
                    if parsed_uri.scheme == 'http' or \
                            parsed_uri.scheme == 'https' or \
                            parsed_uri.scheme == 'ftp' or \
                            parsed_uri.scheme == 'ftps':
                        with urllib.request.urlopen(repo_uri) as s:
                            with cache_file.open('wb') as f:
                                shutil.copyfileobj(s, f)
                    else:
                        repo_json = pathlib.Path(repo_uri).resolve()
                        if not repo_json.is_file():
                            return 1
                        shutil.copy(repo_json, cache_file)
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
                if parsed_uri.scheme == 'http' or \
                        parsed_uri.scheme == 'https' or \
                        parsed_uri.scheme == 'ftp' or \
                        parsed_uri.scheme == 'ftps':
                    with urllib.request.urlopen(repo_uri) as s:
                        with cache_file.open('wb') as f:
                            shutil.copyfileobj(s, f)
                else:
                    origin_file = pathlib.Path(repo_uri).resolve()
                    if not origin_file.is_file():
                        return 1
                    shutil.copy(origin_file, cache_file)

                if not validation.valid_o3de_repo_json(cache_file):
                    logger.error(f'Repo json {repo_uri} is not valid.')
                    cache_file.unlink()
                    return 1

                last_failure = process_add_o3de_repo(cache_file, repo_set)
                if last_failure:
                    result = last_failure

    return result


def search_repo(repo_set: set,
                repo_json_data: dict,
                engine_name: str = None,
                project_name: str = None,
                gem_name: str = None,
                template_name: str = None,
                restricted_name: str = None) -> dict or None:
    cache_folder = manifest.get_o3de_cache_folder()

    if isinstance(engine_name, str) or isinstance(engine_name, pathlib.PurePath):
        for engine_uri in repo_json_data['engines']:
            engine_uri = f'{engine_uri}/engine.json'
            engine_sha256 = hashlib.sha256(engine_uri.encode())
            engine_cache_file = cache_folder / str(engine_sha256.hexdigest() + '.json')
            if engine_cache_file.is_file():
                with engine_cache_file.open('r') as f:
                    try:
                        engine_json_data = json.load(f)
                    except Exception as e:
                        logger.warn(f'{engine_cache_file} failed to load: {str(e)}')
                    else:
                        if engine_json_data['engine_name'] == engine_name:
                            return engine_json_data

    elif isinstance(project_name, str) or isinstance(project_name, pathlib.PurePath):
        for project_uri in repo_json_data['projects']:
            project_uri = f'{project_uri}/project.json'
            project_sha256 = hashlib.sha256(project_uri.encode())
            project_cache_file = cache_folder / str(project_sha256.hexdigest() + '.json')
            if project_cache_file.is_file():
                with project_cache_file.open('r') as f:
                    try:
                        project_json_data = json.load(f)
                    except Exception as e:
                        logger.warn(f'{project_cache_file} failed to load: {str(e)}')
                    else:
                        if project_json_data['project_name'] == project_name:
                            return project_json_data

    elif isinstance(gem_name, str) or isinstance(gem_name, pathlib.PurePath):
        for gem_uri in repo_json_data['gems']:
            gem_uri = f'{gem_uri}/gem.json'
            gem_sha256 = hashlib.sha256(gem_uri.encode())
            gem_cache_file = cache_folder / str(gem_sha256.hexdigest() + '.json')
            if gem_cache_file.is_file():
                with gem_cache_file.open('r') as f:
                    try:
                        gem_json_data = json.load(f)
                    except Exception as e:
                        logger.warn(f'{gem_cache_file} failed to load: {str(e)}')
                    else:
                        if gem_json_data['gem_name'] == gem_name:
                            return gem_json_data

    elif isinstance(template_name, str) or isinstance(template_name, pathlib.PurePath):
        for template_uri in repo_json_data['templates']:
            template_uri = f'{template_uri}/template.json'
            template_sha256 = hashlib.sha256(template_uri.encode())
            template_cache_file = cache_folder / str(template_sha256.hexdigest() + '.json')
            if template_cache_file.is_file():
                with template_cache_file.open('r') as f:
                    try:
                        template_json_data = json.load(f)
                    except Exception as e:
                        logger.warn(f'{template_cache_file} failed to load: {str(e)}')
                    else:
                        if template_json_data['template_name'] == template_name:
                            return template_json_data

    elif isinstance(restricted_name, str) or isinstance(restricted_name, pathlib.PurePath):
        for restricted_uri in repo_json_data['restricted']:
            restricted_uri = f'{restricted_uri}/restricted.json'
            restricted_sha256 = hashlib.sha256(restricted_uri.encode())
            restricted_cache_file = cache_folder / str(restricted_sha256.hexdigest() + '.json')
            if restricted_cache_file.is_file():
                with restricted_cache_file.open('r') as f:
                    try:
                        restricted_json_data = json.load(f)
                    except Exception as e:
                        logger.warn(f'{restricted_cache_file} failed to load: {str(e)}')
                    else:
                        if restricted_json_data['restricted_name'] == restricted_name:
                            return restricted_json_data
    # recurse
    else:
        for repo_repo_uri in repo_json_data['repos']:
            if repo_repo_uri not in repo_set:
                repo_set.add(repo_repo_uri)
                repo_repo_uri = f'{repo_repo_uri}/repo.json'
                repo_repo_sha256 = hashlib.sha256(repo_repo_uri.encode())
                repo_repo_cache_file = cache_folder / str(repo_repo_sha256.hexdigest() + '.json')
                if repo_repo_cache_file.is_file():
                    with repo_repo_cache_file.open('r') as f:
                        try:
                            repo_repo_json_data = json.load(f)
                        except Exception as e:
                            logger.warn(f'{repo_repo_cache_file} failed to load: {str(e)}')
                        else:
                            item = search_repo(repo_set,
                                               repo_repo_json_data,
                                               engine_name,
                                               project_name,
                                               gem_name,
                                               template_name)
                            if item:
                                return item
    return None

