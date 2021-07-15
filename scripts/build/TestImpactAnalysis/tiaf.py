#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import json
import subprocess
import re
import git_utils
from git_utils import Repo
from enum import Enum

# Returns True if the specified child path is a child of the specified parent path, otherwise False
def is_child_path(parent_path, child_path):
    parent_path = os.path.abspath(parent_path)
    child_path = os.path.abspath(child_path)
    return os.path.commonpath([os.path.abspath(parent_path)]) == os.path.commonpath([os.path.abspath(parent_path), os.path.abspath(child_path)])

class TestImpact:
    def __init__(self, config_file, dst_commit, src_branch, dst_branch, pipeline, seeding_branches, seeding_pipelines):
        # Commit
        self.__dst_commit = dst_commit
        print(f"Commit: '{self.__dst_commit}'.")
        self.__src_commit = None
        self.__has_src_commit = False
        # Branch
        self.__src_branch = src_branch
        print(f"Source branch: '{self.__src_branch}'.")
        self.__dst_branch = dst_branch
        print(f"Destination branch: '{self.__dst_branch}'.")
        print(f"Seeding branches: '{seeding_branches}'.")
        if self.__src_branch in seeding_branches:
            self.__is_seeding_branch = True
        else:
            self.__is_seeding_branch = False
        print(f"Is seeding branch: '{self.__is_seeding_branch}'.")
        # Pipeline
        self.__pipeline = pipeline
        print(f"Pipeline: '{self.__pipeline}'.")
        print(f"Seeding pipelines: '{seeding_pipelines}'.")
        if self.__pipeline in seeding_pipelines:
            self.__is_seeding_pipeline = True
        else:
            self.__is_seeding_pipeline = False
        print(f"Is seeding pipeline: '{self.__is_seeding_pipeline}'.")
        # Config
        self.__parse_config_file(config_file)
        # Sequence
        if self.__is_seeding_branch and self.__is_seeding_pipeline:
            self.__is_seeding = True
        else:
            self.__is_seeding = False
        print(f"Is seeding: '{self.__is_seeding}'.")
        if self.__use_test_impact_analysis and not self.__is_seeding:
            self.__generate_change_list()

    # Parse the configuration file and retrieve the data needed for launching the test impact analysis runtime
    def __parse_config_file(self, config_file):
        print(f"Attempting to parse configuration file '{config_file}'...")
        with open(config_file, "r") as config_data:
            config = json.load(config_data)
            self.__repo_dir = config["repo"]["root"]
            self.__repo = Repo(self.__repo_dir)
            # TIAF
            self.__use_test_impact_analysis = config["jenkins"]["use_test_impact_analysis"]
            print(f"Is using test impact analysis: '{self.__use_test_impact_analysis}'.")
            self.__tiaf_bin = config["repo"]["tiaf_bin"]
            if self.__use_test_impact_analysis and not os.path.isfile(self.__tiaf_bin):
                raise FileNotFoundError("Could not find tiaf binary")
            # Workspaces
            self.__active_workspace = config["workspace"]["active"]["root"]
            self.__historic_workspace = config["workspace"]["historic"]["root"]
            self.__temp_workspace = config["workspace"]["temp"]["root"]
            # Last commit hash
            last_commit_hash_path_file = config["workspace"]["historic"]["relative_paths"]["last_run_hash_file"]
            self.__last_commit_hash_path = os.path.join(self.__historic_workspace, last_commit_hash_path_file)
            print("The configuration file was parsed successfully.")

    # Restricts change lists from checking in test impact analysis files
    def __check_for_restricted_files(self, file_path):
        if is_child_path(self.__active_workspace, file_path) or is_child_path(self.__historic_workspace, file_path) or is_child_path(self.__temp_workspace, file_path):
            raise ValueError(f"Checking in test impact analysis framework files is illegal: '{file_path}''.")

    def __read_last_run_hash(self):
        self.__has_src_commit = False
        if os.path.isfile(self.__last_commit_hash_path):
            print(f"Previous commit hash found at '{self.__last_commit_hash_path}'.")
            with open(self.__last_commit_hash_path) as file:
                self.__src_commit = file.read()
                self.__has_src_commit = True

    def __write_last_run_hash(self, last_run_hash):
        os.makedirs(self.__historic_workspace, exist_ok=True)
        f = open(self.__last_commit_hash_path, "w")
        f.write(last_run_hash)
        f.close()

    # Determines the change list bewteen now and the last tiaf run (if any)
    def __generate_change_list(self):
        self.__has_change_list = False
        self.__change_list_path = None
        # Check whether or not a previous commit hash exists (no hash is not a failure)
        self.__read_last_run_hash()
        if self.__has_src_commit == True:
            if git_utils.is_descendent(self.__src_commit, self.__dst_commit) == False:
                print(f"Source commit '{self.__src_commit}' and destination commit '{self.__dst_commit}' are not related.")
                return
            diff_path = os.path.join(self.__temp_workspace, "changelist.diff")
            try:
                git_utils.create_diff_file(self.__src_commit, self.__dst_commit, diff_path)
            except FileNotFoundError as e:
                print(e)
                return
            # A diff was generated, attempt to parse the diff and construct the change list
            print(f"Generated diff between commits '{self.__src_commit}' and '{self.__dst_commit}': '{diff_path}'.") 
            change_list = {}
            change_list["createdFiles"] = []
            change_list["updatedFiles"] = []
            change_list["deletedFiles"] = []
            with open(diff_path, "r") as diff_data:
                lines = diff_data.readlines()
                for line in lines:
                    match = re.split("^R[0-9]+\\s(\\S+)\\s(\\S+)", line)
                    if len(match) > 1:
                        # File rename
                        self.__check_for_restricted_files(match[1])
                        self.__check_for_restricted_files(match[2])
                        # Treat renames as a deletion and an addition
                        change_list["deletedFiles"].append(match[1])
                        change_list["createdFiles"].append(match[2])
                    else:
                        match = re.split("^[AMD]\\s(\\S+)", line)
                        self.__check_for_restricted_files(match[1])
                        if len(match) > 1:
                            if line[0] == 'A':
                                # File addition
                                change_list["createdFiles"].append(match[1])
                            elif line[0] == 'M':
                                # File modification
                                change_list["updatedFiles"].append(match[1])
                            elif line[0] == 'D':
                                # File Deletion
                                change_list["deletedFiles"].append(match[1])
            # Serialize the change list to the JSON format the test impact analysis runtime expects
            change_list_json = json.dumps(change_list, indent = 4)
            change_list_path = os.path.join(self.__temp_workspace, "changelist.json")
            f = open(change_list_path, "w")
            f.write(change_list_json)
            f.close()
            print(f"Change list constructed successfully: '{change_list_path}'.")
            print(f"{len(change_list['createdFiles'])} created files, {len(change_list['updatedFiles'])} updated files and {len(change_list['deletedFiles'])} deleted files.")
            # Note: an empty change list generated due to no changes between last and current commit is valid
            self.__has_change_list = True
            self.__change_list_path = change_list_path
        else:
            print("No previous commit hash found, regular or seeded sequences only will be run.")
            self.__has_change_list = False
            return

    # Runs the specified test sequence
    def run(self, suite, test_failure_policy, safe_mode, test_timeout, global_timeout):
        args = []
        seed_sequence_test_failure_policy = "continue"
        # Suite
        args.append(f"--suite={suite}")
        print(f"Test suite is set to '{suite}'.")
        # Timeouts
        if test_timeout != None:
            args.append(f"--ttimeout={test_timeout}")
            print(f"Test target timeout is set to {test_timeout} seconds.")
        if global_timeout != None:
            args.append(f"--gtimeout={global_timeout}")
            print(f"Global sequence timeout is set to {test_timeout} seconds.")
        if self.__use_test_impact_analysis:
            print("Test impact analysis is enabled.")
            # Seed sequences
            if self.__is_seeding:
                # Sequence type
                args.append("--sequence=seed")
                print("Sequence type is set to 'seed'.")
                # Test failure policy
                args.append(f"--fpolicy={seed_sequence_test_failure_policy}")
                print(f"Test failure policy is set to '{seed_sequence_test_failure_policy}'.")
            # Impact analysis sequences
            else:
                if self.__has_change_list:
                    # Change list
                    args.append(f"--changelist={self.__change_list_path}")
                    print(f"Change list is set to '{self.__change_list_path}'.")
                    # Sequence type
                    args.append("--sequence=tianowrite")
                    print("Sequence type is set to 'tianowrite'.")
                    # Integrity failure policy
                    args.append("--ipolicy=continue")
                    print("Integration failure policy is set to 'continue'.")
                    # Safe mode
                    if safe_mode:
                        args.append("--safemode=on")
                        print("Safe mode set to 'on'.")
                    else:
                        args.append("--safemode=off")
                        print("Safe mode set to 'off'.")
                else:
                    args.append("--sequence=regular")
                    print("Sequence type is set to 'regular'.")
                # Test failure policy
                args.append(f"--fpolicy={test_failure_policy}")
                print(f"Test failure policy is set to '{test_failure_policy}'.")
        else:
            print("Test impact analysis is disabled.")
             # Sequence type
            args.append("--sequence=regular")
            print("Sequence type is set to 'regular'.")
            # Seeding job
            if self.__is_seeding:
                # Test failure policy
                args.append(f"--fpolicy={seed_sequence_test_failure_policy}")
                print(f"Test failure policy is set to '{seed_sequence_test_failure_policy}'.")
            # Non seeding job
            else:
                # Test failure policy
                args.append(f"--fpolicy={test_failure_policy}")
                print(f"Test failure policy is set to '{test_failure_policy}'.")
        
        print("Args: ", end='')
        print(*args)
        result = subprocess.run([self.__tiaf_bin] + args)
        # If the sequence completed (with or without failures) we will update the historical meta-data
        if result.returncode == 0 or result.returncode == 7:
            print("Test impact analysis runtime returned successfully.")
            if self.__is_seeding:
                print("Writing historical meta-data...")
                self.__write_last_run_hash(self.__dst_commit)
            print("Complete!")
        else:
            print(f"The test impact analysis runtime returned with error: '{result.returncode}'.")
        return result.returncode
        