# This python file contains the data used to drive the validator.
"""Data for Validator tool"""
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Anyone seeking to modify this file must follow the process, contact sig-build

# Notice that this is not a JSON file, even though it is almost a valid JSON
# data file.
# The key reason we store this in a python file instead is so that we can
# put explanatory comments int the data, which is not supported by JSON.
# There are other possible approaches (such as removing comments with an re after reading the string)
# but they are more work, so for now we just do it this way.

# In regular expressions in this file, do not use patterns that start with ".*", as they make the entire system very slow.
# Even patterns that start with an single "." open match will slow the system down.
# Patterns should start with a match to a specific character, and match as many specific
# characters as possible before using any wildcard matches.

import os
import logging
import sys
from pathlib import Path

restricted_platforms_for_package = {
    'Mac': [],
    'Windows': [],
    'Provo': ['Provo'],
    'Salem': ['Salem'],
    'Jasper': ['Jasper'],
    'Linux': []
}

restricted_platforms = {}


def find_restricted_platforms():
    this_path = Path(__file__).resolve()
    root_folder = this_path.parents[2]
    relative_path = os.path.relpath(this_path.parent, root_folder)
    restricted_path = os.path.join(root_folder, 'restricted')
    if os.path.exists(restricted_path):
        for dir in [f.path for f in os.scandir(restricted_path) if f.is_dir()]:
            sys.path.append(os.path.join(dir, relative_path))
            try:
                module = __import__('{}_data_LEGAL_REVIEW_REQUIRED'.format(os.path.basename(dir).lower()), locals(), globals())
                module.add_restricted_platform(restricted_platforms)
            except ModuleNotFoundError:
                pass


find_restricted_platforms()

# Order of the exceptions is important. The longer and more specific exceptions should come before the
# short ones to improve the locality of any given exception.
# This is based on the unix style test for binary files.
# It provides a set of characters that occur in text files.
# We assume that file that contain characters outside of this set are binary
# files and skip them.
TEXT_CHARS = bytearray({7, 8, 9, 10, 12, 13, 27} | set(range(0x20, 0x100)) - {0x7f})


def skip_file(filepath):
    _ext = os.path.splitext(filepath)[1].lower()
    """Check if a file is binary"""
    # Don't ever scan these, even thought they sometimes look like text files or are text files
    if _ext in {'.js', '.ma', '.psd', '.fbx', '.obj', '.pem', '.lyr', '.resx', '.pak', '.dat', '.dds', '.ilk', '.ppm', '.html'}:
        return True
    # the dds extension is special, there may be foo.dds, foo.dds.1, foo.dds.2, etc.  In these cases, .dds is not the extension returned by splitext
    if '.dds.' in filepath:
        return True

    # Always scan these, even if someone puts binary characters in them.
    if _ext in {'.cpp', '.c++', '.cxx', '.h', '.hpp', '.hxx', '.inl', '.c', '.cs', '.py', '.mel', '.mm', '.rc', '.ui', '.qrc', '.ts', '.ini', '.cfg', '.cfx',
                '.cfi', '.plist'}:
        return False

    # Otherwise, open the file and see if we think it is binary by looking at the data.
    # This is based on unix style test for binary.
    try:
        with open(filepath, mode='rb') as f:
            data = f.read(1024)
            binary = bool(data.translate(None, TEXT_CHARS))
        return binary
    except:
        logging.error('Unable to load file - check this manually: %s', filepath)
        return True


def get_prohibited_platforms_for_package(package):
    permitted_platforms = restricted_platforms_for_package[package]
    return [p for p in restricted_platforms if p not in permitted_platforms]


def get_bypassed_directories(is_all):
    # Temporarily exempt folders to not fail validation while people is fixing validation errors, they will be removed once the errors are fixed.
    temp_bypass_directories = [
        'commit_validation'
    ]
    bypassed_directories = [
        'python'
    ]
    if not is_all:
        bypassed_directories.extend([
            '.git',
            'restricted',
            'Cache',
            'logs',
            'AssetProcessorTemp',
            'user/log',
            'External'
        ])
        bypassed_directories.extend(temp_bypass_directories)
    return bypassed_directories
