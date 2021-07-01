#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
from __future__ import absolute_import
import os
import re
import json
import sys
try:
    import six
except ImportError:
    import pip
    pip.main(['install', 'six', '--ignore-installed', '-q'])
    import six
from pathlib import Path

this_file_path = os.path.dirname(os.path.realpath(__file__))

# resolve symlinks and eliminate ".." components
engine_root_path = Path(__file__).resolve().parents[3]
    
def convert_glob_pattern_to_regex_pattern(glob_pattern):
    # switch to forward slashes because way easier to pattern match against
    pattern = re.sub(r'\\', r'/', glob_pattern)

    # Replace the dots and question marks
    pattern = re.sub(r'\.', r'\\.', pattern)
    pattern = re.sub(r'\?', r'.', pattern)

    # Handle the * vs ** expansions
    pattern = re.sub(r'([^*])\*($|[^*])', r'\1[^/\\\\]*\2', pattern)
    pattern = re.sub(r'\*\*/', r'(.*/)?', pattern)
    pattern = re.sub(r'\*\*', r'.*', pattern)

    # replace the forward slashes with [/\\] so it works on PC/unix
    pattern = re.sub(r'([^^])/', r'\1[/\\\\]', pattern)
    return pattern

# Convert the package json into a pair of regexes we can use to look for includes and excludes
def convert_glob_list_to_regex_list(filelist, prefix):
    includes = []
    excludes = []
    for key, value in six.iteritems(filelist):
        glob_pattern = os.path.join(prefix, key)
        if isinstance(value, dict):
            (sub_includes, sub_excludes) = convert_glob_list_to_regex_list(value, glob_pattern)
            includes.extend(sub_includes)
            excludes.extend(sub_excludes)
        else:
            # Simulate what glob would do with file walking to scope the * within a directory
            # and ** across directories
            regex_pattern = convert_glob_pattern_to_regex_pattern(os.path.normpath(glob_pattern))

            # Deal with the commands. include/exclude are straight forward. Moves/renames are to be considered
            # includes, and we will stick with validating the original contents for now
            if value == "#include":
                includes.append(regex_pattern)
            elif value == "#exclude":
                excludes.append(regex_pattern)
            elif value.startswith('#move:'):
                includes.append(regex_pattern)
            elif value.startswith('#rename:'):
                includes.append(regex_pattern)
            else:
                pass
    return (includes, excludes)

def generate_excludes_for_platform(root, platform):
    if platform == 'all':
        platform_exclusions_filename = os.path.join(this_file_path, 'platform_exclusions.json')
        with open(platform_exclusions_filename, 'r') as platform_exclusions_file:
            platform_exclusions = json.load(platform_exclusions_file)
    else:
        # Use real path in case root is a symlink path
        if os.name == 'posix' and os.path.islink(root):
            root = os.readlink(root)
        # "root" is the root of the folder structure we're validating 
        # "engine_root_path" is the engine root where the restricted platform folder is linked
        relative_folder = os.path.relpath(this_file_path, engine_root_path)
        platform_exclusions_filename = os.path.join(engine_root_path, 'restricted', platform, relative_folder, platform.lower() + '_exclusions.json')
        with open(platform_exclusions_filename, 'r') as platform_exclusions_file:
            platform_exclusions = json.load(platform_exclusions_file)
        
    if platform not in platform_exclusions:
        raise KeyError('No {} found in {}'.format(platform, platform_exclusions_filename))
    if '@lyengine' not in platform_exclusions[platform]:
        raise KeyError('No {}/@lyengine found in {}'.format(platform, package_file_list))
    (_, excludes) = convert_glob_list_to_regex_list(platform_exclusions[platform]['@lyengine'], root)
    del _
    return excludes

def generate_include_exclude_regexes(package_platform, package_type, root, prohibited_platforms):
    # The general contents will be indicated by the package file
    if package_type == 'all':
        package_file_list = os.path.join(this_file_path, 'package_filelists', 'all.json')
    else:
        # Search non-restricted platform first
        package_file_list = os.path.join(this_file_path, 'Platform', package_platform, 'package_filelists', f'{package_type}.json')
        if not os.path.exists(filelist):
            # Use real path in case root is a symlink path
            if os.name == 'posix' and os.path.islink(root):
                root = os.readlink(root)
            # "root" is the root of the folder structure we're validating
            # "engine_root_path" is the engine root where the restricted platform folder is linked
            rel_path = os.path.relpath(this_file_path, engine_root_path)
            package_file_list = os.path.join(engine_root_path, 'restricted', package_platform, rel_path, 'package_filelists',
                                    f'{package_type}.json')
    with open(package_file_list, 'r') as package_file:
        package = json.load(package_file)

    if '@lyengine' not in package:
        raise KeyError('No @lyengine found in {}'.format(package_file_list))

    (includes_list, excludes_list) = convert_glob_list_to_regex_list(package['@lyengine'], root)
    prohibited_platforms.append('all')

    # Add the exclusions of each prohibited platform
    for p in prohibited_platforms:
        excludes_list.extend(generate_excludes_for_platform(root, p))

    includes = re.compile('|'.join(includes_list), re.IGNORECASE)
    excludes = re.compile('|'.join(excludes_list), re.IGNORECASE)
    return (includes, excludes)

def generate_exclude_regexes_for_platform(root, platform):
    return re.compile('|'.join(generate_excludes_for_platform(root, platform)), re.IGNORECASE)
