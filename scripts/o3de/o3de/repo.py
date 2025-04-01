#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import json
import logging
import pathlib
import urllib.parse
import urllib.request
import hashlib
from datetime import datetime, timezone
from o3de import manifest, utils, validation

logger = logging.getLogger('o3de.repo')
logging.basicConfig(format=utils.LOG_FORMAT)

REPO_IMPLICIT_SCHEMA_VERSION = "0.0.0"

REPO_SCHEMA_VERSION_1_0_0 = "1.0.0"

def get_cache_file_uri(uri: str):
    # check if the passed in uri is a path or uri
    uri_path = pathlib.Path(uri)
    if uri_path.exists():
        uri = uri_path.as_uri()

    parsed_uri = urllib.parse.urlparse(uri)
    uri_sha256 = hashlib.sha256(parsed_uri.geturl().encode())
    cache_file = manifest.get_o3de_cache_folder() / str(uri_sha256.hexdigest() + '.json')
    return cache_file, parsed_uri

def sanitized_repo_uri(repo_uri: str) -> str or None:
    # remove excess whitespace and any trailing slashes
    return repo_uri.strip().rstrip('/') if repo_uri else None

def get_repo_manifest_uri(repo_uri: str) -> str or None:
    if not repo_uri:
        logger.error(f'Repo URI cannot be empty.')
        return None

    return f'{repo_uri}/repo.json'

def repo_enabled(repo_json_data:dict) -> bool:
    # unless explicitely disabled assume enabled for backwards compatibility
    return repo_json_data.get('enabled', True)

def repo_uri_enabled(repo_uri: str) -> bool:
    repo_json_cache_file, _ = get_cache_file_uri(f'{repo_uri}/repo.json')

    repo_json_data = manifest.get_json_data_file(repo_json_cache_file, "repo", validation.valid_o3de_repo_json)
    if repo_json_data:
        return repo_enabled(repo_json_data)

    return False

def download_repo_manifest(manifest_uri: str, force_overwrite: bool = True) -> pathlib.Path or None:
    cache_file, parsed_uri = get_cache_file_uri(manifest_uri)

    git_provider = utils.get_git_provider(parsed_uri)
    if git_provider:
        parsed_uri = git_provider.get_specific_file_uri(parsed_uri)

    result = utils.download_file(parsed_uri, cache_file, force_overwrite)

    return cache_file if result == 0 else None

def download_object_manifests(repo_data: dict, download_missing_files_only: bool = False):

    if get_repo_schema_version(repo_data) == REPO_SCHEMA_VERSION_1_0_0:
        # schema version 1.0.0 includes all json data in repo.json
        return 0

    repo_object_type_manifests = [
        ('engines','engine.json'),
        ('projects','project.json'),
        ('gems','gem.json'),
        ('templates','template.json'),
        ('restricted','restricted.json')
        ]

    for key, manifest_json_filename in repo_object_type_manifests:
        for o3de_object_uri in repo_data.get(key, []):
            manifest_json_uri = f'{o3de_object_uri}/{manifest_json_filename}'
            cache_file, parsed_uri = get_cache_file_uri(manifest_json_uri)

            if not cache_file.exists() or not download_missing_files_only:
                git_provider = utils.get_git_provider(parsed_uri)
                if git_provider:
                    parsed_uri = git_provider.get_specific_file_uri(parsed_uri)

                download_file_result = utils.download_file(parsed_uri, cache_file, True)
                if download_file_result != 0:
                    return download_file_result
    return 0

def get_repo_schema_version(repo_data: dict):
    return repo_data.get("$schemaVersion", REPO_IMPLICIT_SCHEMA_VERSION)

def validate_remote_repo(repo_uri: str, validate_contained_objects: bool = False) -> bool:

    manifest_uri = get_repo_manifest_uri(repo_uri)
    if not manifest_uri:
        return False

    cache_file = download_repo_manifest(manifest_uri)
    if not cache_file:
        logger.error(f'Could not download file at {manifest_uri}')
        return False

    if not validation.valid_o3de_repo_json(cache_file):
        logger.error(f'Repository JSON {cache_file} could not be loaded or is missing required values')
        return False

    if validate_contained_objects:
        repo_data = {}
        with cache_file.open('r') as f:
            try:
                repo_data = json.load(f)
            except json.JSONDecodeError as e:
                logger.error(f'Invalid JSON - {cache_file} could not be loaded')
                return False
            
        repo_schema_version = get_repo_schema_version(repo_data)

        if repo_schema_version == REPO_IMPLICIT_SCHEMA_VERSION:
            if download_object_manifests(repo_data) != 0:
                # we don't issue an error message here because better error messaging is provided
                # in the download functions themselves
                return False
            cached_gems_json_data = get_gem_json_data_from_cached_repo(repo_uri)
            for gem_json_data in cached_gems_json_data:
                if not validation.valid_o3de_gem_json_data(gem_json_data):
                    logger.error(f'Invalid gem JSON - {gem_json_data} is missing required values')
                    return False
            cached_project_json_data = get_project_json_data_from_cached_repo(repo_uri)
            for project_json_data in cached_project_json_data:
                if not validation.valid_o3de_project_json_data(project_json_data):
                    logger.error(f'Invalid project JSON - {project_json_data} is missing required values')
                    return False
            cached_template_json_data = get_template_json_data_from_cached_repo(repo_uri)
            for template_json_data in cached_template_json_data:
                if not validation.valid_o3de_template_json_data(template_json_data):
                    logger.error(f'Invalid template JSON - {template_json_data} is missing required values')
                    return False
                
        elif repo_schema_version == REPO_SCHEMA_VERSION_1_0_0:
            gem_list = repo_data.get("gems_data", [])
            for gem_json in gem_list:
                if not validation.valid_o3de_gem_json_data(gem_json):
                    logger.error(f'Invalid gem JSON - {gem_json} is missing required values')
                    return False

                # validate versioning info
                if "versions_data" in gem_json:
                    versions_data = gem_json["versions_data"]
                    for version in versions_data:
                        if not all(key in version for key in ['version']):
                            logger.error("Invalid gem JSON - {gem_json} The version field must be defined for each entry in the versions_data field")
                            return False
                        
                        source_control_uri_defined = "source_control_uri" in version
                        download_source_uri_defined = "download_source_uri" in version
                        origin_uri_defined = "origin_uri" in version
                        
                        # prevent mixing fields intended for backwards compatibility (using XOR to verify)
                        download_origin_defined = ((download_source_uri_defined and not origin_uri_defined) or
                                                   (not download_source_uri_defined and origin_uri_defined))

                        if not (source_control_uri_defined or download_origin_defined):
                            logger.error(f"Invalid gem JSON - {gem_json} At least one of source_control_uri or download_source_uri must be defined")
                            return False

            project_list = repo_data.get("projects_data", [])
            for project_json in project_list:
                if not validation.valid_o3de_project_json_data(project_json):
                    logger.error(f'Invalid project JSON - {project_json} is missing required values')
                    return False                    

            template_list = repo_data.get("templates_data", [])
            for template_json in template_list:
                if not validation.valid_o3de_template_json_data(template_json):
                    logger.error(f'Invalid template JSON - {template_json} is missing required values')
                    return False

    return True

def process_add_o3de_repo(file_name: str or pathlib.Path,
                          repo_set: set,
                          download_missing_files_only: bool = False) -> int:
    file_name = pathlib.Path(file_name).resolve()
    if not validation.valid_o3de_repo_json(file_name):
        logger.error(f'Repository JSON {file_name} could not be loaded or is missing required values')
        return 1

    repo_data = {}
    with file_name.open('r') as f:
        try:
            repo_data = json.load(f)
        except json.JSONDecodeError as e:
            logger.error(f'{file_name} failed to load: {str(e)}')
            return 1

    with file_name.open('w') as f:
        try:
            # write the ISO8601 format which includes UTC offset
            # YYYY-MM-DDTHH:MM:SS.mmmmmmTZD  (e.g. 2012-03-29T10:05:45.12345+06:00)
            # TZD (time zone designator may have + or - indicating how far ahead or 
            # behind a time zone is from UTC)
            # because we are writing out UTC dates, the TZD will always be +00:00
            repo_data.update({'last_updated': datetime.now(timezone.utc).isoformat()})
            f.write(json.dumps(repo_data, indent=4) + '\n')
        except Exception as e:
            logger.error(f'{file_name} failed to save: {str(e)}')
            return 1

    if download_object_manifests(repo_data, download_missing_files_only) != 0:
        return 1

    # Having a repo is also optional
    repo_list = []
    repo_list.extend(repo_data.get('repos',[]))
    for repo in repo_list:
        if repo not in repo_set:
            repo_set.add(repo)
            repo_uri = f'{repo}/repo.json'
            cache_file, parsed_uri = get_cache_file_uri(repo_uri)
            
            if not cache_file.is_file() or not download_missing_files_only:
                download_file_result = utils.download_file(parsed_uri, cache_file, True)
                if download_file_result != 0:
                    return download_file_result

            return process_add_o3de_repo(cache_file, repo_set, download_missing_files_only)
    return 0

def get_object_versions_json_data(remote_object_list:list, required_json_key:str = None, required_json_value:str = None) -> list:
    """
    Convert a list of remote objects that may have 'versions_data', into a list
    of object json data with a separate entry for every entry in 'versions_data'
    or a single entry for every remote object that has no 'versions_data' entries
    :param remote_object_list The list of remote object json data
    :param required_json_key Optional required json key to look for in each object
    :param required_json_value Optional required value if required json key is specified
    """
    object_json_data_list = []
    for remote_object_json_data in remote_object_list:
        if required_json_key and remote_object_json_data.get(required_json_key, '') != required_json_value:
            continue

        versions_data = remote_object_json_data.pop('versions_data', None)
        if versions_data:
            version_found = False
            for version_json_data in versions_data:
                if remote_object_json_data.get('version') == version_json_data.get('version'):
                    version_found = True
                object_json_data_list.append(remote_object_json_data | version_json_data)
            if not version_found:
                object_json_data_list.append(remote_object_json_data)
        else:
            object_json_data_list.append(remote_object_json_data)

    return object_json_data_list

def get_object_json_data_from_cached_repo(repo_uri: str, repo_key: str, object_typename: str, object_validator, enabled_only = True) -> list:
    url = f'{repo_uri}/repo.json'
    cache_file, _ = get_cache_file_uri(url)

    o3de_object_json_data = list()

    file_name = pathlib.Path(cache_file).resolve()
    if not file_name.is_file():
        logger.info(f'Could not find cached repository json file for {repo_uri}, attempting to download')

        # attempt to download the missing repo.json
        cache_file = download_repo_manifest(url)
        if not cache_file:
            logger.error(f'Could not download the repository json file from {repo_uri}')
            return list()
        file_name = pathlib.Path(cache_file).resolve()
        if not file_name.is_file():
            logger.error(f'Could not download the repository json file from {repo_uri}')
            return list() 

    with file_name.open('r') as f:
        try:
            repo_data = json.load(f)
        except json.JSONDecodeError as e:
            logger.error(f'{file_name} failed to load: {str(e)}')
            return list()

        if enabled_only and not repo_enabled(repo_data):
            return list()

        repo_schema_version = get_repo_schema_version(repo_data)
        if repo_schema_version == REPO_IMPLICIT_SCHEMA_VERSION:        

            # Get list of objects, then add all json paths to the list if they exist in the cache
            repo_objects = []
            try:
                repo_objects.append((repo_data[repo_key], object_typename + '.json'))
            except KeyError:
                pass

            for o3de_object_uris, manifest_json in repo_objects:
                for o3de_object_uri in o3de_object_uris:
                    manifest_json_uri = f'{o3de_object_uri}/{manifest_json}'
                    cache_object_json_filepath, _ = get_cache_file_uri(manifest_json_uri)
                    
                    if not cache_object_json_filepath.is_file():
                        # attempt to download the missing file
                        cache_object_json_filepath = download_repo_manifest(manifest_json_uri)
                        if not cache_object_json_filepath:
                            logger.warning(f'Could not download the missing cached {repo_key} json file {cache_object_json_filepath} from {manifest_json_uri} in repo {repo_uri}')
                            continue

                    json_data = manifest.get_json_data_file(cache_object_json_filepath, object_typename, object_validator)
                    # validation errors will be logged via the function above
                    if json_data:
                        o3de_object_json_data.append(json_data)

        elif repo_schema_version == REPO_SCHEMA_VERSION_1_0_0:
            # the new schema version appends _data to the repo key
            # so it doesn't conflict with version 0.0.0 fields 
            repo_key = repo_key if repo_key.endswith('_data') else (repo_key + '_data')
            o3de_object_json_data.extend(get_object_versions_json_data(repo_data.get(repo_key,[])))

    return o3de_object_json_data

def get_gem_json_data_from_cached_repo(repo_uri: str, enabled_only: bool = True) -> list:
    return get_object_json_data_from_cached_repo(repo_uri, 'gems', 'gem', validation.valid_o3de_gem_json, enabled_only)

def get_gem_json_data_from_all_cached_repos(enabled_only: bool = True) -> list:
    gems_json_data = list()

    for repo_uri in manifest.get_manifest_repos():
        gems_json_data.extend(get_gem_json_data_from_cached_repo(repo_uri, enabled_only))

    return gems_json_data

def get_project_json_data_from_cached_repo(repo_uri: str, enabled_only: bool = True) -> list:
    return get_object_json_data_from_cached_repo(repo_uri, 'projects', 'project', validation.valid_o3de_project_json, enabled_only)

def get_project_json_data_from_all_cached_repos(enabled_only: bool = True) -> list:
    projects_json_data = list()

    for repo_uri in manifest.get_manifest_repos():
        projects_json_data.extend(get_project_json_data_from_cached_repo(repo_uri, enabled_only))

    return projects_json_data

def get_template_json_data_from_cached_repo(repo_uri: str, enabled_only: bool = True) -> list:
    return get_object_json_data_from_cached_repo(repo_uri, 'templates', 'template', validation.valid_o3de_template_json, enabled_only)

def get_template_json_data_from_all_cached_repos(enabled_only: bool = True) -> list:
    templates_json_data = list()

    for repo_uri in manifest.get_manifest_repos():
        templates_json_data.extend(get_template_json_data_from_cached_repo(repo_uri, enabled_only))

    return templates_json_data

def refresh_repo(repo_uri: str,
                 repo_set: set = None,
                 download_missing_files_only: bool = False) -> int:

    if not repo_uri_enabled(repo_uri):
        logger.info(f'Not refreshing {repo_uri} repo because it is deactivated.')
        return 0

    if not repo_set:
        repo_set = set()

    repo_uri = f'{repo_uri}/repo.json'
    cache_file, _ = get_cache_file_uri(repo_uri)
    if not cache_file.is_file() or not download_missing_files_only:
        cache_file = download_repo_manifest(repo_uri)
        if not cache_file:
            logger.error(f'Repo json {repo_uri} could not download.')
            return 1

    if not validation.valid_o3de_repo_json(cache_file):
        logger.error(f'Repo json {repo_uri} is not valid.')
        cache_file.unlink()
        return 1

    return process_add_o3de_repo(cache_file, repo_set, download_missing_files_only)

def refresh_repos(download_missing_files_only: bool = False) -> int:
    result = 0

    # set will stop circular references
    repo_set = set()

    for repo_uri in manifest.get_manifest_repos():
        if repo_uri not in repo_set:
            repo_set.add(repo_uri)

            last_failure = refresh_repo(repo_uri, repo_set, download_missing_files_only)
            if last_failure:
                result = last_failure

    return result


def search_repo(manifest_json_data: dict,
                engine_name: str = None,
                project_name: str = None,
                gem_name: str = None,
                template_name: str = None,
                restricted_name: str = None) -> dict or None:

    # don't search this repo if it isn't enabled
    if not repo_enabled(manifest_json_data):
        return None

    o3de_object = None

    repo_schema_version = get_repo_schema_version(manifest_json_data)

    if repo_schema_version == REPO_IMPLICIT_SCHEMA_VERSION:        
        if isinstance(engine_name, str):
            o3de_object = search_o3de_manifest_for_object(manifest_json_data, 'engines', 'engine.json', 'engine_name', engine_name)
        elif isinstance(project_name, str):
            o3de_object = search_o3de_manifest_for_object(manifest_json_data, 'projects', 'project.json', 'project_name', project_name)
        elif isinstance(gem_name, str):
            o3de_object = search_o3de_manifest_for_object(manifest_json_data, 'gems', 'gem.json', 'gem_name', gem_name)
        elif isinstance(template_name, str):
            o3de_object = search_o3de_manifest_for_object(manifest_json_data, 'templates', 'template.json', 'template_name', template_name)
        elif isinstance(restricted_name, str):
            o3de_object = search_o3de_manifest_for_object(manifest_json_data, 'restricted', 'restricted.json', 'restricted_name', restricted_name)
        else:
            return None
        
    elif repo_schema_version == REPO_SCHEMA_VERSION_1_0_0:
        #search for the o3de object from inside repos object 
        if isinstance(engine_name, str):
            o3de_object = search_o3de_repo_for_object(manifest_json_data, 'engines_data', 'engine_name', engine_name)
        elif isinstance(project_name, str):
            o3de_object = search_o3de_repo_for_object(manifest_json_data, 'projects_data', 'project_name', project_name)
        elif isinstance(gem_name, str):
            o3de_object = search_o3de_repo_for_object(manifest_json_data, 'gems_data', 'gem_name', gem_name)
        elif isinstance(template_name, str):
            o3de_object = search_o3de_repo_for_object(manifest_json_data, 'templates_data', 'template_name', template_name)
        elif isinstance(restricted_name, str):
            o3de_object = search_o3de_repo_for_object(manifest_json_data, 'restricted_data', 'restricted_name', restricted_name)
        else:
            return None
        
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


def search_o3de_repo_for_object(repo_json_data: dict, manifest_attribute:str, target_json_key:str, target_name: str):
    remote_candidates = repo_json_data.get(manifest_attribute, [])

    target_name_without_version_specifier, _ = utils.get_object_name_and_optional_version_specifier(target_name)

    # merge all versioned data into a list of candidates
    versioned_candidates = get_object_versions_json_data(remote_candidates, target_json_key, target_name_without_version_specifier)

    return manifest.get_most_compatible_object(object_name=target_name, name_key=target_json_key, objects=versioned_candidates)


def search_o3de_manifest_for_object(manifest_json_data: dict, manifest_attribute: str, target_manifest_json: str, target_json_key: str, target_name: str):
    o3de_object_uris = manifest_json_data.get(manifest_attribute, [])

    # load all the .json files and then find the most compatible object
    candidates = []
    for o3de_object_uri in o3de_object_uris:
        manifest_uri = f'{o3de_object_uri}/{target_manifest_json}'
        cache_file, _ = get_cache_file_uri(manifest_uri)
        if cache_file.is_file():
            with cache_file.open('r') as f:
                try:
                    manifest_json_data = json.load(f)
                except json.JSONDecodeError as e:
                    logger.warning(f'{cache_file} failed to load: {str(e)}')
                else:
                    candidates.append(manifest_json_data)

    return manifest.get_most_compatible_object(object_name=target_name, name_key=target_json_key, objects=candidates)


def search_o3de_object(manifest_json, o3de_object_uris, search_func):
    # Search for the o3de object based on the supplied object name in the current repo
    for o3de_object_uri in o3de_object_uris:
        manifest_uri = f'{o3de_object_uri}/{manifest_json}'
        cache_file, _ = get_cache_file_uri(manifest_uri)

        if cache_file.is_file():
            with cache_file.open('r') as f:
                try:
                    manifest_json_data = json.load(f)
                except json.JSONDecodeError as e:
                    logger.warning(f'{cache_file} failed to load: {str(e)}')
                else:
                    result_json_data = search_func(manifest_json_data)
                    if result_json_data:
                        return result_json_data
    return None

def set_repo_enabled(repo_uri:str, enabled:bool) -> int:
    repo_uri = f'{repo_uri}/repo.json'

    # avoid downloading if the file already exists and is valid
    repo_json_cache_file, _ = get_cache_file_uri(repo_uri)
    repo_json_data = manifest.get_json_data_file(repo_json_cache_file, "repo", validation.valid_o3de_repo_json)
    if not repo_json_data:
        # attempt to download the repo.json 
        repo_json_cache_file = download_repo_manifest(repo_uri)
        if not repo_json_cache_file.is_file():
            logger.error(f'{repo_json_cache_file} could not be downloaded')
            return 1

        repo_json_data = manifest.get_json_data_file(repo_json_cache_file, "repo", validation.valid_o3de_repo_json)
        if not repo_json_data:
            logger.error(f'Repository JSON {repo_json_cache_file} could not be loaded or is missing required values')
            repo_json_cache_file.unlink()
            return 1

    with repo_json_cache_file.open('w') as f:
        try:
            # write the ISO8601 format which includes UTC offset
            # YYYY-MM-DDTHH:MM:SS.mmmmmmTZD  (e.g. 2012-03-29T10:05:45.12345+06:00)
            # TZD (time zone designator may have + or - indicating how far ahead or 
            # behind a time zone is from UTC)
            # because we are writing out UTC dates, the TZD will always be +00:00
            repo_json_data.update({'last_updated': datetime.now(timezone.utc).isoformat()})
            repo_json_data.update({'enabled': enabled})
            f.write(json.dumps(repo_json_data, indent=4) + '\n')
        except Exception as e:
            logger.error(f'{repo_json_cache_file} failed to save: {str(e)}')
            return 1

    return 0


def _run_repo(args: argparse) -> int:
    if args.refresh_repo:
        return refresh_repo(args.refresh_repo)
    elif args.refresh_all_repos:
        return refresh_repos()
    elif args.activate_repo:
        return set_repo_enabled(args.activate_repo, True)
    elif args.deactivate_repo:
        return set_repo_enabled(args.deactivate_repo, False)

    return 1 
    

def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python print_registration.py --engine-projects
    :param parser: the caller passes an argparse parser like instance to this method
    """
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-ar', '--activate-repo', type=str, required=False,
                       help='Activate the specified remote repository, allowing searching and downloading of objects in it')
    group.add_argument('-dr', '--deactivate-repo', type=str, required=False,
                       help='Deactivate the specified remote repository, preventing searching or downloading any objects in it')
    group.add_argument('-r', '--refresh-repo', type=str, required=False,
                       help='Fetch the latest meta data the specified remote repository')
    group.add_argument('-ra', '--refresh-all-repos', action='store_true', required=False, default=False,
                       help='Fetch the latest meta data from all known remote repository')

    parser.set_defaults(func=_run_repo)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py repo --refresh https://path/to/remote/repo

    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    add_parser_args(subparsers.add_parser('repo'))
