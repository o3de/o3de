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

import argparse
import json
import hashlib
import logging
import sys
import urllib.parse

from o3de import manifest, validation

logger = logging.getLogger()
logging.basicConfig()

def print_this_engine(verbose: int) -> None:
    engine_data = manifest.get_this_engine()
    print(json.dumps(engine_data, indent=4))
    if verbose > 0:
        print_engines_data(engine_data)


def print_engines(verbose: int) -> None:
    engines_data = manifest.get_engines()
    print(json.dumps(engines_data, indent=4))
    if verbose > 0:
        print_engines_data(engines_data)


def print_projects(verbose: int) -> None:
    projects_data = manifest.get_projects()
    print(json.dumps(projects_data, indent=4))
    if verbose > 0:
        print_projects_data(projects_data)


def print_gems(verbose: int) -> None:
    gems_data = manifest.get_gems()
    print(json.dumps(gems_data, indent=4))
    if verbose > 0:
        print_gems_data(gems_data)


def print_templates(verbose: int) -> None:
    templates_data = manifest.get_templates()
    print(json.dumps(templates_data, indent=4))
    if verbose > 0:
        print_templates_data(templates_data)


def print_restricted(verbose: int) -> None:
    restricted_data = manifest.get_restricted()
    print(json.dumps(restricted_data, indent=4))
    if verbose > 0:
        print_restricted_data(restricted_data)

def print_engine_projects(verbose: int) -> None:
    engine_projects_data = manifest.get_engine_projects()
    print(json.dumps(engine_projects_data, indent=4))
    if verbose > 0:
        print_projects_data(engine_projects_data)


def print_engine_gems(verbose: int) -> None:
    engine_gems_data = manifest.get_engine_gems()
    print(json.dumps(engine_gems_data, indent=4))
    if verbose > 0:
        print_gems_data(engine_gems_data)


def print_engine_templates(verbose: int) -> None:
    engine_templates_data = manifest.get_engine_templates()
    print(json.dumps(engine_templates_data, indent=4))
    if verbose > 0:
        print_templates_data(engine_templates_data)


def print_engine_restricted(verbose: int) -> None:
    engine_restricted_data = manifest.get_engine_restricted()
    print(json.dumps(engine_restricted_data, indent=4))
    if verbose > 0:
        print_restricted_data(engine_restricted_data)


def print_engine_external_subdirectories(verbose: int) -> None:
    external_subdirs_data = manifest.get_engine_external_subdirectories()
    print(json.dumps(external_subdirs_data, indent=4))


def print_all_projects(verbose: int) -> None:
    all_projects_data = manifest.get_all_projects()
    print(json.dumps(all_projects_data, indent=4))
    if verbose > 0:
        print_projects_data(all_projects_data)


def print_all_gems(verbose: int) -> None:
    all_gems_data = manifest.get_all_gems()
    print(json.dumps(all_gems_data, indent=4))
    if verbose > 0:
        print_gems_data(all_gems_data)


def print_all_templates(verbose: int) -> None:
    all_templates_data = manifest.get_all_templates()
    print(json.dumps(all_templates_data, indent=4))
    if verbose > 0:
        print_templates_data(all_templates_data)


def print_all_restricted(verbose: int) -> None:
    all_restricted_data = manifest.get_all_restricted()
    print(json.dumps(all_restricted_data, indent=4))
    if verbose > 0:
        print_restricted_data(all_restricted_data)


def print_engines_data(engines_data: dict) -> None:
    print('\n')
    print("Engines================================================")
    for engine_object in engines_data:
        # if it's not local it should be in the cache
        engine_uri = engine_object['path']
        parsed_uri = urllib.parse.urlparse(engine_uri)
        if parsed_uri.scheme == 'http' or \
                parsed_uri.scheme == 'https' or \
                parsed_uri.scheme == 'ftp' or \
                parsed_uri.scheme == 'ftps':
            repo_sha256 = hashlib.sha256(engine_uri.encode())
            cache_folder = manifest.get_o3de_cache_folder()
            engine = cache_folder / str(repo_sha256.hexdigest() + '.json')
            print(f'{engine_uri}/engine.json cached as:')
        else:
            engine_json = pathlib.Path(engine_uri).resolve() / 'engine.json'

        with engine_json.open('r') as f:
            try:
                engine_json_data = json.load(f)
            except json.JSONDecodeError as e:
                logger.warn(f'{engine_json} failed to load: {str(e)}')
            else:
                print(engine_json)
                print(json.dumps(engine_json_data, indent=4))
        print('\n')


def print_projects_data(projects_data: dict) -> None:
    print('\n')
    print("Projects================================================")
    for project_uri in projects_data:
        # if it's not local it should be in the cache
        parsed_uri = urllib.parse.urlparse(project_uri)
        if parsed_uri.scheme == 'http' or \
                parsed_uri.scheme == 'https' or \
                parsed_uri.scheme == 'ftp' or \
                parsed_uri.scheme == 'ftps':
            repo_sha256 = hashlib.sha256(project_uri.encode())
            cache_folder = manifest.get_o3de_cache_folder()
            project_json = cache_folder / str(repo_sha256.hexdigest() + '.json')
        else:
            project_json = pathlib.Path(project_uri).resolve() / 'project.json'

        with project_json.open('r') as f:
            try:
                project_json_data = json.load(f)
            except json.JSONDecodeError as e:
                logger.warn(f'{project_json} failed to load: {str(e)}')
            else:
                print(project_json)
                print(json.dumps(project_json_data, indent=4))
        print('\n')


def print_gems_data(gems_data: dict) -> None:
    print('\n')
    print("Gems================================================")
    for gem_uri in gems_data:
        # if it's not local it should be in the cache
        parsed_uri = urllib.parse.urlparse(gem_uri)
        if parsed_uri.scheme == 'http' or \
                parsed_uri.scheme == 'https' or \
                parsed_uri.scheme == 'ftp' or \
                parsed_uri.scheme == 'ftps':
            repo_sha256 = hashlib.sha256(gem_uri.encode())
            cache_folder = manifest.get_o3de_cache_folder()
            gem_json = cache_folder / str(repo_sha256.hexdigest() + '.json')
        else:
            gem_json = pathlib.Path(gem_uri).resolve() / 'gem.json'

        with gem_json.open('r') as f:
            try:
                gem_json_data = json.load(f)
            except json.JSONDecodeError as e:
                logger.warn(f'{gem_json} failed to load: {str(e)}')
            else:
                print(gem_json)
                print(json.dumps(gem_json_data, indent=4))
        print('\n')


def print_templates_data(templates_data: dict) -> None:
    print('\n')
    print("Templates================================================")
    for template_uri in templates_data:
        # if it's not local it should be in the cache
        parsed_uri = urllib.parse.urlparse(template_uri)
        if parsed_uri.scheme == 'http' or \
                parsed_uri.scheme == 'https' or \
                parsed_uri.scheme == 'ftp' or \
                parsed_uri.scheme == 'ftps':
            repo_sha256 = hashlib.sha256(template_uri.encode())
            cache_folder = manifest.get_o3de_cache_folder()
            template_json = cache_folder / str(repo_sha256.hexdigest() + '.json')
        else:
            template_json = pathlib.Path(template_uri).resolve() / 'template.json'

        with template_json.open('r') as f:
            try:
                template_json_data = json.load(f)
            except json.JSONDecodeError as e:
                logger.warn(f'{template_json} failed to load: {str(e)}')
            else:
                print(template_json)
                print(json.dumps(template_json_data, indent=4))
        print('\n')


def print_repos_data(repos_data: dict) -> None:
    print('\n')
    print("Repos================================================")
    cache_folder = manifest.get_o3de_cache_folder()
    for repo_uri in repos_data:
        repo_sha256 = hashlib.sha256(repo_uri.encode())
        cache_file = cache_folder / str(repo_sha256.hexdigest() + '.json')
        if validation.valid_o3de_repo_json(cache_file):
            with cache_file.open('r') as s:
                try:
                    repo_json_data = json.load(s)
                except json.JSONDecodeError as e:
                    logger.warn(f'{cache_file} failed to load: {str(e)}')
                else:
                    print(f'{repo_uri}/repo.json cached as:')
                    print(cache_file)
                    print(json.dumps(repo_json_data, indent=4))
        print('\n')


def print_restricted_data(restricted_data: dict) -> None:
    print('\n')
    print("Restricted================================================")
    for restricted_path in restricted_data:
        restricted_json = pathlib.Path(restricted_path).resolve() / 'restricted.json'
        with restricted_json.open('r') as f:
            try:
                restricted_json_data = json.load(f)
            except json.JSONDecodeError as e:
                logger.warn(f'{restricted_json} failed to load: {str(e)}')
            else:
                print(restricted_json)
                print(json.dumps(restricted_json_data, indent=4))
        print('\n')


def register_show_repos(verbose: int) -> None:
    repos_data = get_repos()
    print(json.dumps(repos_data, indent=4))
    if verbose > 0:
        print_repos_data(repos_data)


def register_show(verbose: int) -> None:
    json_data = manifest.load_o3de_manifest()
    print(f"{manifest.get_o3de_manifest()}:")
    print(json.dumps(json_data, indent=4))

    if verbose > 0:
        print_engines_data(manifest.get_engines())
        print_projects_data(manifest.get_all_projects())
        print_gems_data(manifest.get_gems())
        print_templates_data(manifest.get_all_templates())
        print_restricted_data(manifest.get_all_restricted())
        print_repos_data(manifest.get_repos())


def _run_register_show(args: argparse) -> int:
    if args.override_home_folder:
        manifest.override_home_folder = args.override_home_folder

    if args.this_engine:
        print_this_engine(args.verbose)
        return 0

    elif args.engines:
        print_engines(args.verbose)
        return 0
    elif args.projects:
        print_projects(args.verbose)
        return 0
    elif args.gems:
        print_gems(args.verbose)
        return 0
    elif args.templates:
        print_templates(args.verbose)
        return 0
    elif args.repos:
        register_show_repos(args.verbose)
        return 0
    elif args.restricted:
        print_restricted(args.verbose)
        return 0

    elif args.engine_projects:
        print_engine_projects(args.verbose)
        return 0
    elif args.engine_gems:
        print_engine_gems(args.verbose)
        return 0
    elif args.engine_templates:
        print_engine_templates(args.verbose)
        return 0
    elif args.engine_restricted:
        print_engine_restricted(args.verbose)
        return 0
    elif args.engine_external_subdirectories:
        print_engine_external_subdirectories(args.verbose)
        return 0

    elif args.all_projects:
        print_all_projects(args.verbose)
        return 0
    elif args.all_gems:
        print_all_gems(args.verbose)
        return 0
    elif args.all_templates:
        print_all_templates(args.verbose)
        return 0
    elif args.all_restricted:
        print_all_restricted(args.verbose)
        return 0

    elif args.downloadables:
        print_downloadables(args.verbose)
        return 0
    if args.downloadable_engines:
        print_downloadable_engines(args.verbose)
        return 0
    elif args.downloadable_projects:
        print_downloadable_projects(args.verbose)
        return 0
    elif args.downloadable_gems:
        print_downloadable_gems(args.verbose)
        return 0
    elif args.downloadable_templates:
        print_downloadable_templates(args.verbose)
        return 0
    else:
        register_show(args.verbose)
        return 0


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
                       help='Just the local engines.')

    group.add_argument('-e', '--engines', action='store_true', required=False,
                       default=False,
                       help='Just the local engines.')
    group.add_argument('-p', '--projects', action='store_true', required=False,
                       default=False,
                       help='Just the local projects.')
    group.add_argument('-g', '--gems', action='store_true', required=False,
                       default=False,
                       help='Just the local gems.')
    group.add_argument('-t', '--templates', action='store_true', required=False,
                       default=False,
                       help='Just the local templates.')
    group.add_argument('-r', '--repos', action='store_true', required=False,
                       default=False,
                       help='Just the local repos. Ignores repos.')
    group.add_argument('-rs', '--restricted', action='store_true', required=False,
                       default=False,
                       help='The local restricted folders.')

    group.add_argument('-ep', '--engine-projects', action='store_true', required=False,
                       default=False,
                       help='Just the local projects. Ignores repos.')
    group.add_argument('-eg', '--engine-gems', action='store_true', required=False,
                       default=False,
                       help='Just the local gems. Ignores repos')
    group.add_argument('-et', '--engine-templates', action='store_true', required=False,
                       default=False,
                       help='Just the local templates. Ignores repos.')
    group.add_argument('-ers', '--engine-restricted', action='store_true', required=False,
                       default=False,
                       help='The restricted folders.')
    group.add_argument('-x', '--engine-external-subdirectories', action='store_true', required=False,
                       default=False,
                       help='The external subdirectories.')

    group.add_argument('-ap', '--all-projects', action='store_true', required=False,
                       default=False,
                       help='Just the local projects. Ignores repos.')
    group.add_argument('-ag', '--all-gems', action='store_true', required=False,
                       default=False,
                       help='Just the local gems. Ignores repos')
    group.add_argument('-at', '--all-templates', action='store_true', required=False,
                       default=False,
                       help='Just the local templates. Ignores repos.')
    group.add_argument('-ars', '--all-restricted', action='store_true', required=False,
                       default=False,
                       help='The restricted folders.')

    group.add_argument('-d', '--downloadables', action='store_true', required=False,
                       default=False,
                       help='Combine all repos into a single list of resources.')
    group.add_argument('-de', '--downloadable-engines', action='store_true', required=False,
                       default=False,
                       help='Combine all repos engines into a single list of resources.')
    group.add_argument('-dp', '--downloadable-projects', action='store_true', required=False,
                       default=False,
                       help='Combine all repos projects into a single list of resources.')
    group.add_argument('-dg', '--downloadable-gems', action='store_true', required=False,
                       default=False,
                       help='Combine all repos gems into a single list of resources.')
    group.add_argument('-dt', '--downloadable-templates', action='store_true', required=False,
                       default=False,
                       help='Combine all repos templates into a single list of resources.')

    parser.add_argument('-v', '--verbose', action='count', required=False,
                                         default=0,
                                         help='How verbose do you want the output to be.')

    parser.add_argument('-ohf', '--override-home-folder', type=str, required=False,
                                         help='By default the home folder is the user folder, override it to this folder.')

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
