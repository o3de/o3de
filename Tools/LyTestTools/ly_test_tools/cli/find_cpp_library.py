"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Tools for inspecting GitHub CODEOWNERS files
"""

import os

from ly_test_tools.cli.codeowners_hint import get_codeowners


def find_cpp_library_from_test_module_name(folder_to_search, test_module_name):
    """
    This function searches through the CMakeLists.txt in a given folder for what library added the given module.
    :param folder_to_search: the folder location to start the search at, recursively
    :param test_module_name: the name of the test module
    :return: the full file path to the library
    """
    cpp_library_full_path = ""
    print(f"Starting search in {folder_to_search}")
    for root, subdirs, files in os.walk(folder_to_search):
        # print(f"walking {folder_to_search}")
        for filename in files:
            if filename == "CMakeLists.txt":
                #print(f"Found cmake file: {root}")
                file_path = os.path.join(root, filename)
                with open(file_path) as cmake_file:
                    if test_module_name in cmake_file.read():
                        print("Found test module")
                        cpp_library_full_path = file_path

    print(f"Finished search for {test_module_name}")
    return cpp_library_full_path


def get_codeowners_hint_for_aztest_module(folder_to_search, test_module_name):
    cpp_library_full_path = find_cpp_library_from_test_module_name(folder_to_search, test_module_name)
    return get_codeowners(cpp_library_full_path)


if __name__ == "__main__":
    print("Starting")
    result = find_cpp_library_from_test_module_name(os.path.join("C:", os.sep, "Git", "o3de", "Tools"), "LyTestTools_Integ_Sanity")
    print(str(result))
    print("Finished")
