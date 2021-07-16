#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from __future__ import (absolute_import, division,
                        print_function, unicode_literals)

import json
import sys
import json
import os
import subprocess
import argparse

import waffiles2cmake
fileContents = ""

def getCopyright():
    return """#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

"""

def getGemCMakeListsTemplate():
    return """ly_add_target(
    NAME {GEM_NAME}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        {GEM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            #AZ::AzCore
)

ly_add_target(
    NAME {GEM_NAME} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        {GEM_NAME_LOWERCASE}_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::{GEM_NAME}.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME {GEM_NAME}.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            {GEM_NAME_LOWERCASE}_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                #AZ::AzCore
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME {GEM_NAME}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            {GEM_NAME_LOWERCASE}_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::{GEM_NAME}.Static
    )
    ly_add_googletest(
        NAME {GEM_NAME}.Tests
    )
endif()
"""

def getEmptyCMakeFiles():
    return """set(FILES
)
""" 

def getDefaultTargetsForGem(gem_name, gem_uuid, gem_version, cmakeListTemplate):
    gem_name_lowercase = gem_name.lower()
    gem_uuid_lowercase = gem_uuid.lower()
    return cmakeListTemplate()\
        .replace('{GEM_NAME}', gem_name)\
        .replace('{GEM_NAME_LOWERCASE}', gem_name_lowercase)\
        .replace('{GEM_UUID}', gem_uuid_lowercase)\
        .replace('{GEM_VERSION}', gem_version)

def createEmptyCMakeLists(cmakelists_path):
    with open(cmakelists_path, 'w') as destination_file:
        destination_file.write(getCopyright())

def createGemCMakeLists(cmakelists_path, gem_name, gem_uuid, gem_version, cmakeListTemplate):
    with open(cmakelists_path, 'w') as destination_file:
        destination_file.write(getCopyright())
        destination_file.write(getDefaultTargetsForGem(gem_name, gem_uuid, gem_version, cmakeListTemplate))

def addSubdirectoryToCMakeLists(cmakelists_path, folder_name):
    if os.path.exists(cmakelists_path):
        print('Editing file {}'.format(cmakelists_path))
        subprocess.run(['p4', 'edit', cmakelists_path])
    else:
        print('Adding file {}'.format(cmakelists_path))
        createEmptyCMakeLists(cmakelists_path)
        subprocess.run(['p4', 'add', cmakelists_path])

    # Edit the file
    with open(cmakelists_path, 'r') as source_file:
        fileContents = source_file.read()

    lineToAdd = 'add_subdirectory({})\n'.format(folder_name)
    if fileContents.find(lineToAdd) == -1:
        fileContents += lineToAdd
        with open(cmakelists_path, 'w') as destination_file:
            destination_file.write(fileContents)

def generateCMakeFilesForGem(gem_path, gem_name, gem_uuid, gem_version, cmakeListTemplate):
    gem_code_path = os.path.join(gem_path, 'Code', 'CMakeLists.txt')
    print('Adding file {}'.format(gem_code_path))
    createGemCMakeLists(gem_code_path, gem_name, gem_uuid, gem_version, cmakeListTemplate)
    subprocess.run(['p4', 'add', gem_code_path])

    gem_shared_filename = gem_name.lower() + '_shared_files.cmake'
    gem_shared_files = os.path.join(gem_path, 'Code', gem_shared_filename)
    with open(gem_shared_files, 'w') as destination_file:
        destination_file.write(getCopyright())
        destination_file.write(getEmptyCMakeFiles())
    subprocess.run(['p4', 'add', gem_shared_files])
    
    waffiles2cmake.main()
    
def main():
    """script main function"""
    parser = argparse.ArgumentParser(description='This script creates a basic CMakeLists.txt file for a gem using the gem.json',
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('path_to_gems', type=str, nargs='+',
        help='list of gem directories to look create CMakeLists.txt files within and add to p4')

    args = parser.parse_args()

    for input_path in args.path_to_gems:
        if not os.path.isdir(input_path):
            print('Expected a valid path, got {}'.format(input_path))
            sys.exit(1)

        gem_path = os.path.abspath(input_path)
        gem_name = os.path.basename(gem_path)
        gems_path = os.path.dirname(gem_path)

        # Get the UUID
        gem_json_file = os.path.join(gem_path, 'gem.json')
        with open(gem_json_file) as f:
            gem_json_dict = json.load(f)
        gem_uuid = gem_json_dict['Uuid']
        gem_version = gem_json_dict['Version']

        addSubdirectoryToCMakeLists(os.path.join(gems_path, 'CMakeLists.txt'), gem_name)
        addSubdirectoryToCMakeLists(os.path.join(gem_path, 'CMakeLists.txt'), 'Code')

        generateCMakeFilesForGem(gem_path, gem_name, gem_uuid, gem_version, getGemCMakeListsTemplate)

#entrypoint
if __name__ == '__main__':
    main()