#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

# Enumerations for test sequence types
class SequenceType(Enum):
    REGULAR = 1
    TEST_IMPACT_ANALYSIS = 2

class TestImpact:
    def __init__(self, config_file, dst_commit):
        self.__parse_config_file(config_file)
        self.__init_repo(dst_commit)
        self.__generate_change_list()

    # Parse the configuration file and retrieve the data needed for launching the test impact analysis runtime
    def __parse_config_file(self, config_file):
        print(f"Attempting to parse configuration file '{config_file}'...")
        with open(config_file, "r") as config_data:
            config = json.load(config_data)
            self.__repo_dir = config["repo"]["root"]
            self.__tiaf_bin = config["repo"]["tiaf_bin"]
            if not os.path.isfile(self.__tiaf_bin):
                raise FileNotFoundError("Could not find tiaf binary")
            self.__source_of_truth = config["repo"]["source_of_truth"]
            self.__active_workspace = config["workspace"]["active"]["root"]
            self.__historic_workspace = config["workspace"]["historic"]["root"]
            self.__temp_workspace = config["workspace"]["temp"]["root"]
            last_commit_hash_path_rel = config["workspace"]["historic"]["relative_paths"]["last_run_hash_file"]
            self.__last_commit_hash_path = os.path.join(self.__historic_workspace, last_commit_hash_path_rel)
            print("The configuration file was parsed successfully.")

    # Initializes the internal representation of the repository being targeted for test impact analysis
    def __init_repo(self, dst_commit):
        self.__repo = Repo(self.__repo_dir)
        self.__branch = self.__repo.current_branch
        self.__dst_commit = dst_commit
        self.__src_commit = None
        self.__has_src_commit = False
        if self.__repo.current_branch == self.__source_of_truth:
            self.__is_source_of_truth = True
        else:
            self.__is_source_of_truth = False
        print(f"The repository is located at '{self.__repo_dir}' and the current branch is '{self.__branch}'.")
        print(f"The source of truth branch is '{self.__source_of_truth}'.")
        if self.__is_source_of_truth:
            print("I am the source of truth.")
        else:
            print("I am *not* the source of truth.")

    # Restricts change lists from checking in test impact analysis files
    def __check_for_restricted_files(self, file_path):
        if is_child_path(self.__active_workspace, file_path) or is_child_path(self.__historic_workspace, file_path) or is_child_path(self.__temp_workspace, file_path):
            raise ValueError(f"Checking in test impact analysis framework files is illegal: '{file_path}''.")

    def __read_last_run_hash(self):
        self.__has_src_commit = False
        if os.path.isfile(self.__last_commit_hash_path):
            print(f"Previous commit hash found at 'self.__last_commit_hash_path'")            
            with open(self.__last_commit_hash_path) as file:
                self.__src_commit = file.read()
                self.__has_src_commit = True

    def __write_last_run_hash(self, last_run_hash):
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
            print(f"Generated diff between commits {self.__src_commit} and {self.__dst_commit}.") 
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
    def run(self, sequence_type, safe_mode, test_timeout, global_timeout):
        args = []
        if self.__has_change_list:
            args.append(f"-changelist={self.__change_list_path}")
            if sequence_type == SequenceType.REGULAR:
                print("Sequence type: regular.")
                args.append("--sequence=regular")
            elif sequence_type == SequenceType.TEST_IMPACT_ANALYSIS:
                print("Sequence type: test impact analysis.")
                if self.__is_source_of_truth:
                    # Source of truth branch will allways attempt a seed if no test impact analysis data is available
                    print("This branch is the source of truth, a seed sequence will be run if there is no test impact analysis data.")
                    args.append("--sequence=tiaorseed")
                    args.append("--fpolicy=continue")
                else:
                    # Non source of truth branches will fall back to a regular test run if no test impact analysis data is available
                    print("This branch is not the source of truth, a regular sequence will be run if there is no test impact analysis data.")
                    args.append("--sequence=tia")
                    args.append("--fpolicy=abort")
                
                if safe_mode == True:
                    print("Safe mode is on, the discarded test targets will be run after the selected test targets.")
                    args.append("--safemode=on")
                else:
                    print("Safe mode is off, the discarded test targets will not be run.")
            else:
                raise ValueError(sequence_type)
        else:
            print(f"No change list was generated, this will cause test impact analysis sequences on branches other than the source of truth to fall back to a regular sequence.")
            print("Sequence type: Regular.")
            args.append("--sequence=regular")

        if test_timeout != None:
            args.append(f"--ttimeout={test_timeout}")
            print(f"Test target timeout is set to {test_timeout} seconds.")
        if global_timeout != None:
            args.append(f"--gtimeout={global_timeout}")
            print(f"Global sequence timeout is set to {test_timeout} seconds.")

        print("Args: ", end='')
        print(*args)
        result = subprocess.run([self.__tiaf_bin] + args)
        if result.returncode == 0:
            print("Test impact analysis runtime returned successfully. Updating historical artifacts...")
            self.__write_last_run_hash(self.__dst_commit)
            print("Complete!")
        else:
            print(f"The test impact analysis runtime returned with error: '{result.returncode}'.")
        return result.returncode

