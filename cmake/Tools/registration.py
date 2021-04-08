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
This file contains all the code that has to do with registering engines, projects, gems and templates
"""
import argparse
import logging
import os
import sys
import json
import pathlib
import hashlib
import shutil
import urllib

logger = logging.getLogger()
logging.basicConfig()


def backup(file_name):
    index = 0
    renamed = False
    while not renamed:
        backup_file_name = pathlib.Path(str(file_name) + '.bak' + str(index))
        if not backup_file_name.is_file():
            file_name = pathlib.Path(file_name)
            file_name.rename(backup_file_name)
            renamed = True

def register_shipped_engine_o3de_objects():
    # register this engines shipped projects, gems and templates
    engine_path = get_this_engine_path()
    for root, dirs, files in os.walk(engine_path / 'AtomSampleViewer', topdown=False):
        for name in files:
            if name == 'project.json':
                register(project_path=root)
            elif name == 'gem.json':
                register(gem_path=root)
            elif name == 'template.json':
                register(template_path=root)

    for root, dirs, files in os.walk(engine_path / 'AtomTest', topdown=False):
        for name in files:
            if name == 'project.json':
                register(project_path=root)
            elif name == 'gem.json':
                register(gem_path=root)
            elif name == 'template.json':
                register(template_path=root)

    for root, dirs, files in os.walk(engine_path / 'Gems', topdown=False):
        for name in files:
            if name == 'gem.json':
                register(gem_path=root)

    engine_templates = os.listdir(engine_path / 'Templates')
    for engine_template in engine_templates:
        register(template_path=engine_path / 'Templates' / engine_template)

    engine_projects = os.listdir(engine_path)
    for engine_project in engine_projects:
        engine_project_json = engine_path / engine_project / 'project.json'
        if engine_project_json.is_file():
            register(project_path=engine_path / engine_project)


def get_this_engine_path():
    return pathlib.Path(os.path.realpath(__file__)).parent.parent.parent


def get_home_folder():
    return pathlib.Path(os.path.expanduser("~"))


def get_o3de_folder():
    o3de_folder = get_home_folder() / '.o3de'
    o3de_folder.mkdir(parents=True, exist_ok=True)
    return o3de_folder


def get_o3de_cache():
    cache_folder = get_o3de_folder() / 'cache'
    cache_folder.mkdir(parents=True, exist_ok=True)
    return cache_folder


def get_o3de_registry():
    registry_path = get_o3de_folder() / 'o3de_manifest.json'
    if not registry_path.is_file():

        username = os.path.split(get_home_folder())[-1]

        json_data = {}
        json_data.update({'repo_name': f'{username}'})
        json_data.update({'origin': get_o3de_folder().as_posix()})
        json_data.update({'engines': []})
        json_data.update({'projects': []})
        json_data.update({'gems': []})
        json_data.update({'templates': []})
        json_data.update({'repos': []})
        default_projects_folder = get_home_folder() / 'my_o3de/projects'
        default_projects_folder.mkdir(parents=True, exist_ok=True)
        json_data.update({'default_projects_folder': default_projects_folder.as_posix()})
        default_gems_folder = get_home_folder() / 'my_o3de/gems'
        default_gems_folder.mkdir(parents=True, exist_ok=True)
        json_data.update({'default_gems_folder': default_gems_folder.as_posix()})
        default_templates_folder = get_home_folder() / 'my_o3de/templates'
        default_templates_folder.mkdir(parents=True, exist_ok=True)
        json_data.update({'default_templates_folder': default_templates_folder.as_posix()})
        with registry_path.open('w') as s:
            s.write(json.dumps(json_data, indent=4))

    return registry_path


def load_o3de_registry():
    with get_o3de_registry().open('r') as f:
        return json.load(f)


def save_o3de_registry(json_data):
    with get_o3de_registry().open('w') as s:
        s.write(json.dumps(json_data, indent=4))


def register_engine_path(json_data,
                         engine_path: str,
                         remove: bool = False) -> int:
    if not engine_path:
        logger.error(f'Engine path cannot be empty.')
        return 1

    while engine_path in json_data['engines']:
        json_data['engines'].remove(engine_path)
    engine_path = pathlib.Path(engine_path)
    while engine_path.as_posix() in json_data['engines']:
        json_data['engines'].remove(engine_path.as_posix())

    if remove:
        logger.warn(f'Removing Engine path {engine_path}.')
        return 0

    if not engine_path.is_dir():
        logger.error(f'Engine path {engine_path} does not exist.')
        return 1

    engine_json = engine_path / 'engine.json'
    if not valid_o3de_engine_json(engine_json):
        logger.error(f'Engine json {engine_json} is not valid.')
        return 1

    json_data['engines'].insert(0, engine_path.as_posix())

    return 0


def register_gem_path(json_data,
                      gem_path: str,
                      remove: bool = False) -> int:
    if not gem_path:
        logger.error(f'Gem path cannot be empty.')
        return 1

    while gem_path in json_data['gems']:
        json_data['gems'].remove(gem_path)
    gem_path = pathlib.Path(gem_path)
    while gem_path.as_posix() in json_data['gems']:
        json_data['gems'].remove(gem_path.as_posix())

    if remove:
        logger.warn(f'Removing Gem path {gem_path}.')
        return 0

    if not gem_path.is_dir():
        logger.error(f'Gem path {gem_path} does not exist.')
        return 1

    gem_json = gem_path / 'gem.json'
    if not valid_o3de_gem_json(gem_json):
        logger.error(f'Gem json {gem_json} is not valid.')
        return 1

    json_data['gems'].insert(0, gem_path.as_posix())

    return 0


def register_project_path(json_data,
                          project_path: str,
                          remove: bool = False) -> int:
    if not project_path:
        logger.error(f'Project path cannot be empty.')
        return 1

    while project_path in json_data['projects']:
        json_data['projects'].remove(project_path)
    project_path = pathlib.Path(project_path)
    while project_path.as_posix() in json_data['projects']:
        json_data['projects'].remove(project_path.as_posix())

    if remove:
        logger.warn(f'Removing Project path {project_path}.')
        return 0

    if not project_path.is_dir():
        logger.error(f'Project path {project_path} does not exist.')
        return 1

    project_json = project_path / 'project.json'
    if not valid_o3de_project_json(project_json):
        logger.error(f'Project json {project_json} is not valid.')
        return 1

    json_data['projects'].insert(0, project_path.as_posix())

    # registering a project has the additional step of setting the project.json 'engine' field
    this_engine_json = get_this_engine_path() / 'engine.json'
    with this_engine_json.open('r') as f:
        this_engine_json = json.load(f)
    with project_json.open('r') as f:
        project_json_data = json.load(f)

    update_project_json = False
    try:
        update_project_json = project_json_data['engine'] != this_engine_json['engine_name']
    except Exception as e:
        update_project_json = True

    if update_project_json:
        project_json_data['engine'] = this_engine_json['engine_name']
        backup(project_json)
        with project_json.open('w') as s:
            s.write(json.dumps(project_json_data, indent=4))

    return 0


def register_template_path(json_data,
                           template_path: str,
                           remove: bool = False) -> int:
    if not template_path:
        logger.error(f'Template path cannot be empty.')
        return 1

    while template_path in json_data['templates']:
        json_data['templates'].remove(template_path)
    template_path = pathlib.Path(template_path)
    while template_path.as_posix() in json_data['templates']:
        json_data['templates'].remove(template_path.as_posix())

    if remove:
        logger.warn(f'Removing Template path {template_path}.')
        return 0

    if not template_path.is_dir():
        logger.error(f'Template path {template_path} does not exist.')
        return 1

    template_json = template_path / 'template.json'
    if not valid_o3de_template_json(template_json):
        logger.error(f'Template json {template_json} is not valid.')
        return 1

    json_data['templates'].insert(0, template_path.as_posix())

    return 0


def register_repo(json_data,
                  repo_uri: str,
                  remove: bool = False) -> int:
    if not repo_uri:
        logger.error(f'Repo URI cannot be empty.')
        return 1

    while repo_uri in json_data['repos']:
        json_data['repos'].remove(repo_uri)

    if remove:
        logger.warn(f'Removing repo uri {repo_uri}.')

    if remove:
        result = refresh_repos()
    else:
        repo_hash = hashlib.md5(repo_uri.encode())
        cache_folder = get_o3de_cache()
        cache_file = cache_folder / str(repo_hash.hexdigest() + '.json')
        parsed_uri = urllib.parse.urlparse(repo_uri)

        if parsed_uri.scheme == 'http' or parsed_uri.scheme == 'https' or parsed_uri.scheme == 'ftp' or parsed_uri.scheme == 'ftps':
            result = 0  # a function that processes the uri and returns result
            if not result:
                json_data['repos'].insert(0, repo_uri)
        else:
            repo_uri = pathlib.Path(repo_uri)
            json_data['repos'].insert(0, repo_uri.as_posix())
            result = 0

    return result


def valid_o3de_manifest_json(file_name: str) -> bool:
    try:
        file_name = pathlib.Path(file_name)
        if not file_name.is_file():
            return False
        with file_name.open('r') as f:
            json_data = json.load(f)
            test = json_data['repo_name']
            test = json_data['origin']
            test = json_data['engines']
            test = json_data['projects']
            test = json_data['gems']
            test = json_data['templates']
            test = json_data['repos']
    except Exception as e:
        return False

    return True


def valid_o3de_engine_json(file_name: str) -> bool:
    try:
        file_name = pathlib.Path(file_name)
        if not file_name.is_file():
            return False
        with file_name.open('r') as f:
            json_data = json.load(f)
            test = json_data['engine_name']
    except Exception as e:
        return False

    return True


def valid_o3de_project_json(file_name: str) -> bool:
    try:
        file_name = pathlib.Path(file_name)
        if not file_name.is_file():
            return False
        with file_name.open('r') as f:
            json_data = json.load(f)
            test = json_data['project_name']
    except Exception as e:
        return False

    return True


def valid_o3de_gem_json(file_name: str) -> bool:
    try:
        file_name = pathlib.Path(file_name)
        if not file_name.is_file():
            return False
        with file_name.open('r') as f:
            json_data = json.load(f)
            test = json_data['gem_name']
    except Exception as e:
        return False

    return True


def valid_o3de_template_json(file_name: str) -> bool:
    try:
        file_name = pathlib.Path(file_name)
        if not file_name.is_file():
            return False
        with file_name.open('r') as f:
            json_data = json.load(f)
            test = json_data['template_name']
    except Exception as e:
        return False

    return True


def register_default_projects_folder(json_data,
                                     default_projects_folder: str,
                                     remove: bool = False) -> int:
    if remove:
        json_data['default_projects_folder'] = ''
    else:
        # make sure the path exists
        default_projects_folder = pathlib.Path(default_projects_folder)
        if not default_projects_folder.is_dir():
            logger.error(f'Default projects folder {default_projects_folder} does not exist.')
            return 1

        default_projects_folder = default_projects_folder.as_posix()
        json_data['default_projects_folder'] = default_projects_folder

    return 0


def register_default_gems_folder(json_data,
                                 default_gems_folder: str,
                                 remove: bool = False) -> int:
    if remove:
        json_data['default_gems_folder'] = ''
    else:
        # make sure the path exists
        default_gems_folder = pathlib.Path(default_gems_folder)
        if not default_gems_folder.is_dir():
            logger.error(f'Default gems folder {default_gems_folder} does not exist.')
            return 1

        default_gems_folder = default_gems_folder.as_posix()
        json_data['default_gems_folder'] = default_gems_folder

    return 0


def register_default_templates_folder(json_data,
                                      default_templates_folder: str,
                                      remove: bool = False) -> int:
    if remove:
        json_data['default_templates_folder'] = ''
    else:
        # make sure the path exists
        default_templates_folder = pathlib.Path(default_templates_folder)
        if not default_templates_folder.is_dir():
            logger.error(f'Default templates folder {default_templates_folder} does not exist.')
            return 1

        default_templates_folder = default_templates_folder.as_posix()
        json_data['default_templates_folder'] = default_templates_folder

    return 0


def register(engine_path: str = None,
             project_path: str = None,
             gem_path: str = None,
             template_path: str = None,
             default_projects_folder: str = None,
             default_gems_folder: str = None,
             default_templates_folder: str = None,
             repo_uri: str = None,
             remove: bool = False) -> int:
    """
    Adds/Updates entries to the .o3de/o3de_manifest.json

    :param engine_path: engine folder
    :param project_path: project folder
    :param gem_path: gem folder
    :param template_path: template folder
    :param default_projects_folder: default projects folder
    :param default_gems_folder: default gems folder
    :param default_templates_folder: default templates folder
    :param repo_uri: repo uri
    :param remove: add/remove the entries
    :return: 0 for success or non 0 failure code
    """

    json_data = load_o3de_registry()

    if isinstance(engine_path, str) or isinstance(engine_path, pathlib.PurePath):
        if not engine_path:
            logger.error(f'Engine path cannot be empty.')
            return 1
        result = register_engine_path(json_data, engine_path, remove)

    elif isinstance(project_path, str) or isinstance(project_path, pathlib.PurePath):
        if not project_path:
            logger.error(f'Project path cannot be empty.')
            return 1
        result = register_project_path(json_data, project_path, remove)

    elif isinstance(gem_path, str) or isinstance(gem_path, pathlib.PurePath):
        if not gem_path:
            logger.error(f'Gem path cannot be empty.')
            return 1
        result = register_gem_path(json_data, gem_path, remove)

    elif isinstance(template_path, str) or isinstance(template_path, pathlib.PurePath):
        if not template_path:
            logger.error(f'Template path cannot be empty.')
            return 1
        result = register_template_path(json_data, template_path, remove)

    elif isinstance(repo_uri, str) or isinstance(repo_uri, pathlib.PurePath):
        if not repo_uri:
            logger.error(f'Repo URI cannot be empty.')
            return 1
        result = register_repo(json_data, repo_uri, remove)

    elif isinstance(default_projects_folder, str) or isinstance(default_projects_folder, pathlib.PurePath):
        result = register_default_projects_folder(json_data, default_projects_folder, remove)

    elif isinstance(default_gems_folder, str) or isinstance(default_gems_folder, pathlib.PurePath):
        result = register_default_gems_folder(json_data, default_gems_folder, remove)

    elif isinstance(default_templates_folder, str) or isinstance(default_templates_folder, pathlib.PurePath):
        result = register_default_templates_folder(json_data, default_templates_folder, remove)

    if not result:
        save_o3de_registry(json_data)

    return 0


def remove_invalid_o3de_objects():
    json_data = load_o3de_registry()

    for engine in json_data['engines']:
        if not valid_o3de_engine_json(pathlib.Path(engine) / 'engine.json'):
            logger.warn(f"Engine path {engine} is invalid.")
            register(engine_path=engine, remove=True)

    for project in json_data['projects']:
        if not valid_o3de_project_json(pathlib.Path(project) / 'project.json'):
            logger.warn(f"Project path {project} is invalid.")
            register(project_path=project, remove=True)

    for gem in json_data['gems']:
        if not valid_o3de_gem_json(pathlib.Path(gem) / 'gem.json'):
            logger.warn(f"Gem path {gem} is invalid.")
            register(gem_path=gem, remove=True)

    for template in json_data['templates']:
        if not valid_o3de_template_json(pathlib.Path(template) / 'template.json'):
            logger.warn(f"Template path {template} is invalid.")
            register(template_path=template, remove=True)

    default_projects_folder = pathlib.Path(json_data['default_projects_folder'])
    if not default_projects_folder.is_dir():
        new_default_projects_folder = get_home_folder() / 'my_o3de/projects'
        new_default_projects_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(f"Default projects folder {default_projects_folder} is invalid. Set default {new_default_projects_folder}")
        register(default_projects_folder=new_default_projects_folder.as_posix())

    default_gems_folder = pathlib.Path(json_data['default_gems_folder'])
    if not default_gems_folder.is_dir():
        new_default_gems_folder = get_home_folder() / 'my_o3de/gems'
        new_default_gems_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(f"Default gems folder {default_gems_folder} is invalid. Set default {new_default_gems_folder}")
        register(default_gems_folder=new_default_gems_folder.as_posix())

    default_templates_folder = pathlib.Path(json_data['default_templates_folder'])
    if not default_templates_folder.is_dir():
        new_default_templates_folder = get_home_folder() / 'my_o3de/templates'
        new_default_templates_folder.mkdir(parents=True, exist_ok=True)
        logger.warn(f"Default templates folder {default_templates_folder} is invalid. Set default {new_default_templates_folder}")
        register(default_templates_folder=new_default_templates_folder.as_posix())


def refresh_repos() -> int:
    json_data = load_o3de_registry()
    cache_folder = get_o3de_cache()
    shutil.rmtree(cache_folder)
    cache_folder.mkdir(parents=True, exist_ok=True)
    if len(json_data['repos']) == 0:
        return 0

    result = 0
    last_failure = 0
    for repo_uri in json_data['repos']:
        repo_hash = hashlib.md5(repo_uri.encode())
        cache_file = cache_folder / str(repo_hash.hexdigest() + '.json')
        parsed_uri = urllib.parse.urlparse(repo_uri)

        last_failure = 0  # download and validate the repo_uri
        if not last_failure:
            result = 1

    return result


def get_registered(engine_name: str = None,
                   project_name: str = None,
                   gem_name: str = None,
                   template_name: str = None,
                   default_folder: str = None,
                   repo_name: str = None):
    json_data = load_o3de_registry()

    if type(engine_name) == str:
        for engine in json_data['engines']:
            engine = pathlib.Path(engine) / 'engine.json'
            with engine.open('r') as f:
                engine_json_data = json.load(f)
                this_engines_name = engine_json_data['engine_name']
                if this_engines_name == engine_name:
                    engine = engine.parent.as_posix()
                    return engine

    elif type(project_name) == str:
        for project in json_data['projects']:
            project = pathlib.Path(project) / 'project.json'
            with project.open('r') as f:
                project_json_data = json.load(f)
                this_projects_name = project_json_data['project_name']
                if this_projects_name == project_name:
                    project = project.parent.as_posix()
                    return project

    elif type(gem_name) == str:
        for gem in json_data['gems']:
            gem = pathlib.Path(gem) / 'gem.json'
            with gem.open('r') as f:
                gem_json_data = json.load(f)
                this_gems_name = gem_json_data['gem_name']
                if this_gems_name == gem_name:
                    gem = gem.parent.as_posix()
                    return gem

    elif type(template_name) == str:
        for template in json_data['templates']:
            template = pathlib.Path(template) / 'template.json'
            with template.open('r') as f:
                template_json_data = json.load(f)
                this_templates_name = template_json_data['template_name']
                if this_templates_name == template_name:
                    template = template.parent.as_posix()
                    return template

    elif type(default_folder) == str:
        if default_folder == 'project':
            return json_data['default_projects_folder']
        elif default_folder == 'gem':
            return json_data['default_gems_folder']
        elif default_folder == 'template':
            return json_data['default_templates_folder']

    elif type(repo_name) == str:
        cache_folder = get_o3de_cache()
        cache_folder.mkdir(parents=True, exist_ok=True)
        for repo_uri in json_data['repos']:
            repo_hash = hashlib.md5(repo_uri.encode())
            cache_file = cache_folder / str(repo_hash.hexdigest() + '.json')
            if cache_file.is_file():
                repo = pathlib.Path(cache_file)
                with repo.open('r') as f:
                    repo_json_data = json.load(f)
                    this_repos_name = repo_json_data['repo_name']
                    if this_repos_name == repo_name:
                        repo = repo.parent.as_posix()
                        return repo
    return None


def print_engines(json_data,
                  verbose: int):
    if verbose > 0:
        print('\n')
        print("Engines================================================")
        for engine in json_data['engines']:
            engine = pathlib.Path(engine) / 'engine.json'
            with engine.open('r') as f:
                engine_json_data = json.load(f)
                print(engine)
                print(json.dumps(engine_json_data, indent=4))
            print('\n')


def print_projects(json_data,
                   verbose: int):
    if verbose > 0:
        print('\n')
        print("Projects================================================")
        for project in json_data['projects']:
            project = pathlib.Path(project) / 'project.json'
            with project.open('r') as f:
                project_json_data = json.load(f)
                print(project)
                print(json.dumps(project_json_data, indent=4))
            print('\n')


def print_gems(json_data,
               verbose: int):
    if verbose > 0:
        print('\n')
        print("Gems================================================")
        for gem in json_data['gems']:
            gem = pathlib.Path(gem) / 'gem.json'
            with gem.open('r') as f:
                gem_json_data = json.load(f)
                print(gem)
                print(json.dumps(gem_json_data, indent=4))
            print('\n')


def print_templates(json_data,
                    verbose: int):
    if verbose > 0:
        print("Templates================================================")
        for template in json_data['templates']:
            template = pathlib.Path(template) / 'template.json'
            with template.open('r') as f:
                template_json_data = json.load(f)
                print(template)
                print(json.dumps(template_json_data, indent=4))
            print('\n')


def print_repos(json_data: str,
                verbose: int):
    if verbose > 0:
        print("Repos================================================")
        cache_folder = get_o3de_cache()
        cache_folder.mkdir(parents=True, exist_ok=True)
        for repo in json_data['repos']:
            repo_hash = hashlib.md5(repo.encode())
            cache_file = cache_folder / str(repo_hash.hexdigest() + '.json')
            if valid_o3de_manifest_json(cache_file):
                with cache_file.open('r') as s:
                    repo_json_data = json.load(s)
                    print(repo)
                    print(json.dumps(repo_json_data, indent=4))
            print('\n')


def register_show_engines(verbose: int):
    json_data = load_o3de_registry()

    del json_data['repo_name']
    del json_data['origin']
    del json_data['projects']
    del json_data['gems']
    del json_data['templates']
    del json_data['repos']
    del json_data['default_projects_folder']
    del json_data['default_gems_folder']
    del json_data['default_templates_folder']

    print(json.dumps(json_data, indent=4))
    print_engines(json_data,
                  verbose)


def register_show_projects(verbose: int):
    json_data = load_o3de_registry()

    del json_data['repo_name']
    del json_data['origin']
    del json_data['engines']
    del json_data['gems']
    del json_data['templates']
    del json_data['repos']
    del json_data['default_projects_folder']
    del json_data['default_gems_folder']
    del json_data['default_templates_folder']

    print(json.dumps(json_data, indent=4))
    print_projects(json_data,
                   verbose)


def register_show_gems(verbose: int):
    json_data = load_o3de_registry()

    del json_data['repo_name']
    del json_data['origin']
    del json_data['engines']
    del json_data['projects']
    del json_data['templates']
    del json_data['repos']
    del json_data['default_projects_folder']
    del json_data['default_gems_folder']
    del json_data['default_templates_folder']

    print(json.dumps(json_data, indent=4))
    print_gems(json_data,
               verbose)


def register_show_templates(verbose: int):
    json_data = load_o3de_registry()

    del json_data['repo_name']
    del json_data['origin']
    del json_data['engines']
    del json_data['projects']
    del json_data['gems']
    del json_data['repos']
    del json_data['default_projects_folder']
    del json_data['default_gems_folder']
    del json_data['default_templates_folder']

    print(json.dumps(json_data, indent=4))
    print_templates(json_data,
                    verbose)


def register_show_repos(verbose: int):
    json_data = load_o3de_registry()

    del json_data['repo_name']
    del json_data['origin']
    del json_data['engines']
    del json_data['projects']
    del json_data['gems']
    del json_data['templates']
    del json_data['default_projects_folder']
    del json_data['default_gems_folder']
    del json_data['default_templates_folder']

    print(json.dumps(json_data, indent=4))
    print_repos(json_data,
                verbose)


def register_show(verbose: int):
    json_data = load_o3de_registry()
    if verbose > 0:
        print(f"{get_o3de_registry()}:")

    print(json.dumps(json_data, indent=4))

    print_engines(json_data,
                  verbose)
    print_projects(json_data,
                   verbose)
    print_gems(json_data,
               verbose)
    print_templates(json_data,
                    verbose)
    print_repos(json_data,
                verbose)

    if verbose > 0:
        print("Default Folders================================================")
        print(f"Default projects folder: {json_data['default_projects_folder']}")
        print(os.listdir(json_data['default_projects_folder']))
        print('\n')
        print(f"Default gems folder: {json_data['default_gems_folder']}")
        print(os.listdir(json_data['default_gems_folder']))
        print('\n')
        print(f"Default templates folder: {json_data['default_templates_folder']}")
        print(os.listdir(json_data['default_templates_folder']))


def aggregate_repo(json_data, repo_uri: str):
    cache_folder = get_o3de_cache()
    cache_folder.mkdir(parents=True, exist_ok=True)
    repo_hash = hashlib.md5(repo_uri.encode())
    cache_file = cache_folder / str(repo_hash.hexdigest() + '.json')
    if valid_o3de_manifest_json(cache_file):
        with cache_file.open('r') as s:
            repo_json_data = json.load(s)
            for engine in repo_json_data['engines']:
                if engine not in json_data['engines']:
                    json_data['engines'].append(engine)

            for project in repo_json_data['projects']:
                if project not in json_data['projects']:
                    json_data['projects'].append(project)

            for gem in repo_json_data['gems']:
                if gem not in json_data['gems']:
                    json_data['gems'].append(gem)

            for template in repo_json_data['templates']:
                if template not in json_data['templates']:
                    json_data['templates'].append(template)

            for repo in repo_json_data['repos']:
                if repo not in json_data['repos']:
                    json_data['repos'].append(repo)

            for repo_uri in repo_json_data['repos']:
                aggregate_repo(json_data, repo_uri)


def register_show_aggregate_engines(verbose: int):
    json_data = load_o3de_registry()
    repos = json_data['repos'].copy()
    for repo_uri in repos:
        aggregate_repo(json_data, repo_uri)

    del json_data['projects']
    del json_data['gems']
    del json_data['templates']
    del json_data['repos']
    del json_data['default_projects_folder']
    del json_data['default_gems_folder']
    del json_data['default_templates_folder']

    print(json.dumps(json_data, indent=4))
    print_engines(json_data,
                  verbose)


def register_show_aggregate_projects(verbose: int):
    json_data = load_o3de_registry()
    repos = json_data['repos'].copy()
    for repo_uri in repos:
        aggregate_repo(json_data, repo_uri)

    del json_data['engines']
    del json_data['gems']
    del json_data['templates']
    del json_data['repos']
    del json_data['default_projects_folder']
    del json_data['default_gems_folder']
    del json_data['default_templates_folder']

    print(json.dumps(json_data, indent=4))
    print_projects(json_data,
                   verbose)


def register_show_aggregate_gems(verbose: int):
    json_data = load_o3de_registry()
    repos = json_data['repos'].copy()
    for repo_uri in repos:
        aggregate_repo(json_data, repo_uri)

    del json_data['engines']
    del json_data['projects']
    del json_data['templates']
    del json_data['repos']
    del json_data['default_projects_folder']
    del json_data['default_gems_folder']
    del json_data['default_templates_folder']

    print(json.dumps(json_data, indent=4))
    print_gems(json_data,
               verbose)


def register_show_aggregate_templates(verbose: int):
    json_data = load_o3de_registry()
    repos = json_data['repos'].copy()
    for repo_uri in repos:
        aggregate_repo(json_data, repo_uri)

    del json_data['engines']
    del json_data['projects']
    del json_data['gems']
    del json_data['repos']
    del json_data['default_projects_folder']
    del json_data['default_gems_folder']
    del json_data['default_templates_folder']

    print(json.dumps(json_data, indent=4))
    print_templates(json_data,
                    verbose)


def register_show_aggregate_repos(verbose: int):
    json_data = load_o3de_registry()
    repos = json_data['repos'].copy()
    for repo_uri in repos:
        aggregate_repo(json_data, repo_uri)

    del json_data['engines']
    del json_data['projects']
    del json_data['gems']
    del json_data['templates']
    del json_data['default_projects_folder']
    del json_data['default_gems_folder']
    del json_data['default_templates_folder']

    print(json.dumps(json_data, indent=4))
    print_repos(json_data,
                verbose)


def register_show_aggregate(verbose: int):
    json_data = load_o3de_registry()
    repos = json_data['repos'].copy()
    for repo_uri in repos:
        aggregate_repo(json_data, repo_uri)

    print(json.dumps(json_data, indent=4))

    print_engines(json_data,
                  verbose)
    print_projects(json_data,
                   verbose)
    print_gems(json_data,
               verbose)
    print_templates(json_data,
                    verbose)
    print_repos(json_data,
                verbose)

    if verbose > 0:
        print("Default Folders================================================")
        print(f"Default projects folder: {json_data['default_projects_folder']}")
        print(os.listdir(json_data['default_projects_folder']))
        print('\n')
        print(f"Default gems folder: {json_data['default_gems_folder']}")
        print(os.listdir(json_data['default_gems_folder']))
        print('\n')
        print(f"Default templates folder: {json_data['default_templates_folder']}")
        print(os.listdir(json_data['default_templates_folder']))


def _run_register(args: argparse) -> int:
    if args.update:
        remove_invalid_o3de_objects()
        return refresh_repos()
    else:
        if args.this_engine:
            register(engine_path=get_this_engine_path())
            register_shipped_engine_o3de_objects()
        else:
            return register(args.engine_path,
                        args.project_path,
                        args.gem_path,
                        args.template_path,
                        args.default_projects_folder,
                        args.default_gems_folder,
                        args.default_templates_folder,
                        args.repo_uri,
                        args.remove)


def _run_get_registered(args: argparse) -> int:
    return get_registered(args.engine_name,
                          args.project_name,
                          args.gem_name,
                          args.template_name,
                          args.default_folder,
                          args.repo_name)


def _run_register_show(args: argparse) -> int:

    if args.aggregate:
        register_show_aggregate(args.verbose)
        return 0
    if args.aggregate_engines:
        register_show_aggregate_engines(args.verbose)
        return 0
    elif args.aggregate_projects:
        register_show_aggregate_projects(args.verbose)
        return 0
    elif args.aggregate_gems:
        register_show_aggregate_gems(args.verbose)
        return 0
    elif args.aggregate_templates:
        register_show_aggregate_templates(args.verbose)
        return 0
    elif args.aggregate_repos:
        register_show_aggregate_repos(args.verbose)
        return 0
    elif args.engines:
        register_show_engines(args.verbose)
        return 0
    elif args.projects:
        register_show_projects(args.verbose)
        return 0
    elif args.gems:
        register_show_gems(args.verbose)
        return 0
    elif args.templates:
        register_show_templates(args.verbose)
        return 0
    elif args.repos:
        register_show_repos(args.verbose)
        return 0
    else:
        register_show(args.verbose)
        return 0


def add_args(parser, subparsers) -> None:
    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked locally or aggregated by a central python file.
    Ex. Directly run from this file alone with: python register.py register --gem-path "C:/TestGem"
    OR
    o3de.py can aggregate commands by importing engine_template,
    call add_args and execute: python o3de.py register --gem-path "C:/TestGem"
    :param parser: the caller instantiates a parser and passes it in here
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    # register
    register_subparser = subparsers.add_parser('register')
    group = register_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('--this-engine', action = 'store_true', required=False,
                       default = False,
                       help='Registers the engine this script is running from.')
    group.add_argument('-e', '--engine-path', type=str, required=False,
                       help='Engine path to register/remove.')
    group.add_argument('-p', '--project-path', type=str, required=False,
                       help='Project path to register/remove.')
    group.add_argument('-g', '--gem-path', type=str, required=False,
                       help='Gem path to register/remove.')
    group.add_argument('-t', '--template-path', type=str, required=False,
                       help='Template path to register/remove.')
    group.add_argument('-dp', '--default-projects-folder', type=str, required=False,
                       help='The default projects folder to register/remove.')
    group.add_argument('-dg', '--default-gems-folder', type=str, required=False,
                       help='The default gems folder to register/remove.')
    group.add_argument('-dt', '--default-templates-folder', type=str, required=False,
                       help='The default templates folder to register/remove.')
    group.add_argument('-ru', '--repo-uri', type=str, required=False,
                       help='A repo uri to register/remove.')
    group.add_argument('-u', '--update', action='store_true', required=False,
                       default=False,
                       help='Refresh the repo cache.')
    register_subparser.add_argument('-r', '--remove', action='store_true', required=False,
                                    default=False,
                                    help='Remove entry.')

    register_subparser.set_defaults(func=_run_register)

    # show
    register_show_subparser = subparsers.add_parser('register-show')
    group = register_show_subparser.add_mutually_exclusive_group(required=False)

    group.add_argument('-e', '--engines', action='store_true', required=False,
                       default=False,
                       help='Just the local engines. Ignores repos')
    group.add_argument('-p', '--projects', action='store_true', required=False,
                       default=False,
                       help='Just the local projects. Ignores repos.')
    group.add_argument('-g', '--gems', action='store_true', required=False,
                       default=False,
                       help='Just the local gems. Ignores repos')
    group.add_argument('-t', '--templates', action='store_true', required=False,
                       default=False,
                       help='Just the local templates. Ignores repos.')
    group.add_argument('-r', '--repos', action='store_true', required=False,
                       default=False,
                       help='Just the local repos. Ignores repos.')
    group.add_argument('-a', '--aggregate', action='store_true', required=False,
                       default=False,
                       help='Combine all repos into a single list of resources.')
    group.add_argument('-ae', '--aggregate-engines', action='store_true', required=False,
                       default=False,
                       help='Combine all repos engines into a single list of resources.')
    group.add_argument('-ap', '--aggregate-projects', action='store_true', required=False,
                       default=False,
                       help='Combine all repos projects into a single list of resources.')
    group.add_argument('-ag', '--aggregate-gems', action='store_true', required=False,
                       default=False,
                       help='Combine all repos gems into a single list of resources.')
    group.add_argument('-at', '--aggregate-templates', action='store_true', required=False,
                       default=False,
                       help='Combine all repos templates into a single list of resources.')
    group.add_argument('-ar', '--aggregate-repos', action='store_true', required=False,
                       default=False,
                       help='Combine all repos into a single list of resources.')

    group = register_show_subparser.add_mutually_exclusive_group(required=False)
    group.add_argument('-v', '--verbose', action='count', required=False,
                       default=0,
                       help='How verbose do you want the output to be.')

    register_show_subparser.set_defaults(func=_run_register_show)

    # get-registered
    get_registered_subparser = subparsers.add_parser('get-registered')
    group = get_registered_subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('-e', '--engine-name', type=str, required=False,
                       help='Engine name.')
    group.add_argument('-p', '--project-name', type=str, required=False,
                       help='Project name.')
    group.add_argument('-g', '--gem-name', type=str, required=False,
                       help='Gem name.')
    group.add_argument('-t', '--template-name', type=str, required=False,
                       help='Template name.')
    group.add_argument('-f', '--default-folder', type=str, required=False,
                       choices=['projects', 'gems', 'templates'],
                       help='The default folder.')
    group.add_argument('-r', '--repo-name', type=str, required=False,
                       help='A repo uri to register/remove.')

    get_registered_subparser.set_defaults(func=_run_get_registered)


if __name__ == "__main__":
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add subparsers
    the_subparsers = the_parser.add_subparsers(help='sub-command help')

    # add args to the parser
    add_args(the_parser, the_subparsers)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args)

    # return
    sys.exit(ret)
