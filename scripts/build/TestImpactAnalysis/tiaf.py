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

    def __init__(self, args: dict):
        """
        Initializes the test impact model with the commit, branches as runtime configuration.

        @param config_file: The runtime config file to obtain the runtime configuration data from.
        @param args: The arguments to be parsed and applied to this TestImpact object.
        """

        self._runtime_args = []
        self._change_list = {"createdFiles": [],
                             "updatedFiles": [], "deletedFiles": []}
        self._has_change_list = False
        self._use_test_impact_analysis = False

        # Unique instance id to be used as part of the report name.
        self._instance_id = uuid.uuid4().hex

        self._s3_bucket = args.get('s3_bucket')
        self._suite = args.get('suite')

        self._config = self._parse_config_file(args['config'])

        # Initialize branches
        self._src_branch = args.get("src_branch")
        self._dst_branch = args.get("dst_branch")
        logger.info(f"Source branch: '{self._src_branch}'.")
        logger.info(f"Destination branch: '{self._dst_branch}'.")

        # Determine our source of truth. Also intializes our source of truth property.
        self._determine_source_of_truth()

        # Initialize commit info
        self._dst_commit = args.get("commit")
        logger.info(f"Commit: '{self._dst_commit}'.")
        self._src_commit = None
        self._commit_distance = None

        # Default to a regular sequence run unless otherwise specified
        sequence_type = "regular"

        # If flag is set for us to use TIAF
        if self._use_test_impact_analysis:
            logger.info("Test impact analysis is enabled.")
            self._persistent_storage = self._intialize_persistent_storage(
                s3_bucket=self._s3_bucket, suite=self._suite, s3_top_level_dir=args.get('s3_top_level_dir'))

            # If persistent storage intialized correctly
            if self._persistent_storage:

                # Historic Data Handling:
                # This flag is used to help handle our corner cases if we have historic data.
                self._can_rerun_with_instrumentation = True
                if self._persistent_storage.has_historic_data:
                    logger.info("Historic data found.")
                    self._handle_historic_data()
                else:
                    logger.info("No historic data found.")

                # Determining our sequence type:
                if self._has_change_list:
                    if self._is_source_of_truth_branch:
                        # Use TIA sequence (instrumented subset of tests) for coverage updating branches so we can update the coverage data with the generated coverage
                        sequence_type = "tia"
                    else:
                        # Use TIA no-write sequence (regular subset of tests) for non coverage updating branches
                        sequence_type = "tianowrite"
                        # Ignore integrity failures for non coverage updating branches as our confidence in the
                        self._runtime_args.append("--ipolicy=continue")
                        logger.info(
                            "Integration failure policy is set to 'continue'.")
                    self._runtime_args.append(
                        f"--changelist={self._change_list_path}")
                    logger.info(
                        f"Change list is set to '{self._change_list_path}'.")
                else:
                    if self._is_source_of_truth_branch and self._can_rerun_with_instrumentation:
                        # Use seed sequence (instrumented all tests) for coverage updating branches so we can generate the coverage bed for future sequences
                        sequence_type = "seed"
                        # We always continue after test failures when seeding to ensure we capture the coverage for all test targets
                        args['test_failure_policy'] = "continue"
                    else:
                        # Use regular sequence (regular all tests) for non coverage updating branches as we have no coverage to use nor coverage to update
                        sequence_type = "regular"
                        # Ignore integrity failures for non coverage updating branches as our confidence in the
                        self._runtime_args.append("--ipolicy=continue")
                        logger.info(
                            "Integration failure policy is set to 'continue'.")

        self._parse_arguments_to_runtime(
            args, sequence_type, self._runtime_args)

    def _parse_arguments_to_runtime(self, args, sequence_type, runtime_args):
        """
        Fetches the relevant keys from the provided dictionary, and applies the values of the arguments(or applies them as a flag) to our runtime_args list.

        @param args: Dictionary containing the arguments passed to this TestImpact object. Will contain all the runtime arguments we need to apply.
        @sequence_type: The sequence type as determined when initialising this TestImpact object.
        @runtime_args: A list of strings that will become the arguments for our runtime.
        """
        runtime_args.append(f"--sequence={sequence_type}")
        logger.info(f"Sequence type is set to '{sequence_type}'.")

        if args.get('safe_mode'):
            runtime_args.append("--safemode=on")
            logger.info("Safe mode set to 'on'.")
        else:
            runtime_args.append("--safemode=off")
            logger.info("Safe mode set to 'off'.")

        # Test failure policy
        test_failure_policy = args.get('test_failure_policy')
        runtime_args.append(f"--fpolicy={test_failure_policy}")
        logger.info(f"Test failure policy is set to '{test_failure_policy}'.")

        # Sequence report
        self._report_file = pathlib.PurePath(self._temp_workspace).joinpath(
            f"report.{self._instance_id}.json")
        runtime_args.append(f"--report={self._report_file}")
        logger.info(f"Sequence report file is set to '{self._report_file}'.")

        # Suite
        suite = args.get('suite')
        runtime_args.append(f"--suite={suite}")
        logger.info(f"Test suite is set to '{suite}'.")

        # Exclude tests
        exclude_file = args.get('exclude_file')
        if exclude_file:
            runtime_args.append(f"--exclude_file={exclude_file}")
            logger.info(
                f"Exclude file found, excluding the tests stored at '{exclude_file}'.")
        else:
            logger.info(f'Exclude file not found, skipping.')

        # Timeouts
        test_timeout = args.get('test_timeout')
        if test_timeout:
            runtime_args.append(f"--ttimeout={test_timeout}")
            logger.info(
                f"Test target timeout is set to {test_timeout} seconds.")

        global_timeout = args.get('global_timeout')
        if global_timeout:
            runtime_args.append(f"--gtimeout={global_timeout}")
            logger.info(
                f"Global sequence timeout is set to {test_timeout} seconds.")

    def _handle_historic_data(self):
        """
        This method handles the different cases of when we have historic data, and carries out the desired action.
        Case 1:
            This commit is different to the last commit in our historic data. Action: Generate change-list.
        Case 2:
            This commit has already been run in TIAF, and we have useful historic data. Action: Use that data for our TIAF run.
        Case 3:
            This commit has already been run in TIAF, but we have no useful historic data for it. Action: A regular sequence is performed instead. Persistent storage is set to none and rerun_with_instrumentation is set to false.
        """
        # src commit is set to the commit hash of the last commit we have historic data for
        self._src_commit = self._persistent_storage.last_commit_hash

        # Check to see if this is a re-run for this commit before any other changes have come in

        # If the last commit hash in our historic data is the same as our current commit hash
        if self._persistent_storage.is_last_commit_hash_equal_to_this_commit_hash:

            # If we have the last commit hash of our previous run in our json then we will just use the data from that run
            if self._persistent_storage.has_previous_last_commit_hash:
                logger.info(
                    f"This sequence is being re-run before any other changes have come in so the last commit '{self._persistent_storage.this_commit_last_commit_hash}' used for the previous sequence will be used instead.")
                self._src_commit = self._persistent_storage.this_commit_last_commit_hash
            else:
                # If we don't have the last commit hash of our previous run then we do a regular run as there will be no change list and no historic coverage data to use
                logger.info(
                    f"This sequence is being re-run before any other changes have come in but there is no useful historic data. A regular sequence will be performed instead.")
                self._persistent_storage = None
                self._can_rerun_with_instrumentation = False
        else:
            # If this commit is different to the last commit in our historic data, we can diff the commits to get our change list
            self._attempt_to_generate_change_list()

    def _intialize_persistent_storage(self, suite: str,  s3_bucket: str = None, s3_top_level_dir: str = None):
        """
        Initialise our persistent storage object. Defaults to initialising local storage, unless the s3_bucket argument is not None. 
        Returns PersistentStorage object or None if initialisation failed.

        @param suite: The testing suite we are using.
        @param s3_bucket: the name of the S3 bucket to connect to. Can be set to none.
        @param s3_top_level_dir: The name of the top level directory to use in the s3 bucket.

        @returns: Returns a persistent storage object, or None if a SystemError exception occurs while initialising the object.
        """
        try:
            if s3_bucket:
                return PersistentStorageS3(
                    self._config, suite, self._dst_commit, s3_bucket, s3_top_level_dir, self._source_of_truth_branch)
            else:
                return PersistentStorageLocal(
                    self._config, suite, self._dst_commit)
        except SystemError as e:
            logger.warning(
                f"The persistent storage encountered an irrecoverable error, test impact analysis will be disabled: '{e}'")
            return None

    def _determine_source_of_truth(self):
        """
        Determines whether the branch we are executing TIAF on is the source of truth (the branch from which the coverage data will be stored/retrieved from) or not.        
        """
        # Source of truth (the branch from which the coverage data will be stored/retrieved from)
        if not self._dst_branch or self._src_branch == self._dst_branch:
            # Branch builds are their own source of truth and will update the coverage data for the source of truth after any instrumented sequences complete
            self._source_of_truth_branch = self._src_branch
        else:
            # Pull request builds use their destination as the source of truth and never update the coverage data for the source of truth
            self._source_of_truth_branch = self._dst_branch

        logger.info(
            f"Source of truth branch: '{self._source_of_truth_branch}'.")
        logger.info(
            f"Is source of truth branch: '{self._is_source_of_truth_branch}'.")

    def _parse_config_file(self, config_file: str):
        """
        Parse the configuration file and retrieve the data needed for launching the test impact analysis runtime.

        @param config_file: The runtime config file to obtain the runtime configuration data from.
        """

        logger.info(
            f"Attempting to parse configuration file '{config_file}'...")
        try:
            with open(config_file, "r") as config_data:
                config = json.load(config_data)
                self._repo_dir = config["repo"]["root"]
                self._repo = Repo(self._repo_dir)

                # TIAF
                self._use_test_impact_analysis = config["jenkins"]["use_test_impact_analysis"]
                self._tiaf_bin = pathlib.Path(config["repo"]["tiaf_bin"])
                if self._use_test_impact_analysis and not self._tiaf_bin.is_file():
                    logger.warning(
                        f"Could not find TIAF binary at location {self._tiaf_bin}, TIAF will be turned off.")
                    self._use_test_impact_analysis = False
                else:
                    logger.info(
                        f"Runtime binary found at location '{self._tiaf_bin}'")

                # Workspaces
                self._active_workspace = config["workspace"]["active"]["root"]
                self._historic_workspace = config["workspace"]["historic"]["root"]
                self._temp_workspace = config["workspace"]["temp"]["root"]
                logger.info("The configuration file was parsed successfully.")
                return config
        except KeyError as e:
            logger.error(f"The config does not contain the key {str(e)}.")
            return None

    def _attempt_to_generate_change_list(self):
        """
        Attempts to determine the change list between now and the last tiaf run (if any).
        """

        self._has_change_list = False
        self._change_list_path = None

        # Check whether or not a previous commit hash exists (no hash is not a failure)
        if self._src_commit:
            if self._is_source_of_truth_branch:
                # For branch builds, the dst commit must be descended from the src commit
                if not self._repo.is_descendent(self._src_commit, self._dst_commit):
                    logger.error(
                        f"Source commit '{self._src_commit}' and destination commit '{self._dst_commit}' must be related for branch builds.")
                    return

                # Calculate the distance (in commits) between the src and dst commits
                self._commit_distance = self._repo.commit_distance(
                    self._src_commit, self._dst_commit)
                logger.info(
                    f"The distance between '{self._src_commit}' and '{self._dst_commit}' commits is '{self._commit_distance}' commits.")
                multi_branch = False
            else:
                # For pull request builds, the src and dst commits are on different branches so we need to ensure a common ancestor is used for the diff
                multi_branch = True

            try:
                # Attempt to generate a diff between the src and dst commits
                logger.info(
                    f"Source '{self._src_commit}' and destination '{self._dst_commit}' will be diff'd.")
                diff_path = pathlib.Path(pathlib.PurePath(self._temp_workspace).joinpath(
                    f"changelist.{self._instance_id}.diff"))
                self._repo.create_diff_file(
                    self._src_commit, self._dst_commit, diff_path, multi_branch)
            except RuntimeError as e:
                logger.error(e)
                return

            # A diff was generated, attempt to parse the diff and construct the change list
            logger.info(
                f"Generated diff between commits '{self._src_commit}' and '{self._dst_commit}': '{diff_path}'.")
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
                                self._change_list["createdFiles"].append(
                                    match[1])
                            elif line[0] == 'M':
                                # File modification
                                self._change_list["updatedFiles"].append(
                                    match[1])
                            elif line[0] == 'D':
                                # File Deletion
                                self._change_list["deletedFiles"].append(
                                    match[1])

            # Serialize the change list to the JSON format the test impact analysis runtime expects
            change_list_json = json.dumps(self._change_list, indent=4)
            change_list_path = pathlib.PurePath(self._temp_workspace).joinpath(
                f"changelist.{self._instance_id}.json")
            f = open(change_list_path, "w")
            f.write(change_list_json)
            f.close()
            logger.info(
                f"Change list constructed successfully: '{change_list_path}'.")
            logger.info(
                f"{len(self._change_list['createdFiles'])} created files, {len(self._change_list['updatedFiles'])} updated files and {len(self._change_list['deletedFiles'])} deleted files.")

            # Note: an empty change list generated due to no changes between last and current commit is valid
            self._has_change_list = True
            self._change_list_path = change_list_path
        else:
            logger.error(
                "No previous commit hash found, regular or seeded sequences only will be run.")
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

    def run(self):
        """
        Builds our runtime argument string based on the initialisation state, then executes the runtime with those arguments.
        Stores the report of this run locally.
        Updates and stores historic data if storage is intialized and source branch is source of truth.
        Returns the runtime result as a dictionary.

        @return: Runtime results in a dictionary.
        """

        unpacked_args = " ".join(self._runtime_args)
        logger.info(f"Args: {unpacked_args}")
        runtime_result = subprocess.run(
            [str(self._tiaf_bin)] + self._runtime_args)
        report = None
        # If the sequence completed (with or without failures) we will update the historical meta-data
        if runtime_result.returncode == 0 or runtime_result.returncode == 7:
            logger.info("Test impact analysis runtime returned successfully.")

            # Get the sequence report the runtime generated
            with open(self._report_file) as json_file:
                report = json.load(json_file)

            # Attempt to store the historic data for this branch and sequence
            if self._is_source_of_truth_branch and self._persistent_storage:
                self._persistent_storage.update_and_store_historic_data()
        else:
            logger.error(
                f"The test impact analysis runtime returned with error: '{runtime_result.returncode}'.")

        return self._generate_result(self._s3_bucket, self._suite, runtime_result.returncode, report, self._runtime_args)

    @property
    def _is_source_of_truth_branch(self):
        """
        True if the source branch the source of truth.
        False otherwise.
        """
        return self._source_of_truth_branch == self._src_branch

    @property
    def _has_historic_data(self):
        """
        True if persistent storage is not None and it has historic data.
        False otherwise.
        """
        if self._persistent_storage:
            return self._persistent_storage.has_historic_data
        return False
