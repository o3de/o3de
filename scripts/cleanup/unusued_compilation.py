#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import fnmatch
import os
import shutil
import sys
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../build'))
import ci_build

EXTENSIONS_OF_INTEREST = ('.c', '.cc', '.cpp', '.cxx', '.h', '.hpp', '.hxx', '.inl')
EXCLUSIONS = (
    'AZ_DECLARE_MODULE_CLASS(', 
    'REGISTER_QT_CLASS_DESC(',
    'TEST(',
    'TEST_F(',
    'INSTANTIATE_TEST_CASE_P(',
    'INSTANTIATE_TYPED_TEST_CASE_P(',
    'AZ_UNIT_TEST_HOOK(',
    'IMPLEMENT_TEST_EXECUTABLE_MAIN(',
    'DllMain(',
    'CreatePluginInstance('
)
PATH_EXCLUSIONS = (
    '*\\Platform\\Android\\*',
    '*\\Platform\\Common\\*',
    '*\\Platform\\iOS\\*',
    '*\\Platform\\Linux\\*',
    '*\\Platform\\Mac\\*',
    'Templates\\*',
    'python\\*',
    'build\\*',
    'install\\*',
    'Code\\Framework\\AzCore\\AzCore\\Android\\*'
)


def create_filelist(path):
    filelist = set()
    for input_file in path:
        if os.path.isdir(input_file):
            for dp, dn, filenames in os.walk(input_file):
                if 'build\\windows_vs2019' in dp:
                    continue
                for f in filenames:
                    extension = os.path.splitext(f)[1]
                    extension_lower = extension.lower()
                    if extension_lower in EXTENSIONS_OF_INTEREST:
                        normalized_file = os.path.normpath(os.path.join(dp, f))
                        filelist.add(normalized_file)
        else:
            extension = os.path.splitext(input_file)[1]
            extension_lower = extension.lower()
            if extension_lower in EXTENSIONS_OF_INTEREST:
                normalized_file = os.path.normpath(os.path.join(os.getcwd(), input_file))
                filelist.add(normalized_file)
            else:
                print(f'Error, file {input_file} does not have an extension of interest')
                sys.exit(1)
    return filelist

def is_excluded(file):
    for path_exclusion in PATH_EXCLUSIONS:
        if fnmatch.fnmatch(file, path_exclusion):
            return True
    with open(file, 'r') as file:
        contents = file.read()
        for exclusion_term in EXCLUSIONS:
            if exclusion_term in contents:
                return True
    return False

def filter_from_processed(filelist, filter_file_path):
    filelist = [f for f in filelist if not is_excluded(f)]
    if os.path.exists(filter_file_path):  
        with open(filter_file_path, 'r') as filter_file:
            processed_files = [s.strip() for s in filter_file.readlines()]
            filelist -= set(processed_files)
    return filelist

def cleanup_unused_compilation(path):
    # 1. Create a list of all h/cpp files (consider multiple extensions)
    filelist = create_filelist(path)
    # 2. Remove files from "processed" list. This is needed because the process is a bruteforce approach and
    #    can take a while. If something is found in the middle, we want to be able to continue instead of 
    #    starting over. Removing the "unusued_compilation_processed.txt" will start over.
    filter_file_path = os.path.join(os.getcwd(), 'unusued_compilation_processed.txt')
    filelist = filter_from_processed(filelist, filter_file_path)
    sorted_filelist = sorted(filelist, reverse=True)
    # 3. For each file
    total_files = len(sorted_filelist)
    current_files = 1
    for file in sorted_filelist:
        print(f"[{current_files}/{total_files}] Trying {file}")
        #   b. create backup
        shutil.copy(file, file + '.bak')
        #   c. set the file contents as empty
        with open(file, 'w') as source_file:
            source_file.write('')
        #   d. build
        ret = ci_build.build('build_config.json', 'Windows', 'profile_vs2019')
        #     e.1 if build succeeds, leave the file empty (leave backup)
        #     e.2 if build fails, restore backup
        if ret != 0:
            shutil.copy(file + '.bak', file)
            print(f"\t[FAILED] restoring")
        else:
            print(f"\t[SUCCEED]")
        #   f. delete backup
        os.remove(file + '.bak')
        #   add file to processed-list
        with open(filter_file_path, 'a') as filter_file:
            filter_file.write(file)
            filter_file.write('\n')
        current_files += 1

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='This script finds C++ files that do not affect compilation (and therefore can be potentially removed)',
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('file_or_dir', type=str, nargs='+', default=os.getcwd(),
        help='list of files or directories to search within for files to consider')
    args = parser.parse_args()
    ret = cleanup_unused_compilation(args.file_or_dir)
    sys.exit(ret)
