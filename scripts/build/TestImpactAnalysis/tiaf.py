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
import uuid
from git_utils import Repo
from tiaf_persistent_storage import PersistentStorageNull
from tiaf_persistent_storage_local import PersistentStorageLocal
from tiaf_persistent_storage_s3 import PersistentStorageS3

# Returns True if the specified child path is a child of the specified parent path, otherwise False
def is_child_path(parent_path, child_path):
    parent_path = os.path.abspath(parent_path)
    child_path = os.path.abspath(child_path)
    return os.path.commonpath([os.path.abspath(parent_path)]) == os.path.commonpath([os.path.abspath(parent_path), os.path.abspath(child_path)])

class TestImpact:
    def __init__(self, config_file, src_branch, dst_branch, coverage_update_branches, commit):
        self.__instance_id = uuid.uuid4().hex
        self.__has_change_list = False
        # Config
        self.__parse_config_file(config_file)
        # Branches
        self.__src_branch = src_branch
        self.__dst_branch = dst_branch
        self.__coverage_update_branches = coverage_update_branches
        print(f"Src branch: '{self.__src_branch}'.")
        print(f"Dst branch: '{self.__dst_branch}'.")
        print(f"Coverage update branches: '{self.__coverage_update_branches}'.")
        self.__is_updating_coverage = False
        self.__source_of_truth_branch = None
        if self.__src_branch in self.__coverage_update_branches and self.__dst_branch is None:
            # Branch build on source of coverage update branch
            self.__is_updating_coverage = True
            self.__source_of_truth_branch = self.__src_branch
        elif self.__dst_branch in self.__coverage_update_branches:
            # PR build destined for coverage update branch
            self.__source_of_truth_branch = self.__dst_branch
        print(f"Is updating coverage: '{self.__is_updating_coverage}'.")
        print(f"Source of truth branch: '{self.__source_of_truth_branch}'.")
        # Commit
        self.__dst_commit = commit
        print(f"Commit: '{self.__dst_commit}'.")
        self.__src_commit = None
        self.__commit_distance = None

    # Parse the configuration file and retrieve the data needed for launching the test impact analysis runtime
    def __parse_config_file(self, config_file):
        print(f"Attempting to parse configuration file '{config_file}'...")
        with open(config_file, "r") as config_data:
            self.__config = json.load(config_data)
            self.__repo_dir = self.__config["repo"]["root"]
            self.__repo = Repo(self.__repo_dir)
            # TIAF
            self.__use_test_impact_analysis = self.__config["jenkins"]["use_test_impact_analysis"]
            self.__tiaf_bin = self.__config["repo"]["tiaf_bin"]
            if self.__use_test_impact_analysis and not os.path.isfile(self.__tiaf_bin):
                raise FileNotFoundError("Could not find tiaf binary")
            # Workspaces
            self.__active_workspace = self.__config["workspace"]["active"]["root"]
            self.__historic_workspace = self.__config["workspace"]["historic"]["root"]
            self.__temp_workspace = self.__config["workspace"]["temp"]["root"]
            print("The configuration file was parsed successfully.")

    # Restricts change lists from checking in test impact analysis files
    def __check_for_restricted_files(self, file_path):
        if is_child_path(self.__active_workspace, file_path) or is_child_path(self.__historic_workspace, file_path) or is_child_path(self.__temp_workspace, file_path):
            raise ValueError(f"Checking in test impact analysis framework files is illegal: '{file_path}''.")

    # Determines the change list bewteen now and the last tiaf run (if any)
    def __attempt_to_generate_change_list(self):
        self.__has_change_list = False
        self.__change_list_path = None
        # Check whether or not a previous commit hash exists (no hash is not a failure)
        self.__src_commit = self.__persistent_storage.last_commit_hash
        if self.__src_commit is not None:
            if self.__repo.is_descendent(self.__src_commit, self.__dst_commit) == False:
                print(f"Source commit '{self.__src_commit}' and destination commit '{self.__dst_commit}' are not related.")
                return
            self.__commit_distance = self.__repo.commit_distance(self.__src_commit, self.__dst_commit)
            diff_path = os.path.join(self.__temp_workspace, f"changelist.{self.__instance_id}.diff")
            try:
                git_utils.create_diff_file(self.__src_commit, self.__dst_commit, diff_path)
            except FileNotFoundError as e:
                print(e)
                return
            # A diff was generated, attempt to parse the diff and construct the change list
            print(f"Generated diff between commits '{self.__src_commit}' and '{self.__dst_commit}': '{diff_path}'.") 
            with open(diff_path, "r") as diff_data:
                lines = diff_data.readlines()
                for line in lines:
                    match = re.split("^R[0-9]+\\s(\\S+)\\s(\\S+)", line)
                    if len(match) > 1:
                        # File rename
                        self.__check_for_restricted_files(match[1])
                        self.__check_for_restricted_files(match[2])
                        # Treat renames as a deletion and an addition
                        self.__change_list.deletedFiles.append(match[1])
                        self.__change_list.createdFiles.append(match[2])
                    else:
                        match = re.split("^[AMD]\\s(\\S+)", line)
                        self.__check_for_restricted_files(match[1])
                        if len(match) > 1:
                            if line[0] == 'A':
                                # File addition
                                self.__change_list.createdFiles.append(match[1])
                            elif line[0] == 'M':
                                # File modification
                                self.__change_list.updatedFiles.append(match[1])
                            elif line[0] == 'D':
                                # File Deletion
                                self.__change_list.deletedFiles.append(match[1])
            # Serialize the change list to the JSON format the test impact analysis runtime expects
            change_list_json = json.dumps(self.__change_list, indent = 4)
            change_list_path = os.path.join(self.__temp_workspace, f"changelist.{self.__instance_id}.json")
            f = open(change_list_path, "w")
            f.write(change_list_json)
            f.close()
            print(f"Change list constructed successfully: '{change_list_path}'.")
            print(f"{len(self.__change_list['createdFiles'])} created files, {len(self.__change_list['updatedFiles'])} updated files and {len(self.__change_list['deletedFiles'])} deleted files.")
            # Note: an empty change list generated due to no changes between last and current commit is valid
            self.__has_change_list = True
            self.__change_list_path = change_list_path
        else:
            print("No previous commit hash found, regular or seeded sequences only will be run.")
            self.__has_change_list = False
            return

    @property
    def src_commit(self):
        return self.__src_commit

    @property
    def dst_commit(self):
        return self.__dst_commit

    @property
    def commit_distance(self):
        return self.__commit_distance

    @property
    def branch(self):
        return self.__branch

    @property
    def pipeline(self):
        return self.__pipeline

    @property
    def seeding_branches(self):
        return self._seeding_branches

    @property
    def seeding_pipelines(self):
        return self.__seeding_pipelines

    @property
    def is_seeding_branch(self):
        return self.__is_seeding_branch

    @property
    def is_seeding_pipeline(self):
        return self.__is_seeding_pipeline

    @property
    def use_test_impact_analysis(self):
        return self.__use_test_impact_analysis

    @property
    def has_change_list(self):
        return self.__has_change_list

    def __generate_result(self, s3_bucket, suite, return_code, report, runtime_args):
        result = {}
        result["src_commit"] = self.__src_commit
        result["dst_commit"] = self.__dst_commit
        result["commit_distance"] = self.__commit_distance
        result["src_branch"] = self.__src_branch
        result["dst_branch"] = self.__dst_branch
        result["suite"] = suite
        result["coverage_update_branches"] = self.__coverage_update_branches
        result["is_coverage_update_branch"] = self.__is_updating_coverage
        result["use_test_impact_analysis"] = self.__use_test_impact_analysis
        result["has_change_list"] = self.__has_change_list
        result["has_historic_data"] = self.__persistent_storage.has_historic_data
        result["s3_bucket"] = s3_bucket
        result["runtime_args"] = runtime_args
        result["return_code"] = return_code
        result["report"] = report
        result["change_list"] = self.__change_list
        return result

    # Runs the specified test sequence
    def run(self, s3_bucket, suite, test_failure_policy, safe_mode, test_timeout, global_timeout):
        args = []
        self.__change_list = {}
        self.__change_list["createdFiles"] = []
        self.__change_list["updatedFiles"] = []
        self.__change_list["deletedFiles"] = []
        if self.__use_test_impact_analysis and self.__source_of_truth_branch is not None:
            print("Test impact analysis is enabled.")
            # Persistent storage location
            if s3_bucket is not None:
                self.__persistent_storage = PersistentStorageS3(self.__config, suite, s3_bucket, self.__source_of_truth_branch)
            else:
                self.__persistent_storage = PersistentStorageLocal(self.__config, suite)
            if self.__persistent_storage.has_historic_data:
                print("Historic data found.")
                self.__attempt_to_generate_change_list()
            else:
                print("No historic data found.")
            # Sequence type
            if self.__has_change_list:
                if self.__is_updating_coverage:
                    # Use TIA sequence (instrumented subset of tests) for coverage updating branches so we can update the coverage data with the generated coverage
                    sequence_type = "tia"
                else:
                    # Use TIA no-write sequence (regular subset of tests) for non coverage updating branche
                    sequence_type = "tianowrite"
                    # Ignore integrity failures for non coverage updating branches as our confidence in the
                    args.append("--ipolicy=continue")
                    print("Integration failure policy is set to 'continue'.")
                # Safe mode
                if safe_mode:
                    args.append("--safemode=on")
                    print("Safe mode set to 'on'.")
                else:
                    args.append("--safemode=off")
                    print("Safe mode set to 'off'.")
                # Change list
                args.append(f"--changelist={self.__change_list_path}")
                print(f"Change list is set to '{self.__change_list_path}'.")
            else:
                if self.__is_updating_coverage:
                    # Use seed sequence (instrumented all tests) for coverage updating branches so we can generate the coverage bed for future sequences
                    sequence_type = "seed"
                    # We always continue after test failures when seeding to ensure we capture the coverage for all test targets
                    test_failure_policy = "continue"
                else:
                    # Use regular sequence (regular all tests) for non coverage updating branches as we have no coverage to use nor coverage to update
                    sequence_type = "regular"
        else:
            self.__persistent_storage = PersistentStorageNull(self.__config, suite)
            # Use regular sequence (regular all tests) when test impact analysis is disabled
            sequence_type = "regular"
        args.append(f"--sequence={sequence_type}")
        print(f"Sequence type is set to '{sequence_type}'.")
         # Test failure policy
        args.append(f"--fpolicy={test_failure_policy}")
        print(f"Test failure policy is set to '{test_failure_policy}'.")
        # Sequence report
        report_file = os.path.join(self.__temp_workspace, f"report.{self.__instance_id}.json")
        args.append(f"--report={report_file}")
        print(f"Sequence report file is set to '{report_file}'.")
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
        # Run sequence
        print("Args: ", end='')
        print(*args)
        runtime_result = subprocess.run([self.__tiaf_bin] + args)
        report = None
        # If the sequence completed (with or without failures) we will update the historical meta-data
        if runtime_result.returncode == 0 or runtime_result.returncode == 7:
            print("Test impact analysis runtime returned successfully.")
            if self.__use_test_impact_analysis and self.__is_updating_coverage:
                print("Writing historical meta-data...")
                self.__persistent_storage.update_historic_data(self.__dst_commit)
            print("Exporting reports...")
            with open(report_file) as json_file:
                report = json.load(json_file)
            print("Complete!")
        else:
            print(f"The test impact analysis runtime returned with error: '{runtime_result.returncode}'.")
        
        result = self.__generate_result(s3_bucket, suite, runtime_result.returncode, report, args)
        return result
        