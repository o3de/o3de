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
"""
This file contains  functions for querying paths from  ~/.o3de directory
"""

import argparse
import hashlib
import json
import logging
import pathlib
import shutil
import urllib.parse
import urllib.request

from o3de import manifest, utils, validation

logger = logging.getLogger()
logging.basicConfig()

def download_engine(engine_name: str,
                    dest_path: str) -> int:
    if not dest_path:
        dest_path = manifest.get_registered(default_folder='engines')
        if not dest_path:
            logger.error(f'Destination path not cannot be empty.')
            return 1

    dest_path = pathlib.Path(dest_path).resolve()
    dest_path.mkdir(exist_ok=True)

    download_path = manifest.get_o3de_download_folder() / 'engines' / engine_name
    download_path.mkdir(exist_ok=True)
    download_zip_path = download_path / 'engine.zip'

    downloadable_engine_data = get_downloadable(engine_name=engine_name)
    if not downloadable_engine_data:
        logger.error(f'Downloadable engine {engine_name} not found.')
        return 1

    origin = downloadable_engine_data['origin']
    url = f'{origin}/project.zip'
    parsed_uri = urllib.parse.urlparse(url)

    if download_zip_path.is_file():
        logger.warn(f'Project already downloaded to {download_zip_path}.')
    elif parsed_uri.scheme == 'http' or \
            parsed_uri.scheme == 'https' or \
            parsed_uri.scheme == 'ftp' or \
            parsed_uri.scheme == 'ftps':
        with urllib.request.urlopen(url) as s:
            with download_zip_path.open('wb') as f:
                shutil.copyfileobj(s, f)
    else:
        origin_file = pathlib.Path(url).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_zip_path)

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"Engine zip {download_zip_path} is invalid.")
        download_zip_path.unlink()
        return 1

    # if the engine.json has a sha256 check it against a sha256 of the zip
    try:
        sha256A = downloadable_engine_data['sha256']
    except Exception as e:
        logger.warn(f'SECURITY WARNING: The advertised engine you downloaded has no "sha256"!!! Be VERY careful!!!'
                    f' We cannot verify this is the actually the advertised engine!!!')
    else:
        sha256B = hashlib.sha256(download_zip_path.open('rb').read()).hexdigest()
        if sha256A != sha256B:
            logger.error(f'SECURITY VIOLATION: Downloaded engine.zip sha256 {sha256B} does not match'
                         f' the advertised "sha256":{sha256A} in the engine.json. Deleting unzipped files!!!')
            shutil.rmtree(dest_path)
            return 1

    dest_engine_folder = dest_path / engine_name
    if dest_engine_folder.is_dir():
        utils.backup_folder(dest_engine_folder)
    with zipfile.ZipFile(download_zip_path, 'r') as project_zip:
        try:
            project_zip.extractall(dest_path)
        except Exception as e:
            logger.error(f'UnZip exception:{str(e)}')
            shutil.rmtree(dest_path)
            return 1

    unzipped_engine_json = dest_engine_folder / 'engine.json'
    if not unzipped_engine_json.is_file():
        logger.error(f'Engine json {unzipped_engine_json} is missing.')
        return 1

    if not validation.valid_o3de_engine_json(unzipped_engine_json):
        logger.error(f'Engine json {unzipped_engine_json} is invalid.')
        return 1

    # remove the sha256 if present in the advertised downloadable engine.json
    # then compare it to the engine.json in the zip, they should now be identical
    try:
        del downloadable_engine_data['sha256']
    except Exception as e:
        pass

    sha256A = hashlib.sha256(json.dumps(downloadable_engine_data, indent=4).encode('utf8')).hexdigest()
    with unzipped_engine_json.open('r') as s:
        try:
            unzipped_engine_json_data = json.load(s)
        except Exception as e:
            logger.error(f'Failed to read engine json {unzipped_engine_json}. Unable to confirm this'
                         f' is the same template that was advertised.')
            return 1
    sha256B = hashlib.sha256(json.dumps(unzipped_engine_json_data, indent=4).encode('utf8')).hexdigest()
    if sha256A != sha256B:
        logger.error(f'SECURITY VIOLATION: Downloaded engine.json does not match'
                     f' the advertised engine.json. Deleting unzipped files!!!')
        shutil.rmtree(dest_path)
        return 1

    return 0


def download_project(project_name: str,
                     dest_path: str or pathlib.Path) -> int:
    if not dest_path:
        dest_path = manifest.get_registered(default_folder='projects')
        if not dest_path:
            logger.error(f'Destination path not specified and not default projects path.')
            return 1

    dest_path = pathlib.Path(dest_path).resolve()
    dest_path.mkdir(exist_ok=True, parents=True)

    download_path = manifest.get_o3de_download_folder() / 'projects' / project_name
    download_path.mkdir(exist_ok=True, parents=True)
    download_zip_path = download_path / 'project.zip'

    downloadable_project_data = get_downloadable(project_name=project_name)
    if not downloadable_project_data:
        logger.error(f'Downloadable project {project_name} not found.')
        return 1

    origin = downloadable_project_data['origin']
    url = f'{origin}/project.zip'
    parsed_uri = urllib.parse.urlparse(url)

    if download_zip_path.is_file():
        logger.warn(f'Project already downloaded to {download_zip_path}.')
    elif parsed_uri.scheme == 'http' or \
            parsed_uri.scheme == 'https' or \
            parsed_uri.scheme == 'ftp' or \
            parsed_uri.scheme == 'ftps':
        with urllib.request.urlopen(url) as s:
            with download_zip_path.open('wb') as f:
                shutil.copyfileobj(s, f)
    else:
        origin_file = pathlib.Path(url).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_zip_path)

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"Project zip {download_zip_path} is invalid.")
        download_zip_path.unlink()
        return 1

    # if the project.json has a sha256 check it against a sha256 of the zip
    try:
        sha256A = downloadable_project_data['sha256']
    except Exception as e:
        logger.warn(f'SECURITY WARNING: The advertised project you downloaded has no "sha256"!!! Be VERY careful!!!'
                    f' We cannot verify this is the actually the advertised project!!!')
    else:
        sha256B = hashlib.sha256(download_zip_path.open('rb').read()).hexdigest()
        if sha256A != sha256B:
            logger.error(f'SECURITY VIOLATION: Downloaded project.zip sha256 {sha256B} does not match'
                         f' the advertised "sha256":{sha256A} in the project.json. Deleting unzipped files!!!')
            shutil.rmtree(dest_path)
            return 1

    dest_project_folder = dest_path / project_name
    if dest_project_folder.is_dir():
        utils.backup_folder(dest_project_folder)
    with zipfile.ZipFile(download_zip_path, 'r') as project_zip:
        try:
            project_zip.extractall(dest_project_folder)
        except Exception as e:
            logger.error(f'UnZip exception:{str(e)}')
            shutil.rmtree(dest_path)
            return 1

    unzipped_project_json = dest_project_folder / 'project.json'
    if not unzipped_project_json.is_file():
        logger.error(f'Project json {unzipped_project_json} is missing.')
        return 1

    if not validation.valid_o3de_project_json(unzipped_project_json):
        logger.error(f'Project json {unzipped_project_json} is invalid.')
        return 1

    # remove the sha256 if present in the advertised downloadable project.json
    # then compare it to the project.json in the zip, they should now be identical
    try:
        del downloadable_project_data['sha256']
    except Exception as e:
        pass

    sha256A = hashlib.sha256(json.dumps(downloadable_project_data, indent=4).encode('utf8')).hexdigest()
    with unzipped_project_json.open('r') as s:
        try:
            unzipped_project_json_data = json.load(s)
        except Exception as e:
            logger.error(f'Failed to read Project json {unzipped_project_json}. Unable to confirm this'
                         f' is the same project that was advertised.')
            return 1
    sha256B = hashlib.sha256(json.dumps(unzipped_project_json_data, indent=4).encode('utf8')).hexdigest()
    if sha256A != sha256B:
        logger.error(f'SECURITY VIOLATION: Downloaded project.json does not match'
                     f' the advertised project.json. Deleting unzipped files!!!')
        shutil.rmtree(dest_path)
        return 1

    return 0


def download_gem(gem_name: str,
                 dest_path: str or pathlib.Path) -> int:
    if not dest_path:
        dest_path = manifest.get_registered(default_folder='gems')
        if not dest_path:
            logger.error(f'Destination path not cannot be empty.')
            return 1

    dest_path = pathlib.Path(dest_path).resolve()
    dest_path.mkdir(exist_ok=True, parents=True)

    download_path = manifest.get_o3de_download_folder() / 'gems' / gem_name
    download_path.mkdir(exist_ok=True, parents=True)
    download_zip_path = download_path / 'gem.zip'

    downloadable_gem_data = get_downloadable(gem_name=gem_name)
    if not downloadable_gem_data:
        logger.error(f'Downloadable gem {gem_name} not found.')
        return 1

    origin = downloadable_gem_data['origin']
    url = f'{origin}/gem.zip'
    parsed_uri = urllib.parse.urlparse(url)

    if download_zip_path.is_file():
        logger.warn(f'Project already downloaded to {download_zip_path}.')
    elif parsed_uri.scheme == 'http' or \
            parsed_uri.scheme == 'https' or \
            parsed_uri.scheme == 'ftp' or \
            parsed_uri.scheme == 'ftps':
        with urllib.request.urlopen(url) as s:
            with download_zip_path.open('wb') as f:
                shutil.copyfileobj(s, f)
    else:
        origin_file = pathlib.Path(url).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_zip_path)

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"Gem zip {download_zip_path} is invalid.")
        download_zip_path.unlink()
        return 1

    # if the gem.json has a sha256 check it against a sha256 of the zip
    try:
        sha256A = downloadable_gem_data['sha256']
    except Exception as e:
        logger.warn(f'SECURITY WARNING: The advertised gem you downloaded has no "sha256"!!! Be VERY careful!!!'
                    f' We cannot verify this is the actually the advertised gem!!!')
    else:
        sha256B = hashlib.sha256(download_zip_path.open('rb').read()).hexdigest()
        if sha256A != sha256B:
            logger.error(f'SECURITY VIOLATION: Downloaded gem.zip sha256 {sha256B} does not match'
                         f' the advertised "sha256":{sha256A} in the gem.json. Deleting unzipped files!!!')
            shutil.rmtree(dest_path)
            return 1

    dest_gem_folder = dest_path / gem_name
    if dest_gem_folder.is_dir():
        utils.backup_folder(dest_gem_folder)
    with zipfile.ZipFile(download_zip_path, 'r') as gem_zip:
        try:
            gem_zip.extractall(dest_path)
        except Exception as e:
            logger.error(f'UnZip exception:{str(e)}')
            shutil.rmtree(dest_path)
            return 1

    unzipped_gem_json = dest_gem_folder / 'gem.json'
    if not unzipped_gem_json.is_file():
        logger.error(f'Engine json {unzipped_gem_json} is missing.')
        return 1

    if not validation.valid_o3de_engine_json(unzipped_gem_json):
        logger.error(f'Engine json {unzipped_gem_json} is invalid.')
        return 1

    # remove the sha256 if present in the advertised downloadable gem.json
    # then compare it to the gem.json in the zip, they should now be identical
    try:
        del downloadable_gem_data['sha256']
    except Exception as e:
        pass

    sha256A = hashlib.sha256(json.dumps(downloadable_gem_data, indent=4).encode('utf8')).hexdigest()
    with unzipped_gem_json.open('r') as s:
        try:
            unzipped_gem_json_data = json.load(s)
        except Exception as e:
            logger.error(f'Failed to read gem json {unzipped_gem_json}. Unable to confirm this'
                         f' is the same gem that was advertised.')
            return 1
    sha256B = hashlib.sha256(json.dumps(unzipped_gem_json_data, indent=4).encode('utf8')).hexdigest()
    if sha256A != sha256B:
        logger.error(f'SECURITY VIOLATION: Downloaded gem.json does not match'
                     f' the advertised gem.json. Deleting unzipped files!!!')
        shutil.rmtree(dest_path)
        return 1

    return 0


def download_template(template_name: str,
                      dest_path: str or pathlib.Path) -> int:
    if not dest_path:
        dest_path = manifest.get_registered(default_folder='templates')
        if not dest_path:
            logger.error(f'Destination path not cannot be empty.')
            return 1

    dest_path = pathlib.Path(dest_path).resolve()
    dest_path.mkdir(exist_ok=True, parents=True)

    download_path = manifest.get_o3de_download_folder() / 'templates' / template_name
    download_path.mkdir(exist_ok=True, parents=True)
    download_zip_path = download_path / 'template.zip'

    downloadable_template_data = get_downloadable(template_name=template_name)
    if not downloadable_template_data:
        logger.error(f'Downloadable template {template_name} not found.')
        return 1

    origin = downloadable_template_data['origin']
    url = f'{origin}/project.zip'
    parsed_uri = urllib.parse.urlparse(url)

    result = 0

    if download_zip_path.is_file():
        logger.warn(f'Project already downloaded to {download_zip_path}.')
    elif parsed_uri.scheme == 'http' or \
            parsed_uri.scheme == 'https' or \
            parsed_uri.scheme == 'ftp' or \
            parsed_uri.scheme == 'ftps':
        with urllib.request.urlopen(url) as s:
            with download_zip_path.open('wb') as f:
                shutil.copyfileobj(s, f)
    else:
        origin_file = pathlib.Path(url).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_zip_path)

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"Template zip {download_zip_path} is invalid.")
        download_zip_path.unlink()
        return 1

    # if the template.json has a sha256 check it against a sha256 of the zip
    try:
        sha256A = downloadable_template_data['sha256']
    except Exception as e:
        logger.warn(f'SECURITY WARNING: The advertised template you downloaded has no "sha256"!!! Be VERY careful!!!'
                    f' We cannot verify this is the actually the advertised template!!!')
    else:
        sha256B = hashlib.sha256(download_zip_path.open('rb').read()).hexdigest()
        if sha256A != sha256B:
            logger.error(f'SECURITY VIOLATION: Downloaded template.zip sha256 {sha256B} does not match'
                         f' the advertised "sha256":{sha256A} in the template.json. Deleting unzipped files!!!')
            shutil.rmtree(dest_path)
            return 1

    dest_template_folder = dest_path / template_name
    if dest_template_folder.is_dir():
        utils.backup_folder(dest_template_folder)
    with zipfile.ZipFile(download_zip_path, 'r') as project_zip:
        try:
            project_zip.extractall(dest_path)
        except Exception as e:
            logger.error(f'UnZip exception:{str(e)}')
            shutil.rmtree(dest_path)
            return 1

    unzipped_template_json = dest_template_folder / 'template.json'
    if not unzipped_template_json.is_file():
        logger.error(f'Template json {unzipped_template_json} is missing.')
        return 1

    if not validation.valid_o3de_engine_json(unzipped_template_json):
        logger.error(f'Template json {unzipped_template_json} is invalid.')
        return 1

    # remove the sha256 if present in the advertised downloadable template.json
    # then compare it to the template.json in the zip, they should now be identical
    try:
        del downloadable_template_data['sha256']
    except Exception as e:
        pass

    sha256A = hashlib.sha256(json.dumps(downloadable_template_data, indent=4).encode('utf8')).hexdigest()
    with unzipped_template_json.open('r') as s:
        try:
            unzipped_template_json_data = json.load(s)
        except Exception as e:
            logger.error(f'Failed to read Template json {unzipped_template_json}. Unable to confirm this'
                         f' is the same template that was advertised.')
            return 1
    sha256B = hashlib.sha256(json.dumps(unzipped_template_json_data, indent=4).encode('utf8')).hexdigest()
    if sha256A != sha256B:
        logger.error(f'SECURITY VIOLATION: Downloaded template.json does not match'
                     f' the advertised template.json. Deleting unzipped files!!!')
        shutil.rmtree(dest_path)
        return 1

    return 0


def download_restricted(restricted_name: str,
                        dest_path: str or pathlib.Path) -> int:
    if not dest_path:
        dest_path = manifest.get_registered(default_folder='restricted')
        if not dest_path:
            logger.error(f'Destination path not cannot be empty.')
            return 1

    dest_path = pathlib.Path(dest_path).resolve()
    dest_path.mkdir(exist_ok=True, parents=True)

    download_path = manifest.get_o3de_download_folder() / 'restricted' / restricted_name
    download_path.mkdir(exist_ok=True, parents=True)
    download_zip_path = download_path / 'restricted.zip'

    downloadable_restricted_data = get_downloadable(restricted_name=restricted_name)
    if not downloadable_restricted_data:
        logger.error(f'Downloadable Restricted {restricted_name} not found.')
        return 1

    origin = downloadable_restricted_data['origin']
    url = f'{origin}/restricted.zip'
    parsed_uri = urllib.parse.urlparse(url)

    if download_zip_path.is_file():
        logger.warn(f'Restricted already downloaded to {download_zip_path}.')
    elif parsed_uri.scheme == 'http' or \
            parsed_uri.scheme == 'https' or \
            parsed_uri.scheme == 'ftp' or \
            parsed_uri.scheme == 'ftps':
        with urllib.request.urlopen(url) as s:
            with download_zip_path.open('wb') as f:
                shutil.copyfileobj(s, f)
    else:
        origin_file = pathlib.Path(url).resolve()
        if not origin_file.is_file():
            return 1
        shutil.copy(origin_file, download_zip_path)

    if not zipfile.is_zipfile(download_zip_path):
        logger.error(f"Restricted zip {download_zip_path} is invalid.")
        download_zip_path.unlink()
        return 1

    # if the restricted.json has a sha256 check it against a sha256 of the zip
    try:
        sha256A = downloadable_restricted_data['sha256']
    except Exception as e:
        logger.warn(f'SECURITY WARNING: The advertised restricted you downloaded has no "sha256"!!! Be VERY careful!!!'
                    f' We cannot verify this is the actually the advertised restricted!!!')
    else:
        sha256B = hashlib.sha256(download_zip_path.open('rb').read()).hexdigest()
        if sha256A != sha256B:
            logger.error(f'SECURITY VIOLATION: Downloaded restricted.zip sha256 {sha256B} does not match'
                         f' the advertised "sha256":{sha256A} in the restricted.json. Deleting unzipped files!!!')
            shutil.rmtree(dest_path)
            return 1

    dest_restricted_folder = dest_path / restricted_name
    if dest_restricted_folder.is_dir():
        utils.backup_folder(dest_restricted_folder)
    with zipfile.ZipFile(download_zip_path, 'r') as project_zip:
        try:
            project_zip.extractall(dest_path)
        except Exception as e:
            logger.error(f'UnZip exception:{str(e)}')
            shutil.rmtree(dest_path)
            return 1

    unzipped_restricted_json = dest_restricted_folder / 'restricted.json'
    if not unzipped_restricted_json.is_file():
        logger.error(f'Restricted json {unzipped_restricted_json} is missing.')
        return 1

    if not validation.valid_o3de_engine_json(unzipped_restricted_json):
        logger.error(f'Restricted json {unzipped_restricted_json} is invalid.')
        return 1

    # remove the sha256 if present in the advertised downloadable restricted.json
    # then compare it to the restricted.json in the zip, they should now be identical
    try:
        del downloadable_restricted_data['sha256']
    except Exception as e:
        pass

    sha256A = hashlib.sha256(json.dumps(downloadable_restricted_data, indent=4).encode('utf8')).hexdigest()
    with unzipped_restricted_json.open('r') as s:
        try:
            unzipped_restricted_json_data = json.load(s)
        except Exception as e:
            logger.error(
                f'Failed to read Restricted json {unzipped_restricted_json}. Unable to confirm this'
                f' is the same restricted that was advertised.')
            return 1
    sha256B = hashlib.sha256(
        json.dumps(unzipped_restricted_json_data, indent=4).encode('utf8')).hexdigest()
    if sha256A != sha256B:
        logger.error(f'SECURITY VIOLATION: Downloaded restricted.json does not match'
                     f' the advertised restricted.json. Deleting unzipped files!!!')
        shutil.rmtree(dest_path)
        return 1

    return 0


def _run_download(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    if args.engine_name:
        return download_engine(args.engine_name,
                               args.dest_path)
    elif args.project_name:
        return download_project(args.project_name,
                                args.dest_path)
    elif args.gem_nanme:
        return download_gem(args.gem_name,
                            args.dest_path)
    elif args.template_name:
        return download_template(args.template_name,
                                 args.dest_path)


def add_args(parser, subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python register.py register --gem-path "C:/TestGem"
    OR
    o3de.py can downloadable commands by importing engine_template,
    call add_args and execute: python o3de.py register --gem-path "C:/TestGem"
    :param parser: the caller instantiates a parser and passes it in here
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    download_subparser = subparsers.add_parser('download')
    group = download_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-e', '--engine-name', type=str, required=False,
                       help='Downloadable engine name.')
    group.add_argument('-p', '--project-name', type=str, required=False,
                       help='Downloadable project name.')
    group.add_argument('-g', '--gem-name', type=str, required=False,
                       help='Downloadable gem name.')
    group.add_argument('-t', '--template-name', type=str, required=False,
                       help='Downloadable template name.')
    download_subparser.add_argument('-dp', '--dest-path', type=str, required=False,
                                    default=None,
                                    help='Optional destination folder to download into.'
                                         ' i.e. download --project-name "StarterGame" --dest-path "C:/projects"'
                                         ' will result in C:/projects/StarterGame'
                                         ' If blank will download to default object type folder')

    download_subparser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                    help='By default the home folder is the user folder, override it to this folder.')

    download_subparser.set_defaults(func=_run_download)

