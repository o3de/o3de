#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import json
import hashlib
import logging
import pathlib
import sys
import urllib.parse

from o3de import manifest, validation, utils, repo

logger = logging.getLogger('o3de.print_registration')
logging.basicConfig(format=utils.LOG_FORMAT)


def get_project_path(project_path: pathlib.Path, project_name: str) -> pathlib.Path:
    if not project_name and not project_path:
        logger.error(f'Must either specify a Project path or Project Name.')
        return None

    if not project_path:
        project_path = manifest.get_registered(project_name=project_name)
    if not project_path:
        logger.error(f'Unable to locate project path from the registered manifest json files:'
                     f' {str(pathlib.Path("~/.o3de/o3de_manifest.json").expanduser())}, engine.json')
        return None

    if not project_path.is_dir():
        logger.error(f'Project path {project_path} is not a folder.')
        return None

    return project_path


def print_this_engine(verbose: int = 0) -> int:
    this_engine_path = manifest.get_this_engine_path()
    print(f'This Engine:\n{json.dumps(str(this_engine_path), indent=4)}')
    if verbose > 0:
        return print_manifest_json_data([this_engine_path], 'This Engine',
                                        manifest.get_engine_json_data, 'engine_path')
    return 0


def print_engines(verbose: int = 0) -> int:
    engines_data = manifest.get_manifest_engines()
    print(f'Engine Paths:\n{json.dumps(engines_data, indent=4)}')

    if verbose > 0:
        return print_manifest_json_data(engines_data, 'Engine Jsons',
                                        manifest.get_engine_json_data, 'engine_path')
    return 0


def print_projects(verbose: int = 0) -> int:
    projects_data = manifest.get_all_projects()
    print(f'Project Paths:\n{json.dumps(projects_data, indent=4)}')

    if verbose > 0:
        return print_manifest_json_data(projects_data, 'Project Jsons',
                                        manifest.get_project_json_data, 'project_path')
    return 0


def print_gems(verbose: int = 0) -> int:
    gems_data = manifest.get_all_gems()
    print(f'Gem Paths:\n{json.dumps(gems_data, indent=4)}')

    if verbose > 0:
        return print_manifest_json_data(gems_data, 'Gem Jsons',
                                        manifest.get_gem_json_data, 'gem_path')
    return 0


def print_external_subdirectories(verbose: int = 0) -> int:
    external_subdirs_data = manifest.get_all_external_subdirectories(gems_json_data_by_path=dict())
    print(f'External Subdirectories:\n{json.dumps(external_subdirs_data, indent=4)}')
    return 0


def print_templates(verbose: int) -> int:
    templates_data = manifest.get_all_templates()
    print(f'Template Paths:\n{json.dumps(templates_data, indent=4)}')

    if verbose > 0:
        return print_manifest_json_data(templates_data, 'Template Jsons',
                                        manifest.get_template_json_data, 'template_path')
    return 0


def print_restricted(verbose: int) -> int:
    restricted_data = manifest.get_manifest_restricted()
    print(f'Restricted Paths:\n{json.dumps(restricted_data, indent=4)}')

    if verbose > 0:
        return print_manifest_json_data(restricted_data, 'Restricted Jsons',
                                        manifest.get_restricted_json_data, 'restricted_path')
    return 0


# Engine output methods
def print_engine_projects(verbose: int) -> int:
    engine_projects_data = manifest.get_engine_projects()
    print(f'Project Paths:\n{json.dumps(engine_projects_data, indent=4)}')

    if verbose > 0:
        return print_manifest_json_data(engine_projects_data, 'Project Jsons',
                                        manifest.get_project_json_data, 'project_path')
    return 0


def print_engine_gems(verbose: int) -> int:
    engine_gems_data = manifest.get_engine_gems()
    print(f'Gem Paths:\n{json.dumps(engine_gems_data, indent=4)}')

    if verbose > 0:
        return print_manifest_json_data(engine_gems_data, 'Gem Jsons',
                                        manifest.get_gem_json_data, 'gem_path')
    return 0


def print_engine_templates(verbose: int) -> int:
    engine_templates_data = manifest.get_engine_templates()
    print(f'Template Paths:\n{json.dumps(engine_templates_data, indent=4)}')

    if verbose > 0:
        return print_manifest_json_data(engine_templates_data, 'Template Jsons',
                                        manifest.get_template_json_data, 'template_path')
    return 0


def print_engine_external_subdirectories(verbose: int) -> int:
    external_subdirs_data = manifest.get_engine_external_subdirectories()
    print(f'External Subdirectories:\n{json.dumps(external_subdirs_data, indent=4)}')
    return 0


# Project output methods
def print_project_gems(verbose: int, project_path: pathlib.Path, project_name: str) -> int:
    project_path = get_project_path(project_path, project_name)
    if not project_path:
        return 1

    project_gems_data = manifest.get_project_gems(project_path)
    print(f'Gem Paths:\n{json.dumps(project_gems_data, indent=4)}')

    if verbose > 0:
        return print_manifest_json_data(project_gems_data, 'Gems Jsons',
                                        manifest.get_gem_json_data, 'gem_path')
    return 0


def print_project_external_subdirectories(verbose: int, project_path: pathlib.Path, project_name: str) -> int:
    project_path = get_project_path(project_path, project_name)
    if not project_path:
        return 1

    external_subdirs_data = manifest.get_project_external_subdirectories(project_path)
    print(f'External Subdirectories:\n{json.dumps(external_subdirs_data, indent=4)}')
    return 0


def print_project_templates(verbose: int, project_path: pathlib.Path, project_name: str) -> int:
    project_path = get_project_path(project_path, project_name)
    if not project_path:
        return 1

    project_templates_data = manifest.get_project_templates(project_path)
    print(f'Template Paths:\n{json.dumps(project_templates_data, indent=4)}')
    if verbose > 0:
        return print_manifest_json_data(project_templates_data, 'Template Jsons',
                                        manifest.get_template_json_data, 'template_path')
    return 0


def print_project_engine_name(verbose: int, project_path: pathlib.Path, project_name: str) -> int:
    project_path = get_project_path(project_path, project_name)
    if not project_path:
        return 1

    project_json_data = manifest.get_project_json_data(project_path=project_path)
    print(f'Engine Name:\n{project_json_data.get("engine", "")}')
    return 0


def print_all_projects(verbose: int) -> int:
    all_projects_data = manifest.get_all_projects()
    print(f'Project Paths:\n{json.dumps(all_projects_data, indent=4)}')

    if verbose > 0:
        return print_manifest_json_data(all_projects_data, 'Project Jsons',
                                        manifest.get_project_json_data, 'project_path')
    return 0


def print_all_gems(verbose: int, project_path: pathlib.Path = None, project_name: str = None) -> int:
    all_gems = manifest.get_manifest_gems()
    all_gems.extend(manifest.get_engine_gems())

    # If a project path or project name is supplied query the gems from that project, otherwise query the gems from
    # all projects
    project_path = get_project_path(project_path, project_name) if project_path or project_name else None
    projects = [project_path] if project_path else manifest.get_all_projects()
    for project in projects:
        all_gems.extend(manifest.get_project_gems(project))

    # Filter out duplicates
    all_gems = list(dict.fromkeys(all_gems))
    print(f'Gem Paths:\n{json.dumps(all_gems, indent=4)}')

    if verbose > 0:
        return print_manifest_json_data(all_gems, 'Gem Jsons',
                                        manifest.get_gem_json_data, 'gem_path')
    return 0


def print_all_external_subdirectories(verbose: int, project_path: pathlib.Path = None, project_name: str = None) -> int:
    all_external_subdirectories = manifest.get_manifest_external_subdirectories()
    all_external_subdirectories.extend(manifest.get_engine_external_subdirectories())

    # If a project path or project name is supplied query the external subdirectories from that project,
    # otherwise query the external subdirectories from all projects
    project_path = get_project_path(project_path, project_name) if project_path or project_name else None
    projects = [project_path] if project_path else manifest.get_all_projects()
    for project in projects:
        all_external_subdirectories.extend(manifest.get_project_external_subdirectories(project))

    # Filter out duplicates
    all_external_subdirectories = list(dict.fromkeys(all_external_subdirectories))
    print(f'External Subdirectories:\n{json.dumps(all_external_subdirectories, indent=4)}')
    return 0


def print_all_templates(verbose: int, project_path: pathlib.Path = None, project_name: str = None) -> int:
    all_templates = manifest.get_manifest_templates()
    all_templates.extend(manifest.get_engine_templates())

    # If a project path or project name is supplied query the templates from that project,
    # otherwise query the templates from all projects
    project_path = get_project_path(project_path, project_name) if project_path or project_name else None
    projects = [project_path] if project_path else manifest.get_all_projects()
    for project in projects:
        all_templates.extend(manifest.get_project_templates(project))

    # Filter out duplicates
    all_templates = list(dict.fromkeys(all_templates))
    print(f'Template Paths:\n{json.dumps(all_templates, indent=4)}')

    if verbose > 0:
        return print_manifest_json_data(all_templates, 'Template Jsons',
                                        manifest.get_template_json_data, 'template_path')
    return 0

def print_manifest_json_data(uri_json_data: list,
                             print_prefix: str, get_json_func: callable, get_json_data_kw: str) -> int:
    print('\n')
    print(f"{print_prefix}================================================")
    for manifest_uri in uri_json_data:
        # if it's not local it should be in the cache
        manifest_json_path, parsed_uri = repo.get_cache_file_uri(pathlib.Path(manifest_uri).as_posix())
        if parsed_uri.scheme not in ['http', 'https', 'ftp', 'ftps']:
            manifest_json_path = pathlib.Path(manifest_uri).resolve()

        json_data = get_json_func(**{get_json_data_kw: manifest_json_path})
        if json_data:
            print(manifest_json_path)
            print(json.dumps(json_data, indent=4) + '\n')
    return 0


def print_repos_data(repos_data: dict) -> int:
    print('\n')
    print("Repos================================================")
    cache_folder = manifest.get_o3de_cache_folder()
    for repo_uri in repos_data:
        cache_file, _ = repo.get_cache_file_uri(repo_uri)
        if validation.valid_o3de_repo_json(cache_file):
            with cache_file.open('r') as s:
                try:
                    repo_json_data = json.load(s)
                except json.JSONDecodeError as e:
                    logger.warning(f'{cache_file} failed to load: {str(e)}')
                else:
                    print(f'{repo_uri}/repo.json cached as:')
                    print(cache_file)
                    print(json.dumps(repo_json_data, indent=4))
        print('\n')
    return 0


def print_repos(verbose: int) -> int:
    repos_data = manifest.get_manifest_repos()
    print(json.dumps(repos_data, indent=4))

    if verbose > 0:
        return print_repos_data(repos_data)
    return 0


def register_show(verbose: int, project_path: pathlib.Path = None, project_name: str = None) -> int:
    json_data = manifest.load_o3de_manifest()
    print(f"{manifest.get_o3de_manifest()}:")
    print(json.dumps(json_data, indent=4))

    result = 0
    if verbose > 0:
        result = print_engines(verbose) or result
        result = print_all_projects(verbose) or result
        result = print_all_gems(verbose, project_path, project_name) or result
        result = print_all_templates(verbose, project_path, project_name) or result
        result = print_restricted(verbose) or result
        result = print_repos(verbose) or result

    return result


def _run_register_show(args: argparse) -> int:
    if args.this_engine:
        return print_this_engine(args.verbose)
    elif args.engines:
        return print_engines(args.verbose)
    elif args.projects:
        return print_projects(args.verbose)
    elif args.gems:
        return print_gems(args.verbose)
    elif args.external_subdirectories:
        return print_external_subdirectories(args.verbose)
    elif args.templates:
        return print_templates(args.verbose)
    elif args.repos:
        return print_repos(args.verbose)
    elif args.restricted:
        return print_restricted(args.verbose)

    elif args.engine_projects:
        return print_engine_projects(args.verbose)
    elif args.engine_gems:
        return print_engine_gems(args.verbose)
    elif args.engine_external_subdirectories:
        return print_engine_external_subdirectories(args.verbose)
    elif args.engine_templates:
        return print_engine_templates(args.verbose)

    elif args.project_gems:
        return print_project_gems(args.verbose, args.project_path, args.project_name)
    elif args.project_external_subdirectories:
        return print_project_external_subdirectories(args.verbose, args.project_path, args.project_name)
    elif args.project_templates:
        return print_project_templates(args.verbose, args.project_path, args.project_name)
    elif args.project_engine_name:
        return print_project_engine_name(args.verbose, args.project_path, args.project_name)

    elif args.all_projects:
        return print_all_projects(args.verbose)
    elif args.all_gems:
        return print_all_gems(args.verbose, args.project_path, args.project_name)
    elif args.all_external_subdirectories:
        return print_all_external_subdirectories(args.verbose, args.project_path, args.project_name)
    elif args.all_templates:
        return print_all_templates(args.verbose, args.project_path, args.project_name)

    else:
        return register_show(args.verbose, args.project_path, args.project_name)


def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file alone with: python print_registration.py --engine-projects
    :param parser: the caller passes an argparse parser like instance to this method
    """
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument('-te', '--this-engine', action='store_true', required=False,
                       default=False,
                       help='Output the current engine path.')

    group.add_argument('-e', '--engines', action='store_true', required=False,
                       default=False,
                       help='Output the engines registered in the global ~/.o3de/o3de_manifest.json.')
    group.add_argument('-p', '--projects', action='store_true', required=False,
                       default=False,
                       help='Output the projects registered in the global ~/.o3de/o3de_manifest.json.')
    group.add_argument('-g', '--gems', action='store_true', required=False,
                       default=False,
                       help='Output the gems registered in the global ~/.o3de/o3de_manifest.json.')
    group.add_argument('-t', '--templates', action='store_true', required=False,
                       default=False,
                       help='Output the templates registered in the global ~/.o3de/o3de_manifest.json.')
    group.add_argument('-r', '--repos', action='store_true', required=False,
                       default=False,
                       help='Output the repos registered in the global ~/.o3de/o3de_manifest.json. Ignores repos.')
    group.add_argument('-rs', '--restricted', action='store_true', required=False,
                       default=False,
                       help='Output the restricted directories registered in the global ~/.o3de/o3de_manifest.json.')
    group.add_argument('-es', '--external-subdirectories', action='store_true', required=False,
                       default=False,
                       help='Output the external subdirectories registered in the global ~/.o3de/o3de_manifest.json.')

    group.add_argument('-ep', '--engine-projects', action='store_true', required=False,
                       default=False,
                       help='Output the projects registered in the current engine engine.json. Ignores repos.')
    group.add_argument('-eg', '--engine-gems', action='store_true', required=False,
                       default=False,
                       help='Output the gems registered in the current engine engine.json. Ignores repos')
    group.add_argument('-et', '--engine-templates', action='store_true', required=False,
                       default=False,
                       help='Output the templates registered in the current engine engine.json. Ignores repos.')
    group.add_argument('-ers', '--engine-restricted', action='store_true', required=False,
                       default=False,
                       help='Output the restricted directories registered in the current engine engine.json.')
    group.add_argument('-ees', '--engine-external-subdirectories', action='store_true', required=False,
                       default=False,
                       help='Output the external subdirectories registered in the current engine engine.json.')

    group.add_argument('-pg', '--project-gems', action='store_true',
                       default=False,
                       help='Returns the gems registered with the project.json.')
    group.add_argument('-pt', '--project-templates', action='store_true',
                       default=False,
                       help='Returns the templates registered with the project.json.')
    group.add_argument('-prs', '--project-restricted', action='store_true',
                       default=False,
                       help='Returns the restricted directories registered with the project.json.')
    group.add_argument('-pes', '--project-external-subdirectories', action='store_true',
                       default=False,
                       help='Returns the external subdirectories register with the project.json.')
    group.add_argument('-pen', '--project-engine-name', action='store_true',
                       default=False,
                       help='Returns the "engine" name identifier registered with the project.json.')

    group.add_argument('-ap', '--all-projects', action='store_true', required=False,
                       default=False,
                       help='Output all projects registered in the ~/.o3de/o3de_manifest.json and the current engine.json. Ignores repos.')
    group.add_argument('-ag', '--all-gems', action='store_true', required=False,
                       default=False,
                       help='Output all gems registered in the ~/.o3de/o3de_manifest.json and the current engine.json.'
                            ' If --project-path or --project-name option is supplied, outputs gems registered in'
                            ' that project\'s project.json otherwise outputs registered gems from all registered projects.'
                            ' Ignores repos')
    group.add_argument('-at', '--all-templates', action='store_true', required=False,
                       default=False,
                       help='Output all templates registered in the ~/.o3de/o3de_manifest.json and the current engine.json.'
                            ' If --project-path or --project-name option is supplied, outputs templates registered in'
                            ' that project\'s project.json otherwise outputs registered templates from all registered'
                            ' projects. Ignores repos')
    group.add_argument('-ares', '--all-restricted', action='store_true', required=False,
                       default=False,
                       help='Output all restricted directory registered in the ~/.o3de/o3de_manifest.json and the current engine.json.'
                            ' If --project-path or --project-name option is supplied, outputs restricted'
                            ' directories registered in that project\'s project.json otherwise outputs restricted'
                            ' directories registered from all registered projects. Ignores repos')
    group.add_argument('-aes', '--all-external-subdirectories', action='store_true',
                       default=False,
                       help='Output all external subdirectories registered in the ~/.o3de/o3de_manifest.json and the current engine.json.'
                            ' If --project-path or --project-name options is supplied, outputs external'
                            ' subdirectories registered in that project\'s project.json otherwise outputs external'
                            ' subdirectories registered from all registered projects. Ignores repos')

    parser.add_argument('-v', '--verbose', action='count', required=False,
                        default=0,
                        help='How verbose do you want the output to be.')

    project_group = parser.add_mutually_exclusive_group(required=False)
    project_group.add_argument('-pp', '--project-path', type=pathlib.Path,
                       help='The path to a project.')
    project_group.add_argument('-pn', '--project-name', type=str,
                       help='The name of a project.')

    parser.set_defaults(func=_run_register_show)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py register-show --engine-projects
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    register_show_subparser = subparsers.add_parser('register-show')
    add_parser_args(register_show_subparser)


def main():
    """
    Runs print_registration.py script as standalone script
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


if __name__ == "__main__":
    main()
