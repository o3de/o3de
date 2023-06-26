#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
This file contains all the code that has to do with editing the remote repo template
"""
import argparse
import copy
import pathlib
import sys
import json
import logging
import hashlib
import shutil
from packaging.version import Version, InvalidVersion
from packaging.specifiers import SpecifierSet
from o3de import manifest, utils, validation

logger = logging.getLogger('o3de.repo_properties')
logging.basicConfig(format=utils.LOG_FORMAT)


def merge_json_data(json_path: pathlib.Path, json_data: dict) -> dict:
    """
    Merges the json data with the existing json data if it exists.
    If the json data does not exist, it is created.
    The json data is saved to the json path.
    Returns the merged json data.
    
    Args:
        json_path (pathlib.Path): The path to the json file.
        json_data (dict): The json data to merge with the existing json data.
    
    Returns:
        dict: The merged json data.
    
    Raises:
        FileNotFoundError: If the json path does not exist.
        ValueError: If the json data is not a dict.
    """
    merged_json_data = {}
    if json_path.exists():
        existing_json_data = {}
        with open(json_path, 'r') as f:
            existing_json_data = json.load(f)

        merged_json_data = existing_json_data | json_data
        
        if merged_json_data == existing_json_data and 'version' in existing_json_data:
            # no changes were made, avoid making unnecessary change to the file modification time
            return existing_json_data

        if not 'version' in merged_json_data:
            merged_json_data['version'] = '0.0.0'

        with open(json_path, 'w') as f:
            f.write(json.dumps(merged_json_data, indent=4) + '\n')

    return merged_json_data

def create_remote_object_archive(src_data_path: pathlib.Path, 
                   json_data_path: pathlib.Path, 
                   archive_filename: str, 
                   releases_path: pathlib.Path, 
                   repo_uri:str,
                   force: bool,
                   download_prefix:str) -> dict:
    """
    Creates a release of a specific version of a 
    remote object for the given src_data_path.
    The release is saved to the releases_path.
    Returns the json data. 
 
    Args:
        src_data_path (pathlib.Path): The path to the src root folder.
        json_data_path (pathlib.Path): The path to the json data.
        name (str): The object name.
        releases_path (pathlib.Path): The path where the release archive should be output.
        repo_uri (str): The uri of the repo.
        download_prefix (str): The prefix of the download uri.
    
    Returns:
        dict: The json data.
    
    Raises:
        FileNotFoundError: If the src_data_path does not exist.
        FileNotFoundError: If the json_data_path does not exist.
        ValueError: If the json_data_path is not a dict.
    """
    if download_prefix is None:
        logger.error('The --download-prefix argument must be provided. A url prefix for a file attached to a Github release might look like this:'
                     '-dp https://github.com/o3de/o3de-extras/releases/download/2305.0')
        return {}
    zip_path = releases_path / archive_filename
    # check if a object.zip folder already exist in the path - ask user if they want to overwrite the current zip
    if not force and zip_path.exists():
        logger.error(f'{zip_path} already exists.  Use --force command to overwrite the existing zip or '
                     'provide a new location to save your archive.')
        return {}
    else:
        # create the release zip file
        json_data = merge_json_data(json_data_path, {
            'repo_uri':repo_uri,
            'download_source_uri':f'{download_prefix}/{archive_filename}',
        })

        logging.info(f"Creating '{releases_path / archive_filename}'")

        shutil.make_archive(releases_path / pathlib.Path(archive_filename).stem, 'zip', src_data_path)
        with zip_path.open('rb') as f:
            json_data['sha256'] = hashlib.sha256(f.read()).hexdigest()

    return json_data
    
# retrieves json data of the repository from a file path
def get_repo_props(path: pathlib.Path) -> dict or None:
    repo_json = manifest.get_json_data_file(path, "Repo", validation.valid_o3de_repo_json)
    if not isinstance(repo_json, dict):
        logger.error(f'Could not retrieve repo.json file from {path} or the repo.json file is invalid.')
        return None
    return repo_json

def _find_index(data:list, key:str, value:str) -> int:
    for index, item in enumerate(data):
        if item.get(key) == value:
            return index
    return -1

def _changed(original:dict, new:dict) -> dict:
    changed = {}
    for key, value in new.items():
        if key not in original:
            changed[key] = value
        elif original[key] != value:
            changed[key] = value

    return changed

def _edit_objects(object_typename:str,
                  validator:callable,
                  repo_json: dict,
                  add_objects: pathlib.Path or list = None,
                  delete_objects: str or list = None,
                  replace_objects: pathlib.Path or list = None,
                  release_archive_path: pathlib.Path = None,
                  force: bool = None,
                  download_prefix: str = None):
    """
    Modifies the 'gems_data/projects_data/templates_data' in repo_json
    :param object_typename: The type object field you want to change
    :param validator: validates the object you want to edit
    :param add_objects: Any object paths you want to add to your repo_json object field
    :param delete_objects: Any object_names to be removed from your repo_json object field
    :param replace_objects: A list of object paths that will completely replace the current object_list
    :param release_archive_path: Optional local path to a folder where a release archive should be written
    :param download_prefix: The prefix of the download uri
    """
    # The beginning remote repo json template
    repo_objects_data = repo_json.get(f'{object_typename}s_data',[])
   
    if add_objects or replace_objects:
        if replace_objects:
            repo_objects_data = []
            paths = replace_objects.split() if isinstance(replace_objects, str) else replace_objects
        else:
            paths = add_objects.split() if isinstance(add_objects, str) else add_objects
        for object_path in paths:
            # object JSON data of the object you want to add
            json_data = manifest.get_json_data(object_typename, object_path, validator)
            if json_data:
                # for a project called TestProject that is version 1.0.0, 
                # --release_archive_path would create a filename of `testproject-1.0.0-project.zip`
                if release_archive_path:
                    object_path = pathlib.Path(object_path)
                    version = json_data.get('version','0.0.0')
                    archive_filename = f"{json_data[f'{object_typename}_name']}-{version}-{object_typename}.zip".lower()
                    json_data = create_remote_object_archive(object_path, 
                                object_path / f'{object_typename}.json', 
                                archive_filename, 
                                release_archive_path, 
                                repo_json['repo_uri'], 
                                force,
                                download_prefix)
                    # if create_remote_object_archive is not successful, then exit
                    if not json_data:
                        return 1
                    
                index = _find_index(repo_objects_data, f'{object_typename}_name', json_data[f'{object_typename}_name'])
                # index will be non-negative if an instance of this object already exists in the repo.json
                if index != -1:
                    # we want changes only
                    version_data = _changed(repo_objects_data[index], json_data)
                    if not version_data:
                        # there is no difference between the object being added and 
                        # what already exists in repo.json
                        logger.warning(f"No changes were found between the object JSON data in repo.json and the object data being added from {object_path}")
                        continue

                    # versions data must always include the version
                    version_data['version'] = json_data.get('version','0.0.0')
                    # versions data must always include one of the download/source uris
                    if not 'download_source_uri' in json_data and not 'source_control_uri' in json_data :
                        logger.warning('No download_source_uri found and sorce_control_uri found!')
                    if 'download_source_uri' in json_data:
                        version_data['download_source_uri'] = json_data['download_source_uri']
                    if 'source_control_uri' in json_data:
                        version_data['source_control_uri'] = json_data['source_control_uri']
                    if 'source_control_ref' in json_data:
                        version_data['source_control_ref'] = json_data['source_control_ref']

                    version_index = _find_index(repo_objects_data[index].get('versions_data',[]), 'version', json_data['version'])
                    if version_index != -1:
                        # update the existing version
                        if version_data:
                            repo_objects_data[index]['versions_data'][version_index] = version_data
                    # if version is same, update field                        
                    elif repo_objects_data[index].get('version','0.0.0') == version_data['version']:
                        repo_objects_data[index].update(version_data)
                    else:    
                        # add the new version
                        if 'versions_data' not in repo_objects_data[index]:
                            repo_objects_data[index]['versions_data'] = [version_data] 
                        else:
                            repo_objects_data[index]['versions_data'].append(version_data) 
                else:
                    repo_objects_data.append(json_data)

    if delete_objects:      
        removal_list = delete_objects.split() if isinstance(delete_objects, str) else delete_objects
        # find name field of object you want to delete
        object_key = str(f'{object_typename}_name')
        
        for removal_object in removal_list:
            # Remove the JSON object(s) with the specified object_name value
            object_name, version_specifier = utils.get_object_name_and_optional_version_specifier(removal_object)
            if not version_specifier:
                # Remove the JSON object(s) with the specified object_name value
                repo_objects_data = [object_typename for object_typename in repo_objects_data if object_typename.get(object_key) != object_name]
            else:
               index = _find_index(repo_objects_data, f'{object_typename}_name', object_name)
               #if version exists 
               if index > -1:
                   # returns the version data field associated object version data with input, if not return version data return empty field
                   versions_data = repo_objects_data[index].get('versions_data',[])
                   if versions_data:
                       # remove matching versions
                       versions_data = [data for data in versions_data if Version(data.get('version', '0.0.0')) not in SpecifierSet(version_specifier)]
                       repo_objects_data[index]['versions_data'] = versions_data
                              
                   # remove/replace base version if it matches
                   if Version(repo_objects_data[index].get('version', '0.0.0')) in SpecifierSet(version_specifier):
                       if versions_data:
                            # updates the base version with newer version
                            repo_objects_data[index].update(versions_data.pop(0))
                            repo_objects_data[index]['versions_data'] = versions_data
                       else: 
                            # versions data is empty so remove the whole object entry
                            del repo_objects_data[index]              

    repo_json[f'{object_typename}s_data'] = repo_objects_data

def edit_repo_props(repo_path: pathlib.Path = None,
                       repo_name: str = None,
                       add_gems: pathlib.Path or list = None,
                       delete_gems: str or list = None,
                       replace_gems: pathlib.Path or list = None,
                       add_projects: pathlib.Path or list = None,
                       delete_projects: str or list = None,
                       replace_projects: pathlib.Path or list = None,
                       add_templates: pathlib.Path or list = None,
                       delete_templates: str or list = None,
                       replace_templates: pathlib.Path or list = None,
                       release_archive_path: pathlib.Path = None,
                       force: bool = None,
                       download_prefix: str = None
                       ) -> int:
    """
    Edits and modifies the remote repo properties for the repo.json located at 'repo_path'.
    :param repo_path: The path to the repo.json file
    :param repo_name: The new name for the remote repo
    :param add_gems: Any gem paths to be added to the list
    :param delete_gems: Any gem names to be removed from the list
    :param replace_gems: A list of gem paths that will completely replace the current list of path
    :add_projects: Any project paths to be added to the list
    :delete_projects:  Any project names to be removed from the list
    :replace_projects: A list of project paths that will completely replace the current list of path
    :add_templates: Any template paths to be added to the list
    :delete_templates: Any template names to be removed from the list
    :replace_templates: A list of template paths that will completely replace the current list of path
    :release_archive_path: Path where you want your release to be located
    :download_prefix: The string prefix of the download uri
    """
    repo_json = get_repo_props(repo_path)

    if not repo_json:
        return 1

    repo_json_original = copy.deepcopy(repo_json)

    if isinstance(repo_name, str):
        if not utils.validate_identifier(repo_name):
            logger.error(f'Repo name must be fewer than 64 characters, contain only alphanumeric, "_" or "-"'
                         f' characters, and start with a letter.  {repo_name}')
            return 1
        repo_json['repo_name'] = repo_name

    if add_gems or delete_gems or replace_gems:
        _edit_objects('gem', validation.valid_o3de_gem_json, repo_json, add_gems, delete_gems, replace_gems, release_archive_path, force, download_prefix)

    if add_projects or delete_projects or replace_projects:
        _edit_objects('project', validation.valid_o3de_project_json, repo_json, add_projects, delete_projects, replace_projects, release_archive_path, force, download_prefix)

    if add_templates or delete_templates or replace_templates:
        _edit_objects('template', validation.valid_o3de_template_json, repo_json, add_templates, delete_templates, replace_templates, release_archive_path, force, download_prefix)

    if repo_json_original != repo_json:
        utils.backup_file(repo_path)

    return 0 if manifest.save_o3de_manifest(repo_json, repo_path) else 1


def _edit_repo_props(args: argparse) -> int:
    return edit_repo_props(repo_path=args.repo_path,
                              repo_name=args.repo_name,

                              add_gems=args.add_gems,
                              delete_gems=args.delete_gems,
                              replace_gems=args.replace_gems,

                              add_projects=args.add_projects,
                              delete_projects=args.delete_projects,
                              replace_projects=args.replace_projects,

                              add_templates=args.add_templates,
                              delete_templates=args.delete_templates,
                              replace_templates=args.replace_templates,

                              release_archive_path=args.release_archive_path,
                              force=args.force,
                              download_prefix=args.download_prefix
                              )


def add_parser_args(parser):
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--repo-path', '-rp', type=pathlib.Path, required=False,
                       help='The local path to the remote repository.')
    group = parser.add_argument_group('properties', 'arguments for modifying individual remote repo properties.')
    group.add_argument('--repo-name','-rn',  type=str, required=False,
                       help='The name of the remote repository.')
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('--add-gems', '-ag', type=pathlib.Path, nargs='*', required=False,
                       help="Adds gem(s) to the 'gems_data' property. Space delimited list (ex. -ag c:/gem1 c:/gem2)")
    group.add_argument('--delete-gems', '-dg', type=str, nargs='*', required=False,
                       help='Removes gems(s) from the gems_data property by gem name with optional version specifier.'
                       'Space delimited list (ex. -dg gemA==1.0.0 gemB gemC>2.0.0')
    group.add_argument('--replace-gems', '-rg', type=pathlib.Path, nargs='*', required=False,
                       help='Replace entirety of gems_data property with the provided gems')

    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('--add-templates', '-at', type=pathlib.Path, nargs='*', required=False,
                       help="Adds template(s) to the 'templates_data' property. Space delimited list (ex. -at c:/template1 c:/template2)")
    group.add_argument('--delete-templates', '-dt', type=str, nargs='*', required=False,
                       help='Removes templates(s) from the templates_data property by template name with optional version specifier.'
                        ' Space delimited list (ex. -dt templateA templateB==1.0.0 templateC>1.0.0')
    group.add_argument('--replace-templates', '-rt', type=pathlib.Path, nargs='*', required=False,
                       help='Replace entirety of templates_data property with the provided templates')

    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('--add-projects', '-apr', type=pathlib.Path, nargs='*', required=False,
                       help="Adds projects(s) to the 'projects_data' property. Space delimited list (ex. -at c:/project1 c:/project2)") 
    group.add_argument('--delete-projects', '-dpr', type=str, nargs='*', required=False,
                       help='Removes projects(s) from the projects_data property by project name with optional version specifier'
                       'Space delimited list (ex. -dpr projectA==1.0.0 projectB projectC>2.0.0')
    group.add_argument('--replace-projects', '-rpr', type=pathlib.Path, nargs='*', required=False,
                       help='Replace entirety of projects_data property with the provided projects')

    modify_gems_group = parser.add_argument_group(title='modify gems',
                                                  description='path arguments to use with the --add-gems or --replace-gems option')
    modify_gems_group.add_argument('--release-archive-path','-rap',  type=pathlib.Path, required=False,
                            help='Create a release archive at the specified local path and update the download_source_uri and sha256 fields.')
    modify_gems_group.add_argument('--force', '-f', action='store_true', default=False,
                            help='Overwrites the release-archive zip file if there is already an existing zip with the same name')
    modify_gems_group.add_argument('--download-prefix','-dp',  type=str, required=False,
                            help='a URL prefix for a file attached to a GitHub release might look like this:'
                            '-dp https://github.com/o3de/o3de-extras/releases/download/2305.0/')
    parser.set_defaults(func=_edit_repo_props)


def add_args(subparsers) -> None:
    enable_repo_props_subparser = subparsers.add_parser('edit-repo-properties')
    add_parser_args(enable_repo_props_subparser)


def main():
    the_parser = argparse.ArgumentParser()
    add_parser_args(the_parser)
    the_args = the_parser.parse_args()
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1
    sys.exit(ret)


if __name__ == "__main__":
    main()
