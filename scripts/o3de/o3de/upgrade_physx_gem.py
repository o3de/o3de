#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

"""
Contains command to upgrade PhysX gem in a project's project.json file, as well as references in CMakeLists.txt and setreg files, from PhysX4 to PhysX5.
"""

import argparse
import logging
import json
import pathlib
import re
import sys

from o3de import compatibility, manifest, project_properties, utils

logging.basicConfig(format=utils.LOG_FORMAT)
logger = logging.getLogger('o3de.upgrade_physx_gem')
logger.setLevel(logging.INFO)


def contains_exact_word(text: str, word: str) -> bool:
    """
    Determine whether *word* occurs as a whole word within *text*.

    This will not match substrings.  For example ``PhysX`` will be found
    in ``Gem::PhysX`` but **not** in ``PhysX5`` or ``SuperPhysXGem``.
    The check is case sensitive and uses ``\b`` word boundaries, escaping the
    search term so that regular expression metacharacters in *word* are
    treated literally.

    :param text: the string to search
    :param word: the word to look for
    :return: ``True`` if the word is present, ``False`` otherwise
    """
    pattern = rf"\b{re.escape(word)}\b"
    return re.search(pattern, text) is not None


def replace_exact_word(text: str, old: str, new: str) -> str:
    """
    Replace *old* with *new* when *old* appears as a standalone word in *text*.

    This behaves like ``str.replace`` but avoids touching substrings.  For
    example, ``replace_exact_word("PhysX5 and PhysX", "PhysX", "PX")``
    will return ``"PhysX5 and PX"`` the ``PhysX`` inside ``PhysX5`` is
    untouched.

    The function is case-sensitive and escapes the search term for use in a
    regular expression, using ``\b`` boundaries to ensure whole-word matches.

    :param text: string to modify
    :param old: word to look for
    :param new: replacement text
    :return: modified string with the replacements applied
    """
    pattern = rf"\b{re.escape(old)}\b"
    return re.sub(pattern, new, text)

def upgrade_physx_gem_in_gem(gem_path: pathlib.Path = None,
                             dry_run: bool = False) -> int:
    """
    Upgrade PhysX(4) references for a gem
    :param gem_path: path to the gem to upgrade any PhysX references
    :param dry_run: check version compatibility without modifying anything
    :return: 0 for success or non 0 failure code
    """
    # we need either a project name or path
    assert gem_path, "Gem path required"

    if not gem_path.is_dir():
        logger.error(f'Gem path {gem_path} is not a folder.')
        return 1

    gem_json_file = gem_path / 'gem.json'
    if not gem_json_file.is_file():
        logger.error(f'Unable to locate gem.json file in gem path {gem_path}.')
        return 1

    logger.info(f'Upgrading PhysX in Gem {gem_path}...')

    # Locate any setreg files
    physx_setreg_files = []
    setreg_files = list(gem_path.glob('**/*.setreg'))
    for setreg_file in setreg_files:
        logger.info(f'Checking setreg file {setreg_file} for PhysX references...')
        with open(setreg_file, 'r') as f:
            setreg_contents = f.read()
            if contains_exact_word(setreg_contents, 'PhysX') or contains_exact_word(setreg_contents, 'PhysX.Debug'):
                physx_setreg_files.append(setreg_file)

    # Locate in all CmakeLists.txt files
    physx_cmakelists_files = []
    cmakelists_files = list(gem_path.glob('**/CMakeLists.txt'))
    for cmakelists_file in cmakelists_files:
        logger.info(f'Checking CMakeLists.txt file {cmakelists_file} for PhysX references...')
        with open(cmakelists_file, 'r') as f:
            cmakelists_contents = f.read()
            # look only for the original PhysX gem references, not already upgraded names
            if contains_exact_word(cmakelists_contents, 'Gem::PhysX') or contains_exact_word(cmakelists_contents, 'Gem::PhysX.Debug'):
                physx_cmakelists_files.append(cmakelists_file)

    # Process the setreg files
    for setreg_file in physx_setreg_files:
        logger.info(f'Processing setreg file {setreg_file}...')

        with open(setreg_file, 'r') as f:
            setreg_contents = f.read()
        setreg_contents = replace_exact_word(setreg_contents, 'PhysX.Debug', 'PhysX5.Debug')
        setreg_contents = replace_exact_word(setreg_contents, 'PhysX', 'PhysX5')
        if dry_run:
            logger.info(f'Would update setreg file {setreg_file} with contents:\n{setreg_contents}')
        else:
            with open(setreg_file, 'w') as f:
                f.write(setreg_contents)

    # Process the CMakeLists.txt files
    for cmakelists_file in physx_cmakelists_files:
        logger.info(f'Processing CMakeLists.txt file {cmakelists_file}...')
        with open(cmakelists_file, 'r') as f:
            cmakelists_contents = f.read()
        cmakelists_contents = replace_exact_word(cmakelists_contents,'Gem::PhysX', 'Gem::PhysX5')
        cmakelists_contents = replace_exact_word(cmakelists_contents,'Gem::PhysX.Static', 'Gem::PhysX5.Static')
        cmakelists_contents = replace_exact_word(cmakelists_contents,'Gem::PhysX.Debug', 'Gem::PhysX5.Debug')
        if dry_run:
            logger.info(f'Would update CMakeLists.txt file {cmakelists_file} with contents:\n{cmakelists_contents}')
        else:
            with open(cmakelists_file, 'w') as f:
                f.write(cmakelists_contents)

    # Update gems.json
    with open(gem_json_file, 'r') as f:
        gem_json_contents = json.load(f)
        gem_names = gem_json_contents.get('dependencies', [])
        updated_gem_names = []
        updated = False
        for gem_name in gem_names:
            if gem_name == 'PhysX':
                updated_gem_names.append('PhysX5')
                updated = True
            elif gem_name == 'PhysXDebug':
                updated_gem_names.append('PhysX5Debug')
                updated = True
            else:
                updated_gem_names.append(gem_name)
        if updated:
            gem_json_contents['dependencies'] = updated_gem_names
            updated_json = json.dumps(gem_json_contents, indent=4)
            if dry_run:
                logger.info(f'Would update gem.json file {gem_json_file} with contents:\n{updated_json}')
            else:
                with open(gem_json_file, 'w') as f:
                    f.write(updated_json)

def upgrade_physx_gem_in_project(project_name: str = None,
                                 project_path: pathlib.Path = None,
                                 dry_run: bool = False) -> int:
    """
    Upgrade PhysX(4) references for a project
    :param project_name: name of to the project to upgrade any PhysX references
    :param project_path: path to the project to upgrade any PhysX references
    :param dry_run: check version compatibility without modifying anything
    :return: 0 for success or non 0 failure code
    """
    # we need either a project name or path
    if not project_name and not project_path:
        logger.error(f'Must either specify a Project path or Project Name.')
        return 1

    # if project name resolve it into a path
    if project_name or project_path:
        if project_name and not project_path:
            project_path = manifest.get_registered(project_name=project_name)
        if not project_path:
            logger.error(f'Unable to locate project path from the registered manifest.json files:'
                        f' {str(pathlib.Path.home() / ".o3de/manifest.json")}, engine.json')
            return 1
    elif gem_path:
        if not gem_path.is_dir():
            logger.error(f'Gem path {gem_path} is not a folder.')
    else:
        logger.error(f'Must either specify a Project path or Project Name, or a Gem path or Gem name.')
        return 1


    project_path = pathlib.Path(project_path).resolve()
    if not project_path.is_dir():
        logger.error(f'Project path {project_path} is not a folder.')
        return 1
    
    logger.info(f'Upgrading PhysX Gem in project {project_path}...')

    # Skip the 'Cache' folder in the project folder completely
    cache_skip = project_path / 'Cache'

    # Locate any setreg files
    physx_setreg_files = []
    setreg_files = list(project_path.glob('**/*.setreg'))
    for setreg_file in setreg_files:
        if str(setreg_file.absolute()).startswith(str(cache_skip.absolute())):
            continue
        logger.debug(f'Checking setreg file {setreg_file} for PhysX references...')
        with open(setreg_file, 'r') as f:
            setreg_contents = f.read()
            if contains_exact_word(setreg_contents, 'PhysX') or contains_exact_word(setreg_contents, 'PhysX.Debug'):
                physx_setreg_files.append(setreg_file)

    # Locate in all CmakeLists.txt files
    physx_cmakelists_files = []
    cmakelists_files = list(project_path.glob('**/CMakeLists.txt')) 
    for cmakelists_file in cmakelists_files:
        logger.debug(f'Checking CMakeLists.txt file {cmakelists_file} for PhysX references...')
        with open(cmakelists_file, 'r') as f:
            cmakelists_contents = f.read()
            # look only for the original PhysX gem references, not already upgraded names
            if contains_exact_word(cmakelists_contents, 'Gem::PhysX') or contains_exact_word(cmakelists_contents, 'Gem::PhysX.Static') or contains_exact_word(cmakelists_contents, 'Gem::PhysX.Debug'):
                physx_cmakelists_files.append(cmakelists_file)

    # Locate in all enabled_gems.cmake files
    physx_enabled_gems_files = []
    enabled_gems_files = list(project_path.glob('**/enabled_gems.cmake'))
    for enabled_gems_file in enabled_gems_files:
        logger.debug(f'Checking enabled_gems.cmake file {enabled_gems_file} for PhysX references...')
        with open(enabled_gems_file, 'r') as f:
            enabled_gems_contents = f.read()
            # look only for the original PhysX gem references, not already upgraded names
            if contains_exact_word(enabled_gems_contents, 'PhysX') or contains_exact_word(enabled_gems_contents, 'PhysX.Debug'):
                physx_enabled_gems_files.append(enabled_gems_file)

    # Locate the project.json file
    project_json_file = project_path / 'project.json'
    if not project_json_file.is_file():
        logger.error(f'Unable to locate project.json file in project path {project_path}.')
        return 1
    
    # Process the setreg files
    for setreg_file in physx_setreg_files:
        logger.info(f'Processing setreg file {setreg_file}...')
        
        with open(setreg_file, 'r') as f:
            setreg_contents = f.read()
        setreg_contents = replace_exact_word(setreg_contents, 'PhysX.Debug', 'PhysX5.Debug')
        setreg_contents = replace_exact_word(setreg_contents, 'PhysX', 'PhysX5')
        if dry_run:
            logger.info(f'Would update setreg file {setreg_file} with contents:\n{setreg_contents}')
        else:
            with open(setreg_file, 'w') as f:
                f.write(setreg_contents)

    # Process the CMakeLists.txt files
    for cmakelists_file in physx_cmakelists_files:
        logger.info(f'Processing CMakeLists.txt file {cmakelists_file}...')
        with open(cmakelists_file, 'r') as f:
            cmakelists_contents = f.read()
        cmakelists_contents = replace_exact_word(cmakelists_contents,'Gem::PhysX', 'Gem::PhysX5')
        cmakelists_contents = replace_exact_word(cmakelists_contents,'Gem::PhysX.Static', 'Gem::PhysX5.Static')
        cmakelists_contents = replace_exact_word(cmakelists_contents,'Gem::PhysX.Debug', 'Gem::PhysX5.Debug')
        if dry_run:
            logger.info(f'Would update CMakeLists.txt file {cmakelists_file} with contents:\n{cmakelists_contents}')
        else:
            with open(cmakelists_file, 'w') as f:
                f.write(cmakelists_contents)

    # Process the CMakeLists.txt files
    for physx_enabled_gems_file in physx_enabled_gems_files:
        logger.info(f'Processing enabled_gems.cmake file {physx_enabled_gems_file}...')
        with open(physx_enabled_gems_file, 'r') as f:
            enabled_gems_contents = f.read()
        enabled_gems_contents = replace_exact_word(enabled_gems_contents,'PhysX', 'PhysX5')
        enabled_gems_contents = replace_exact_word(enabled_gems_contents,'PhysX.Debug', 'PhysX5.Debug')
        if dry_run:
            logger.info(f'Would update enabled_gems.cmake file {physx_enabled_gems_file} with contents:\n{enabled_gems_contents}')
        else:
            with open(physx_enabled_gems_file, 'w') as f:
                f.write(enabled_gems_contents)

    # Update project.json
    with open(project_json_file, 'r') as f:
        project_json_contents = json.load(f)
        gem_names = project_json_contents.get('gem_names', [])
        updated_gem_names = []
        updated = False
        for gem_name in gem_names:
            if gem_name == 'PhysX':
                updated_gem_names.append('PhysX5')
                updated = True
            elif gem_name == 'PhysXDebug':
                updated_gem_names.append('PhysX5Debug')
                updated = True
            else:
                updated_gem_names.append(gem_name)
        if updated:
            project_json_contents['gem_names'] = updated_gem_names
            updated_json = json.dumps(project_json_contents, indent=4)
            if dry_run:
                logger.info(f'Would update project.json file {project_json_file} with contents:\n{updated_json}')
            else:
                with open(project_json_file, 'w') as f:
                    f.write(updated_json)


def _run_upgrade_physx_gem_in_project(args: argparse) -> int:
    if args.project_name or args.project_path:
        ret = upgrade_physx_gem_in_project(project_name=args.project_name,
                                        project_path=args.project_path,
                                        dry_run=args.dry_run)
        if ret != 0:
            return ret

    if args.gem_path:
        ret = upgrade_physx_gem_in_gem(gem_path=args.gem_path,
                                       dry_run=args.dry_run)
        if ret != 0:
            return ret

    return 0


def add_parser_args(parser):
    """
    add_parser_args is called to add arguments to each command such that it can be
    invoked locally or added by a central python file.
    Ex. Directly run from this file with: python enable_gem.py --project-path "D:/TestProject" --gem-path "D:/TestGem"
    :param parser: the caller passes an argparse parser like instance to this method
    """

    # Sub-commands should declare their own verbosity flag, if desired
    utils.add_verbosity_arg(parser)

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                       help='The path to the project to upgrade physx.')
    group.add_argument('-pn', '--project-name', type=str, required=False,
                       help='The name of the project to upgrade physx.')
    group.add_argument('-gp', '--gem-path', type=pathlib.Path, required=False,
                       help='The path to the gem to upgrade physx.')
    group = parser.add_argument_group()
    group.add_argument('-dry', '--dry-run', required=False, action='store_true', default=False,
                       help='Performs a dry run, reporting the result without changing anything.')

    parser.set_defaults(func=_run_upgrade_physx_gem_in_project)


def add_args(subparsers) -> None:
    """
    add_args is called to add subparsers arguments to each command such that it can be
    a central python file such as o3de.py.
    It can be run from the o3de.py script as follows
    call add_args and execute: python o3de.py upgrade-physx-gem --project-path "D:/TestProject"
    :param subparsers: the caller instantiates subparsers and passes it in here
    """
    enable_gem_project_subparser = subparsers.add_parser('upgrade-physx-gem',
                                                        help='Upgrade the PhysX4 Gem in a project to PhysX5.')
    add_parser_args(enable_gem_project_subparser)


def main():
    """
    Runs upgrade_physx_gem.py script as standalone script
    """
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add args to the parser
    add_parser_args(the_parser)

    # parse args
    the_args = the_parser.parse_args()

    # run
    ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1
    logger.info('Success!' if ret == 0 else 'Completed with issues: result {}'.format(ret))

    # return
    sys.exit(ret)


if __name__ == "__main__":
    main()
