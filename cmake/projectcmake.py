#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import json
import sys
import os
import subprocess
import argparse
import glob

import waffiles2cmake
import gemcmake

def getProjectGemCMakeListsTemplate():
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
# Gem dependencies
################################################################################
ly_add_project_dependencies(
    PROJECT_NAME
        {GEM_NAME}
    TARGETS
        {GEM_NAME}.GameLauncher
    DEPENDENCIES_FILES
        runtime_dependencies.cmake
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_project_dependencies(
        PROJECT_NAME
            {GEM_NAME}
        TARGETS
            AssetBuilder
            AssetProcessor
            AssetProcessorBatch
            Editor
        DEPENDENCIES_FILES
            tool_dependencies.cmake
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

def getEmptyGemDependencyCMakeFormat():
    return """set(GEM_DEPENDENCIES
"""
        
def getGemPaths(gems_list, project_path):
    gem_paths = []
    
    # Get the parent directory of the project
    # If this is not an external project this should be the dev folder for the LY install
    project_parent_path = os.path.abspath(os.path.join(project_path, os.pardir))
    for gem_json in gems_list:
        gem_path = gem_json['Path']
        
        # First check if this Gem is found within the project itself
        project_gem_path = os.path.normcase(os.path.join(project_path, gem_path))
        
        # If so then store the path
        if os.path.exists(project_gem_path):
            gem_paths.append(project_gem_path)
            continue
        
        # Otherwise check if the Gem can be found in the parent directory's Gems folder
        parent_gems_path = os.path.normcase(os.path.join(project_parent_path, gem_path))
        
        if os.path.exists(parent_gems_path):
            gem_paths.append(parent_gems_path)
        else:
            # Could not find the gem in the project or in the LY install
            print(f'Could not find full path for {gem_path}')
            sys.exit(1)
    
    return gem_paths
    
def getGemJson(gem_path):
    # Since the gem.json file can live in subdirectories of the gem root
    # walk the directory tree until we come across it
    gem_json_path = None
    for root, dirs, files in os.walk(gem_path):
        for filename in files:
            if filename == 'gem.json':
                gem_json_path = os.path.join(root, filename)
    
    # load and return the gem.json for this particular gem
    if gem_json_path:
        with open(gem_json_path) as f:
            gem_json = json.load(f)
    
    return gem_json
    
def processGemDependencies(gem_paths):
    toolTime_gems = []
    runTime_gems = []
    for gem_path in gem_paths:
        
        #Acquire the gem.json info for the gem located at this path
        gem_json = getGemJson(gem_path)
        
        if gem_json is None:
            print(f'Could not find gem.json file for gem located at {gem_path}')
            sys.exit(1)
        
        # Filter out asset only gems
        if 'LinkType' in gem_json:
            link_type = gem_json['LinkType']
            if link_type == 'NoCode':
                continue
        
        # Assume there's always a game gem
        has_editor_gem = False
        has_game_gem = True
        
        has_modules = 'Modules' in gem_json
        gem_name = gem_json['Name']
            
        if gem_name is None:
            print(f'gem.json for gem located at {gem_path} does not have a Name field')
            sys.exit(1)
        
        if has_modules:
            # If only an EditorModule is declared there is no GameModule
            has_game_gem = False
            gem_modules = gem_json['Modules']
            for module in gem_modules:
                if 'Type' not in module:
                    continue
                module_type = module['Type']
                if module_type == 'EditorModule':
                    has_editor_gem = True
                elif module_type == 'GameModule':
                    has_game_gem = True
        else:
            if 'EditorModule' in gem_json:
                editor_module = gem_json['EditorModule']
                if editor_module is True:
                    has_editor_gem = True
                    
        
        if not has_editor_gem and not has_game_gem:
            print(f'gem.json for gem located at {gem_path} does not define a Game or Editor Module and is not an asset only gem')
            sys.exit(1)
        
        # If an EditorModule was explicitly declared then we will load the .Editor form
        # Otherwise we will still add the game module to our toolTime list
        if has_editor_gem:
            toolTime_gems.append(f'{gem_name}.Editor')
        else:
            toolTime_gems.append(gem_name);
        
        if has_game_gem:
            runTime_gems.append(gem_name)
        
    return toolTime_gems, runTime_gems
        
def generateCMakeFilesForProjectGemDependencies(toolTime_dependencies, runTime_dependencies, project_code_path):
    toolTime_filePath = os.path.join(project_code_path, 'tool_dependencies.cmake')
    runTime_filePath = os.path.join(project_code_path, 'runtime_dependencies.cmake')
    
    # Generate tool_dependencies.cmake
    with open(toolTime_filePath, 'w') as toolTime_file:
        toolTime_file.write(gemcmake.getCopyright())
        toolTime_file.write(getEmptyGemDependencyCMakeFormat())
        # Add each dependent tooltime gem as a line item into the file
        for toolTime_dependency in toolTime_dependencies:
            toolTime_file.write(f'    Gem::{toolTime_dependency}\n')
        toolTime_file.write(')')
    
    subprocess.run(['p4', 'add', toolTime_filePath])
    
    # Generate runtime dependencies
    with open(runTime_filePath, 'w') as runTime_file:
        runTime_file.write(gemcmake.getCopyright())
        runTime_file.write(getEmptyGemDependencyCMakeFormat())
        # Add each dependent runtime gem as a line item into the file
        for runTime_dependency in runTime_dependencies:
            runTime_file.write(f'    Gem::{runTime_dependency}\n')
        runTime_file.write(')')
    
    subprocess.run(['p4', 'add', runTime_filePath])
    
def main():
    parser = argparse.ArgumentParser(description='This script creates basic CMakeLists.txt and .cmake files for a waf based LY project',
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('path_to_projects', type=str, nargs='+',
        help='list of project directories to create CMakeLists.txt and .cmake files within and add to p4')
        
    args = parser.parse_args()
    
    for input_path in args.path_to_projects:
        if not os.path.isdir(input_path):
            print('Expected a valid path, got {}'.format(input_path))
            sys.exit(1)
        
        project_path = os.path.abspath(input_path)
        project_name = os.path.basename(project_path)
        project_gem_path = os.path.abspath(os.path.join(project_path, 'Gem'))
        project_gem_code_path = os.path.abspath(os.path.join(project_gem_path, 'Code'))
        
        gems_json_file = os.path.join(project_path, 'gems.json')
        project_gem_json_file = os.path.join(project_gem_path, 'gem.json')
        
        with open(gems_json_file) as f:
            gems_json_dict = json.load(f)
        
        if not gems_json_dict:
            print(f'Could not load gems.json for project {project_name}')
            sys.exit(1)
            
        gems_list = gems_json_dict['Gems']
        
        if not gems_list:
            print('Invalid Gems list found in gems.json for project {}').format(project_name)
            sys.exit(1)
            
        with open(project_gem_json_file) as f:
            project_gem_json_dict = json.load(f)
        
        if not project_gem_json_dict:
            print(f'Could not load gem.json within the Gem folder for project {project_name}')
            sys.exit(1)
        
        project_gem_uuid = project_gem_json_dict['Uuid']
        project_gem_version = project_gem_json_dict['Version']
        
        gem_paths = getGemPaths(gems_list, project_path)
        
        toolTime_dependencies, runTime_dependencies = processGemDependencies(gem_paths)
        
        # Create the chain of subdirectory CMakeLists files needed to get to the projects gem code
        gemcmake.addSubdirectoryToCMakeLists(os.path.join(project_path, 'CMakeLists.txt'), 'Gem')
        gemcmake.addSubdirectoryToCMakeLists(os.path.join(project_gem_path, 'CMakeLists.txt'), 'Code')

        # Invoke the gem conversion script and pass in our project CMakeLists template
        gemcmake.generateCMakeFilesForGem(project_gem_path, project_name, project_gem_uuid, project_gem_version, getProjectGemCMakeListsTemplate)
        
        # Generate our tooltime and runtime .cmake files
        generateCMakeFilesForProjectGemDependencies(toolTime_dependencies, runTime_dependencies, project_gem_code_path)
        
#entrypoint
if __name__ == '__main__':
    main()