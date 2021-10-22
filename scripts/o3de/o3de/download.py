#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
Implements functionality for downloading o3de objects either locally or from a URI
"""

import argparse
import hashlib
import json
import logging
import os
import pathlib
import shutil
import sys
import urllib.parse
import urllib.request
import zipfile

from o3de import manifest, repo, utils, validation, register

logger = logging.getLogger()
logging.basicConfig()

def unzip_manifest_json_data(download_zip_path: pathlib.Path, zip_file_name: str) -> dict:
    json_data = {}
    with zipfile.ZipFile(download_zip_path, 'r') as zip_data:
        with zip_data.open(zip_file_name) as manifest_json_file:
            try:
                json_data = json.load(manifest_json_file)
            except json.JSONDecodeError as e:
                logger.error(f'UnZip exception:{str(e)}')

    return json_data

def validate_downloaded_zip_sha256(download_uri_json_data: dict, download_zip_path: pathlib.Path,
                                   manifest_json_name) -> int:
    # if the json has a sha256 check it against a sha256 of the zip
    try:
        sha256A = download_uri_json_data['sha256']
    except KeyError as e:
        logger.warn('SECURITY WARNING: The advertised o3de object you downloaded has no "sha256"!!! Be VERY careful!!!'
                    ' We cannot verify this is the actually the advertised object!!!')
        return 1
    else:
        sha256B = hashlib.sha256(download_zip_path.open('rb').read()).hexdigest()
        if sha256A != sha256B:
            logger.error(f'SECURITY VIOLATION: Downloaded zip sha256 {sha256B} does not match'
                         f' the advertised "sha256":{sha256A} in the f{manifest_json_name}.')
            return 0

    unzipped_manifest_json_data = unzip_manifest_json_data(download_zip_path, manifest_json_name)

    # remove the sha256 if present in the advertised downloadable manifest json
    # then compare it to the json in the zip, they should now be identical
    try:
        del download_uri_json_data['sha256']
    except KeyError as e:
        pass

    sha256A = hashlib.sha256(json.dumps(download_uri_json_data, indent=4).encode('utf8')).hexdigest()
    sha256B = hashlib.sha256(json.dumps(unzipped_manifest_json_data, indent=4).encode('utf8')).hexdigest()
    if sha256A != sha256B:
        logger.error('SECURITY VIOLATION: Downloaded manifest json does not match'
                     ' the advertised manifest json.')
        return 0

    return 1


def get_downloadable(engine_name: str = None,
                     project_name: str = None,
                     gem_name: str = None,
                     template_name: str = None,
                     restricted_name: str = None) -> dict or None:
    json_data = manifest.load_o3de_manifest()
    try:
        o3de_object_uris = json_data['repos']
    except KeyError as key_err:
        logger.error(f'Unable to load repos from o3de manifest: {str(key_err)}')
        return None

    manifest_json = 'repo.json'
    search_func = lambda manifest_json_data: repo.search_repo(manifest_json_data, engine_name, project_name, gem_name, template_name)
    return repo.search_o3de_object(manifest_json, o3de_object_uris, search_func)


def download_o3de_object(object_name: str, default_folder_name: str, dest_path: str or pathlib.Path,
                         object_type: str, downloadable_kwarg_key, skip_auto_register: bool,
                         download_progress_callback = None) -> int:

    download_path = manifest.get_o3de_cache_folder() / default_folder_name / object_name
    download_path.mkdir(parents=True, exist_ok=True)
    download_zip_path = download_path / f'{object_type}.zip'

    downloadable_object_data = get_downloadable(**{downloadable_kwarg_key : object_name})
    if not downloadable_object_data:
        logger.error(f'Downloadable o3de object {object_name} not found.')
        return 1

    origin_uri = downloadable_object_data['originuri']
    parsed_uri = urllib.parse.urlparse(origin_uri)

    download_zip_result = utils.download_zip_file(parsed_uri, download_zip_path, download_progress_callback)
    if download_zip_result != 0:
        return download_zip_result

    if not validate_downloaded_zip_sha256(downloadable_object_data, download_zip_path, f'{object_type}.json'):
        logger.error(f'Could not validate zip, deleting {download_zip_path}')
        os.unlink(download_zip_path)
        return 1

    if not dest_path:
        dest_path = manifest.get_registered(default_folder=default_folder_name)
        dest_path = pathlib.Path(dest_path).resolve()
        dest_path = dest_path / object_name
    else:
        dest_path = pathlib.Path(dest_path).resolve()

    if not dest_path:
        logger.error(f'Destination path cannot be empty.')
        return 1
    if dest_path.exists():
        logger.error(f'Destination path {dest_path} already exists.')
        return 1

    dest_path.mkdir(exist_ok=True)

    # extract zip
    with zipfile.ZipFile(download_zip_path, 'r') as zip_file_ref:
        try:
            zip_file_ref.extractall(dest_path)
        except Exception:
            logger.error(f'Error unzipping {download_zip_path} to {dest_path}. Deleting {dest_path}.')
            shutil.rmtree(dest_path)
            return 1

    if not skip_auto_register:
        if object_type == 'gem':
            return register.register(gem_path=dest_path)

    return 0


def download_engine(engine_name: str,
                    dest_path: str or pathlib.Path,
                    skip_auto_register: bool,
                    download_progress_callback = None) -> int:
    return download_o3de_object(engine_name, 'engines', dest_path, 'engine', 'engine_name', skip_auto_register, download_progress_callback)


def download_project(project_name: str,
                     dest_path: str or pathlib.Path,
                     skip_auto_register: bool,
                     download_progress_callback = None) -> int:
    return download_o3de_object(project_name, 'projects', dest_path, 'project', 'project_name', skip_auto_register, download_progress_callback)


def download_gem(gem_name: str,
                 dest_path: str or pathlib.Path,
                 skip_auto_register: bool,
                 download_progress_callback = None) -> int:
    return download_o3de_object(gem_name, 'gems', dest_path, 'gem', 'gem_name', skip_auto_register, download_progress_callback)


def download_template(template_name: str,
                      dest_path: str or pathlib.Path,
                      skip_auto_register: bool,
                      download_progress_callback = None) -> int:
    return download_o3de_object(template_name, 'templates', dest_path, 'template', 'template_name', skip_auto_register, download_progress_callback)



def download_restricted(restricted_name: str,
                        dest_path: str or pathlib.Path,
                        skip_auto_register: bool,
                        download_progress_callback = None) -> int:
    return download_o3de_object(restricted_name, 'restricted', dest_path, 'restricted', 'restricted_name', skip_auto_register, download_progress_callback)


def _run_download(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    if args.engine_name:
        return download_engine(args.engine_name,
                               args.dest_path,
                               args.skip_auto_register)
    elif args.project_name:
        return download_project(args.project_name,
                                args.dest_path,
                               args.skip_auto_register)
    elif args.gem_name:
        return download_gem(args.gem_name,
                            args.dest_path,
                               args.skip_auto_register)
    elif args.template_name:
        return download_template(args.template_name,
                                 args.dest_path,
                               args.skip_auto_register)

    return 1

def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python download.py --engine-name "o3de"
    :param parser: the caller passes an argparse parser like instance to this method
    """
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-e', '--engine-name', type=str, required=False,
                       help='Downloadable engine name.')
    group.add_argument('-p', '--project-name', type=str, required=False,
                       help='Downloadable project name.')
    group.add_argument('-g', '--gem-name', type=str, required=False,
                       help='Downloadable gem name.')
    group.add_argument('-t', '--template-name', type=str, required=False,
                       help='Downloadable template name.')
    parser.add_argument('-dp', '--dest-path', type=str, required=False,
                            default=None,
                            help='Optional destination folder to download into.'
                                 ' i.e. download --project-name "CustomProject" --dest-path "C:/projects"'
                                 ' will result in C:/projects/CustomProject'
                                 ' If blank will download to default object type folder')
    parser.add_argument('-sar', '--skip-auto-register', action='store_true', required=False,
                            default=False,
                            help = 'Skip the automatic registration of new object download')
    parser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                            help='By default the home folder is the user folder, override it to this folder.')

    parser.set_defaults(func=_run_download)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py download --engine-name "o3de"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    download_subparser = subparsers.add_parser('download')
    add_parser_args(download_subparser)


def main():
    """
    Runs download.py script as standalone script
    """
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers

    # add args to the parser
    add_parser_args(the_parser)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1

    # return
    sys.exit(ret)


# Do not allow running the download.py script as a standalone script until it is reviewed by app-sec
