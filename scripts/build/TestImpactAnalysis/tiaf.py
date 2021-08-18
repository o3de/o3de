#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import json
import subprocess
import re
import uuid
import pathlib
from git_utils import Repo
from tiaf_persistent_storage_local import PersistentStorageLocal
from tiaf_persistent_storage_s3 import PersistentStorageS3
from tiaf_logger import get_logger

logger = get_logger(__file__)

class TestImpact:
    def __init__(self, config_file: str):
        """
        Initializes the test impact model with the commit, branches as runtime configuration.

        @param config_file: The runtime config file to obtain the runtime configuration data from.
        """

        self._has_change_list = False
        self._parse_config_file(config_file)

    def _parse_config_file(self, config_file: str):
        """
        Parse the configuration file and retrieve the data needed for launching the test impact analysis runtime.

        @param config_file: The runtime config file to obtain the runtime configuration data from.
        """

        logger.info(f"Attempting to parse configuration file '{config_file}'...")
        try:
            with open(config_file, "r") as config_data:
                self._config = json.load(config_data)
                self._repo_dir = self._config["repo"]["root"]
                self._repo = Repo(self._repo_dir)

                # TIAF
                self._use_test_impact_analysis = self._config["jenkins"]["use_test_impact_analysis"]
                self._tiaf_bin = pathlib.Path(self._config["repo"]["tiaf_bin"])
                if self._use_test_impact_analysis and not self._tiaf_bin.is_file():
                    logger.warning(f"Could not find TIAF binary at location {self._tiaf_bin}, TIAF will be turned off.")
                    self._use_test_impact_analysis = False
                else:
                    logger.info(f"Runtime binary found at location '{self._tiaf_bin}'")

                # Workspaces
                self._active_workspace = self._config["workspace"]["active"]["root"]
                self._historic_workspace = self._config["workspace"]["historic"]["root"]
                self._temp_workspace = self._config["workspace"]["temp"]["root"]
                logger.info("The configuration file was parsed successfully.")
        except KeyError as e:
            logger.error(f"The config does not contain the key {str(e)}.")
            return

    def _attempt_to_generate_change_list(self):
        """
        Attempts to determine the change list bewteen now and the last tiaf run (if any).
        """

        self._has_change_list = False
        self._change_list_path = None

        # Check whether or not a previous commit hash exists (no hash is not a failure)
        if self._src_commit:
            if self._is_source_of_truth_branch:
                # For branch builds, the dst commit must be descended from the src commit
                if not self._repo.is_descendent(self._src_commit, self._dst_commit):
                    logger.error(f"Source commit '{self._src_commit}' and destination commit '{self._dst_commit}' must be related for branch builds.")
                    return

                # Calculate the distance (in commits) between the src and dst commits
                self._commit_distance = self._repo.commit_distance(self._src_commit, self._dst_commit)
                logger.info(f"The distance between '{self._src_commit}' and '{self._dst_commit}' commits is '{self._commit_distance}' commits.")
                multi_branch = False
            else:
                # For pull request builds, the src and dst commits are on different branches so we need to ensure a common ancestor is used for the diff
                multi_branch = True

            try:
                # Attempt to generate a diff between the src and dst commits
                logger.info(f"Source '{self._src_commit}' and destination '{self._dst_commit}' will be diff'd.")
                diff_path = pathlib.Path(pathlib.PurePath(self._temp_workspace).joinpath(f"changelist.{self._instance_id}.diff"))
                self._repo.create_diff_file(self._src_commit, self._dst_commit, diff_path, multi_branch)
            except RuntimeError as e:
                logger.error(e)
                return

            # A diff was generated, attempt to parse the diff and construct the change list
            logger.info(f"Generated diff between commits '{self._src_commit}' and '{self._dst_commit}': '{diff_path}'.") 
            with open(diff_path, "r") as diff_data:
                lines = diff_data.readlines()
                for line in lines:
                    match = re.split("^R[0-9]+\\s(\\S+)\\s(\\S+)", line)
                    if len(match) > 1:
                        # File rename
                        # Treat renames as a deletion and an addition
                        self._change_list["deletedFiles"].append(match[1])
                        self._change_list["createdFiles"].append(match[2])
                    else:
                        match = re.split("^[AMD]\\s(\\S+)", line)
                        if len(match) > 1:
                            if line[0] == 'A':
                                # File addition
                                self._change_list["createdFiles"].append(match[1])
                            elif line[0] == 'M':
                                # File modification
                                self._change_list["updatedFiles"].append(match[1])
                            elif line[0] == 'D':
                                # File Deletion
                                self._change_list["deletedFiles"].append(match[1])

            # Serialize the change list to the JSON format the test impact analysis runtime expects
            change_list_json = json.dumps(self._change_list, indent = 4)
            change_list_path = pathlib.PurePath(self._temp_workspace).joinpath(f"changelist.{self._instance_id}.json")
            f = open(change_list_path, "w")
            f.write(change_list_json)
            f.close()
            logger.info(f"Change list constructed successfully: '{change_list_path}'.")
            logger.info(f"{len(self._change_list['createdFiles'])} created files, {len(self._change_list['updatedFiles'])} updated files and {len(self._change_list['deletedFiles'])} deleted files.")
            
            # Note: an empty change list generated due to no changes between last and current commit is valid
            self._has_change_list = True
            self._change_list_path = change_list_path
        else:
            logger.error("No previous commit hash found, regular or seeded sequences only will be run.")
            self._has_change_list = False
            return

    def _generate_result(self, s3_bucket: str, suite: str, return_code: int, report: dict, runtime_args: list):
        """
        Generates the result object from the pertinent runtime meta-data and sequence report.

        @param The generated result object.
        """

        result = {}
        result["src_commit"] = self._src_commit
        result["dst_commit"] = self._dst_commit
        result["commit_distance"] = self._commit_distance
        result["src_branch"] = self._src_branch
        result["dst_branch"] = self._dst_branch
        result["suite"] = suite
        result["use_test_impact_analysis"] = self._use_test_impact_analysis
        result["source_of_truth_branch"] = self._source_of_truth_branch
        result["is_source_of_truth_branch"] = self._is_source_of_truth_branch
        result["has_change_list"] = self._has_change_list
        result["has_historic_data"] = self._has_historic_data
        result["s3_bucket"] = s3_bucket
        result["runtime_args"] = runtime_args
        result["return_code"] = return_code
        result["report"] = report
        result["change_list"] = self._change_list
        return result

    def run(self, commit: str, src_branch: str, dst_branch: str, s3_bucket: str, s3_top_level_dir: str, suite: str, test_failure_policy: str, safe_mode: bool, test_timeout: int, global_timeout: int):
        """
        Determins the type of sequence to run based on the commit, source branch and test branch before running the
        sequence with the specified values.

        @param commit:              The commit hash of the changes to run test impact analysis on. 
        @param src_branch:          If not equal to dst_branch, the branch that is being built.
        @param dst_branch:          If not equal to src_branch, the destination branch for the PR being built.
        @param s3_bucket:           Location of S3 bucket to use for persistent storage, otherwise local disk storage will be used.
        @param s3_top_level_dir:    Top level directory to use in the S3 bucket.
        @param suite:               Test suite to run.
        @param test_failure_policy: Test failure policy for regular and test impact sequences (ignored when seeding).
        @param safe_mode:           Flag to run impact analysis tests in safe mode (ignored when seeding).
        @param test_timeout:        Maximum run time (in seconds) of any test target before being terminated (unlimited if None).
        @param global_timeout:      Maximum run time of the sequence before being terminated (unlimited if None).
        """

        args = []
        persistent_storage = None
        self._has_historic_data = False
        self._change_list = {}
        self._change_list["createdFiles"] = []
        self._change_list["updatedFiles"] = []
        self._change_list["deletedFiles"] = []

        # Branches
        self._src_branch = src_branch
        self._dst_branch = dst_branch
        logger.info(f"Src branch: '{self._src_branch}'.")
        logger.info(f"Dst branch: '{self._dst_branch}'.")

        # Source of truth (the branch from which the coverage data will be stored/retrieved from)
        if not self._dst_branch or self._src_branch == self._dst_branch:
            # Branch builds are their own source of truth and will update the coverage data for the source of truth after any instrumented sequences complete
            self._is_source_of_truth_branch = True
            self._source_of_truth_branch = self._src_branch
        else:
            # Pull request builds use their destination as the source of truth and never update the coverage data for the source of truth
            self._is_source_of_truth_branch = False
            self._source_of_truth_branch = self._dst_branch

        logger.info(f"Source of truth branch: '{self._source_of_truth_branch}'.")
        logger.info(f"Is source of truth branch: '{self._is_source_of_truth_branch}'.")

        # Commit
        self._dst_commit = commit
        logger.info(f"Commit: '{self._dst_commit}'.")
        self._src_commit = None
        self._commit_distance = None

        # Generate a unique ID to be used as part of the file name for required runtime dynamic artifacts.
        self._instance_id = uuid.uuid4().hex
        
        if self._use_test_impact_analysis:
            logger.info("Test impact analysis is enabled.")
            try:
                # Persistent storage location
                if s3_bucket:
                    persistent_storage = PersistentStorageS3(self._config, suite, self._dst_commit, s3_bucket, s3_top_level_dir, self._source_of_truth_branch)
                else:
                    persistent_storage = PersistentStorageLocal(self._config, suite, self._dst_commit)
            except SystemError as e:
                logger.warning(f"The persistent storage encountered an irrecoverable error, test impact analysis will be disabled: '{e}'")
                persistent_storage = None

            if persistent_storage:
                
                # Flag for corner case where:
                # 1. TIAF was already run previously for this commit.
                # 2. There was no last commit hash when TIAF last ran on this commit (due to no coverage data existing yet for this branch)
                # 3. TIAF has not been run on any other commits between the run for this commit and the last run for this commit.
                # The above results in TIAF being stuck in a state of generating an empty change list (and thus doing no work until another
                # commit comes in) which is problematic if the commit needs to be re-run for whatever reason so in these conditions we revert
                # back to a regular test run until another commit comes in
                can_rerun_with_instrumentation = True

                if persistent_storage.has_historic_data:
                    logger.info("Historic data found.")
                    self._src_commit = persistent_storage.last_commit_hash

                    # Check to see if this is a re-run for this commit before any other changes have come in
                    if persistent_storage.is_repeat_sequence:
                        if persistent_storage.can_rerun_sequence:
                            logger.info(f"This sequence is being re-run before any other changes have come in so the last commit '{persistent_storage.this_commit_last_commit_hash}' used for the previous sequence will be used instead.")
                            self._src_commit = persistent_storage.this_commit_last_commit_hash
                        else:
                            logger.info(f"This sequence is being re-run before any other changes have come in but there is no useful historic data. A regular sequence will be performed instead.")
                            persistent_storage = None
                            can_rerun_with_instrumentation = False
                    else:
                        self._attempt_to_generate_change_list()
                else:
                    logger.info("No historic data found.")
                    
                # Sequence type
                if self._has_change_list:
                    if self._is_source_of_truth_branch:
                        # Use TIA sequence (instrumented subset of tests) for coverage updating branches so we can update the coverage data with the generated coverage
                        sequence_type = "tia"
                    else:
                        # Use TIA no-write sequence (regular subset of tests) for non coverage updating branche
                        sequence_type = "tianowrite"
                        # Ignore integrity failures for non coverage updating branches as our confidence in the
                        args.append("--ipolicy=continue")
                        logger.info("Integration failure policy is set to 'continue'.")
                    # Safe mode
                    if safe_mode:
                        args.append("--safemode=on")
                        logger.info("Safe mode set to 'on'.")
                    else:
                        args.append("--safemode=off")
                        logger.info("Safe mode set to 'off'.")
                    # Change list
                    args.append(f"--changelist={self._change_list_path}")
                    logger.info(f"Change list is set to '{self._change_list_path}'.")
                else:
                    if self._is_source_of_truth_branch and can_rerun_with_instrumentation:
                        # Use seed sequence (instrumented all tests) for coverage updating branches so we can generate the coverage bed for future sequences
                        sequence_type = "seed"
                        # We always continue after test failures when seeding to ensure we capture the coverage for all test targets
                        test_failure_policy = "continue"
                    else:
                        # Use regular sequence (regular all tests) for non coverage updating branches as we have no coverage to use nor coverage to update
                        sequence_type = "regular"
                        # Ignore integrity failures for non coverage updating branches as our confidence in the
                        args.append("--ipolicy=continue")
                        logger.info("Integration failure policy is set to 'continue'.")
            else:
                # Use regular sequence (regular all tests) when the persistent storage fails to avoid wasting time generating seed data that will not be preserved
                sequence_type = "regular"
        else:
            # Use regular sequence (regular all tests) when test impact analysis is disabled
            sequence_type = "regular"
        args.append(f"--sequence={sequence_type}")
        logger.info(f"Sequence type is set to '{sequence_type}'.")

         # Test failure policy
        args.append(f"--fpolicy={test_failure_policy}")
        logger.info(f"Test failure policy is set to '{test_failure_policy}'.")

        # Sequence report
        report_file = pathlib.PurePath(self._temp_workspace).joinpath(f"report.{self._instance_id}.json")
        args.append(f"--report={report_file}")
        logger.info(f"Sequence report file is set to '{report_file}'.")

        # Suite
        args.append(f"--suite={suite}")
        logger.info(f"Test suite is set to '{suite}'.")

        # Timeouts
        if test_timeout is not None:
            args.append(f"--ttimeout={test_timeout}")
            logger.info(f"Test target timeout is set to {test_timeout} seconds.")
        if global_timeout is not None:
            args.append(f"--gtimeout={global_timeout}")
            logger.info(f"Global sequence timeout is set to {test_timeout} seconds.")

        # Run sequence
        unpacked_args = " ".join(args)
        logger.info(f"Args: {unpacked_args}")
        runtime_result = subprocess.run([str(self._tiaf_bin)] + args)
        report = None
        # If the sequence completed (with or without failures) we will update the historical meta-data
        if runtime_result.returncode == 0 or runtime_result.returncode == 7:
            logger.info("Test impact analysis runtime returned successfully.")

            # Get the sequence report the runtime generated
            with open(report_file) as json_file:
                report = json.load(json_file)

            # Attempt to store the historic data for this branch and sequence
            if self._is_source_of_truth_branch and persistent_storage is not None:
                persistent_storage.update_and_store_historic_data()
        else:
            logger.error(f"The test impact analysis runtime returned with error: '{runtime_result.returncode}'.")
    
        return self._generate_result(s3_bucket, suite, runtime_result.returncode, report, args)