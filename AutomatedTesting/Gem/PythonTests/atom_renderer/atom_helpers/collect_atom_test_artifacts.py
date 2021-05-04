"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Functions used for collecting various Atom test artifacts (logs, screenshots, etc.).
"""

import os

VALID_ARTIFACT_FILE_TYPES = ['.csv', '.dmp', '.json', '.log', '.xml', '.ppm', '.png']


def _find_artifact_files(directory):
    # type: (str) -> list
    """
    Returns a fully built list of artifact files paths.
    :param directory: a path to search with os.walk() for all sub-directories and files.
    :return: list of artifact file paths.
    """
    directories_with_artifacts = []

    for artifact_directory in os.walk(directory):
        # example output for artifact_directory:
        # ('C:\path\to\artifact_directory\', ['Subfolder1', 'Subfolder2'], ['file1', 'file2'])
        artifact_file_path = artifact_directory[0]
        artifact_file_path_subfolders = artifact_directory[1]
        artifact_files = artifact_directory[2]
        if artifact_files and 'JobLogs' or 'LogBackups' not in artifact_file_path_subfolders:
            artifact_file_paths = _build_artifact_file_paths(artifact_file_path, artifact_files)
            directories_with_artifacts.extend(artifact_file_paths)

    return directories_with_artifacts


def _build_artifact_file_paths(path_to_artifact_files, artifact_files):
    """
    Given a path to artifact file(s) and a list of artifact files inside that path,
    build & return a list of paths leading to each artifact file.
    :param path_to_artifact_files: path to the location storing artifact files.
    :param artifact_files: list of artifact files that are inside the path_to_artifact_files.
    :return: list of fully parsed artifact file paths.
    """
    parsed_artifact_file_paths = []

    for artifact_file in artifact_files:
        file_type = os.path.splitext(artifact_file)[1]
        if file_type in VALID_ARTIFACT_FILE_TYPES:
            artifact_file_path = os.path.join(path_to_artifact_files, artifact_file)
            parsed_artifact_file_paths.append(artifact_file_path)

    return parsed_artifact_file_paths


def get_atom_test_artifacts(engine_root):
    """
    Search for & return a list of test artifact file paths for Atom renderer test artifacts.
    :param engine_root: path to the engine root.
    :return: list of test artifact file paths.
    """
    atom_test_artifacts = [
        os.path.join(engine_root, 'AutomatedTesting', "user", "log"),
    ]

    artifact_file_paths = []
    for artifact_file_path in atom_test_artifacts:
        if os.path.exists(artifact_file_path):
            artifact_file_paths.extend(_find_artifact_files(artifact_file_path))

    return artifact_file_paths
