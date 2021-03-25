"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

String and search related functions.
"""
import os
import re


def in_file(file_path, pattern_string):
    """
    This method checks if pattern_string exists in any line in the file specified in file_path.
    It cannot match multi-line patterns.
    :param file_path: files path for the file to search in
    :param pattern_string: string to search for.
    :return: True if the String is found, False otherwise.
    """
    if not os.path.exists(file_path):
        raise RuntimeError("File does not exist at {}".format(file_path))
    if not pattern_string:
        raise RuntimeError("Must provide string to search for")

    with open(file_path, "r") as game_log:
        for line in game_log.readlines():
            if pattern_string in line:
                return True
    return False


def regex_in_file(file_path, pattern_string):
    """
    This method uses regex to check if pattern_string exists in the file specified in file_path.
    It can match multi-line patterns but is a lot heavier as it parses whole file as string.
    :param file_path: files path for the file to search in
    :param pattern_string: regex-pattern to search for.
    :return: True if the regex-pattern is found found in file, False otherwise.
    """
    if not os.path.exists(file_path):
        raise RuntimeError("File does not exist at {}".format(file_path))
    if not pattern_string:
        raise RuntimeError("Must provide string to search for")

    re.compile(pattern_string)
    with open(file_path, "r") as game_log:
        return re.search(pattern_string, game_log.read())


def regex_in_lines_in_file(file_path, pattern_string):
    """
    This method uses regex to check if pattern_string exists in any line in the file specified in file_path.
    It cannot match multi-line patterns, but will often more performant than regex_in_file.
    :param file_path: files path for the file to search in
    :param pattern_string: regex-pattern to search for.
    :return: True if the regex-pattern is found found in file, False otherwise.
    """
    if not os.path.exists(file_path):
        raise RuntimeError("File does not exist at {}".format(file_path))
    if not pattern_string:
        raise RuntimeError("Must provide string to search for")

    re.compile(pattern_string)
    with open(file_path, "r") as game_log:
        for line in game_log.readlines():
            if re.search(pattern_string, line):
                return True
    return False
