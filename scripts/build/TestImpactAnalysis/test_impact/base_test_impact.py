#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from abc import ABC, abstractmethod
from pathlib import PurePath, Path
import json
import subprocess
import re
import uuid
from test_impact import RuntimeArgs
from git_utils import Repo
from persistent_storage import PersistentStorageLocal, PersistentStorageS3
from tiaf_tools import get_logger
import tiaf_report_constants as constants

logger = get_logger(__file__)

# Constants to access our argument dictionary for the values of different arguments. Not guarunteed to be in dictionary in all cases.
ARG_S3_BUCKET = 's3_bucket'
ARG_SUITES = 'suites'
ARG_LABEL_EXCLUDES = 'label_excludes'
ARG_CONFIG = 'config'
ARG_SOURCE_BRANCH = 'src_branch'
ARG_DESTINATION_BRANCH = 'dst_branch'
ARG_COMMIT = 'commit'
ARG_S3_TOP_LEVEL_DIR = 's3_top_level_dir'
ARG_SEQUENCE_OVERRIDE = 'sequence_override'
ARG_INTEGRATION_POLICY = RuntimeArgs.COMMON_IPOLICY.driver_argument
ARG_TEST_FAILURE_POLICY = RuntimeArgs.COMMON_FPOLICY.driver_argument
ARG_CHANGE_LIST = RuntimeArgs.COMMON_CHANGELIST.driver_argument
ARG_SEQUENCE = RuntimeArgs.COMMON_SEQUENCE.driver_argument
ARG_REPORT = RuntimeArgs.COMMON_REPORT.driver_argument

# Sequence types as constants
TIA_NOWRITE = 'tianowrite'
TIA_SEED = 'seed'
TIA_ON = 'tia'
TIA_REGULAR = 'regular'


class BaseTestImpact(ABC):

    _runtime_type = None

    def __init__(self, args: dict):
        """
        Initializes the test impact model with the commit, branches as runtime configuration.

        @param args: The arguments to be parsed and applied to this TestImpact object.
        """

        self._runtime_args = []
        self._change_list = {"createdFiles": [],
                             "updatedFiles": [], "deletedFiles": []}
        self._has_change_list = False
        self._enabled = False
        self._use_test_impact_analysis = False

        # Unique instance id to be used as part of the report name.
        self._instance_id = uuid.uuid4().hex

        self._s3_bucket = args.get(ARG_S3_BUCKET)
        self._suites = args.get(ARG_SUITES)
        self._label_excludes = args.get(ARG_LABEL_EXCLUDES)

        # Compile the dash-separated concatenation of the ordered suites and labels to be used as path components
        self._suites_string = '-'.join(self._suites) if isinstance(self._suites, list) else self._suites
        self._label_excludes_string = '-'.join(self._label_excludes) if isinstance(self._label_excludes, list) else self._label_excludes

        self._config = self._parse_config_file(args.get(ARG_CONFIG))
        if not self._enabled:
            logger.info(f"TIAF is disabled.")
            return

        # Initialize branches
        self._src_branch = args.get(ARG_SOURCE_BRANCH)
        self._dst_branch = args.get(ARG_DESTINATION_BRANCH)
        logger.info(f"Source branch: '{self._src_branch}'.")
        logger.info(f"Destination branch: '{self._dst_branch}'.")

        # Determine our source of truth. Also intializes our source of truth property.
        self._determine_source_of_truth()

        # Initialize commit info
        self._dst_commit = args.get(ARG_COMMIT)
        logger.info(f"Commit: '{self._dst_commit}'.")
        self._src_commit = None
        self._commit_distance = None

        sequence_type = self._default_sequence_type

        # If flag is set for us to use TIAF
        if self._use_test_impact_analysis:
            logger.info("Test impact analysis is enabled.")
            self._persistent_storage = self._initialize_persistent_storage(s3_top_level_dir=args.get(ARG_S3_TOP_LEVEL_DIR))

            # If persistent storage intialized correctly
            if self._persistent_storage:

                # Historic Data Handling:
                # This flag is used to help handle our corner cases if we have historic data.
                # NOTE: need to draft in failing tests or only update upon success otherwise reruns for failed runs will have the same last commit
                # hash as the commit and generate an empty changelist
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
                        sequence_type = TIA_ON
                    else:
                        # Use TIA no-write sequence (regular subset of tests) for non coverage updating branches
                        sequence_type = TIA_NOWRITE
                        # Ignore integrity failures for non coverage updating branches as our confidence in the
                        args[ARG_INTEGRATION_POLICY] = "continue"
                    args[ARG_CHANGE_LIST] = self._change_list_path
                else:
                    if self._is_source_of_truth_branch and self._can_rerun_with_instrumentation:
                        # Use seed sequence (instrumented all tests) for coverage updating branches so we can generate the coverage bed for future sequences
                        sequence_type = TIA_SEED
                        # We always continue after test failures when seeding to ensure we capture the coverage for all test targets
                        args[ARG_TEST_FAILURE_POLICY] = "continue"
                    else:
                        # Use regular sequence (regular all tests) for non coverage updating branches as we have no coverage to use nor coverage to update
                        sequence_type = TIA_REGULAR
                        # Ignore integrity failures for non coverage updating branches as our confidence in the
                        args[ARG_INTEGRATION_POLICY] = "continue"
        # Store sequence and report into args so that our argument enum can be used to apply all relevant arguments.
        args[ARG_SEQUENCE] = args.get(ARG_SEQUENCE_OVERRIDE) or sequence_type
        self._report_file = PurePath(self._report_workspace).joinpath(
            f"report.{self._instance_id}.json")
        args[ARG_REPORT] = self._report_file
        self._parse_arguments_to_runtime(args)

    def _parse_arguments_to_runtime(self, args):
        """
        Fetches the relevant keys from the provided dictionary, and applies the values of the arguments(or applies them as a flag) to our runtime_args list.

        @param args: Dictionary containing the arguments passed to this TestImpact object. Will contain all the runtime arguments we need to apply.
        """

        for argument in RuntimeArgs:
            value = args.get(argument.driver_argument)
            if value:
                self._runtime_args.append(f"{argument.runtime_arg}{','.join(value) if isinstance(value, list) else value}") 
                logger.info(f"{argument.message}{value}")

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

    def _initialize_persistent_storage(self, s3_top_level_dir: str = None):
        """
        Initialise our persistent storage object. Defaults to initialising local storage, unless the s3_bucket argument is not None.
        Returns PersistentStorage object or None if initialisation failed.

        @param s3_top_level_dir: The name of the top level directory to use in the s3 bucket.

        @returns: Returns a persistent storage object, or None if a SystemError exception occurs while initialising the object.
        """
        try:
            if self._s3_bucket:
                return PersistentStorageS3(
                    self._config, self._suites_string, self._dst_commit, self._s3_bucket, self._compile_s3_top_level_dir_name(s3_top_level_dir), self._source_of_truth_branch, self._active_workspace, self._unpacked_coverage_data_file, self._previous_test_run_data_file, self._temp_workspace)
            else:
                return PersistentStorageLocal(
                    self._config, self._suites_string, self._dst_commit, self._active_workspace, self._unpacked_coverage_data_file, self._previous_test_run_data_file, self._historic_workspace, self._historic_data_file, self._temp_workspace)
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

        COMMON_CONFIG_KEY = "common"
        WORKSPACE_KEY = "workspace"
        HISTORIC_SEQUENCES_KEY = "historic_sequences"
        ACTIVE_KEY = "active"
        ROOT_KEY = "root"
        TEMP_KEY = "temp"
        REPO_KEY = "repo"
        HISTORIC_KEY = "historic"
        RELATIVE_PATHS_KEY = "relative_paths"
        TEST_IMPACT_DATA_FILE_KEY = "test_impact_data_file"
        PREVIOUS_TEST_RUN_DATA_FILE_KEY = "previous_test_run_data_file"
        LAST_COMMIT_HASH_KEY = "last_commit_hash"
        COVERAGE_DATA_KEY = "coverage_data"
        PREVIOUS_TEST_RUNS_KEY = "previous_test_runs"
        HISTORIC_DATA_FILE_KEY = "data"
        JENKINS_KEY = "jenkins"
        ENABLED_KEY = "enabled"
        USE_TEST_IMPACT_ANALYSIS_KEY = "use_test_impact_analysis"
        RUNTIME_BIN_KEY = "runtime_bin"
        RUNTIME_ARTIFACT_DIR_KEY = "run_artifact_dir"
        RUNTIME_COVERAGE_DIR_KEY = "coverage_artifact_dir"
        REPORT_KEY = "reports"
        CHANGE_LIST_KEY = "change_list"

        logger.info(
            f"Attempting to parse configuration file '{config_file}'...")
        try:
            with open(config_file, "r") as config_data:
                config = json.load(config_data)
                self._repo_dir = config[COMMON_CONFIG_KEY][REPO_KEY][ROOT_KEY]
                self._repo = Repo(self._repo_dir)

                # TIAF
                self._enabled = config[self.runtime_type][JENKINS_KEY][ENABLED_KEY]
                self._use_test_impact_analysis = config[self.runtime_type][JENKINS_KEY][USE_TEST_IMPACT_ANALYSIS_KEY]
                self._tiaf_bin = Path(
                    config[self.runtime_type][RUNTIME_BIN_KEY])
                if self._use_test_impact_analysis and not self._tiaf_bin.is_file():
                    logger.warning(
                        f"Could not find TIAF binary at location {self._tiaf_bin}, TIAF will be turned off.")
                    self._use_test_impact_analysis = False
                else:
                    logger.info(
                        f"Runtime binary found at location '{self._tiaf_bin}'")

                # Workspaces
                self._active_workspace = config[self.runtime_type][WORKSPACE_KEY][ACTIVE_KEY][ROOT_KEY]
                self._historic_workspace = config[self.runtime_type][WORKSPACE_KEY][HISTORIC_KEY][ROOT_KEY]
                self._temp_workspace = config[self.runtime_type][WORKSPACE_KEY][TEMP_KEY][ROOT_KEY]
                self._report_workspace = config[self.runtime_type][WORKSPACE_KEY][TEMP_KEY][REPORT_KEY]
                self._change_list_workspace = config[self.runtime_type][WORKSPACE_KEY][TEMP_KEY][CHANGE_LIST_KEY]

                # Data file paths
                self._unpacked_coverage_data_file = config[self.runtime_type][
                    WORKSPACE_KEY][ACTIVE_KEY][RELATIVE_PATHS_KEY][TEST_IMPACT_DATA_FILE_KEY]
                self._previous_test_run_data_file = config[self.runtime_type][WORKSPACE_KEY][
                    ACTIVE_KEY][RELATIVE_PATHS_KEY][PREVIOUS_TEST_RUN_DATA_FILE_KEY]
                self._historic_data_file = config[self.runtime_type][WORKSPACE_KEY][
                    HISTORIC_KEY][RELATIVE_PATHS_KEY][HISTORIC_DATA_FILE_KEY]

                # Runtime artifact and coverage directories
                self._runtime_artifact_directory = config[self.runtime_type][WORKSPACE_KEY][TEMP_KEY][RUNTIME_ARTIFACT_DIR_KEY]
                self._runtime_coverage_directory = config[self.runtime_type][WORKSPACE_KEY][TEMP_KEY][RUNTIME_COVERAGE_DIR_KEY]
                logger.info("The configuration file was parsed successfully.")
                return config
        except KeyError as e:
            logger.error(f"The config does not contain the key {str(e)}.")
            return None
        except json.JSONDecodeError as e:
            logger.error("The config file doesn not contain valid JSON")
            raise SystemError(
                "Config file does not contain valid JSON, stopping TIAF")

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
                diff_path = Path(PurePath(self._change_list_workspace).joinpath(
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
            change_list_path = PurePath(self._temp_workspace).joinpath(
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

    def _generate_result(self, return_code: int, report: dict):
        """
        Generates the result object from the pertinent runtime meta-data and sequence report.

        @param The generated result object.
        """

        result = {}
        result[constants.SRC_COMMIT_KEY] = self._src_commit
        result[constants.DST_COMMIT_KEY] = self._dst_commit
        result[constants.COMMIT_DISTANCE_KEY] = self._commit_distance
        result[constants.SRC_BRANCH_KEY] = self._src_branch
        result[constants.DST_BRANCH_KEY] = self._dst_branch
        result[constants.SUITES_KEY] = self._suites
        result[constants.LABEL_EXCLUDES_KEY] = self._label_excludes
        result[constants.USE_TEST_IMPACT_ANALYSIS_KEY] = self._use_test_impact_analysis
        result[constants.SOURCE_OF_TRUTH_BRANCH_KEY] = self._source_of_truth_branch
        result[constants.IS_SOURCE_OF_TRUTH_BRANCH_KEY] = self._is_source_of_truth_branch
        result[constants.HAS_CHANGE_LIST_KEY] = self._has_change_list
        result[constants.HAS_HISTORIC_DATA_KEY] = self._has_historic_data
        result[constants.S3_BUCKET_KEY] = self._s3_bucket
        result[constants.RUNTIME_ARGS_KEY] = self._runtime_args
        result[constants.RUNTIME_RETURN_CODE_KEY] = return_code
        result[constants.REPORT_KEY] = report
        result[constants.CHANGE_LIST_KEY] = self._change_list
        result[constants.RUNTIME_TYPE_KEY] = self.runtime_type
        mismatched_tests = self._cross_check_tests(report)
        result[constants.MISMATCHED_TESTS_KEY] = mismatched_tests
        result[constants.MISMATCHED_TESTS_COUNT_KEY] = len(mismatched_tests)
        return result

    def _cross_check_tests(self, report: dict):
        """
        Function to compare our report with the report provided by another test runner. Will perform a comparison and return a list of any tests that failed in the other test runner that did not fail in TIAF, or were not selected.
        Returns an empty list if not overloaded by a specialised test impact class.
        @param report: Dictionary containing the report provided by TIAF binary
        @return: List of tests that failed in test runner but did not fail in TIAF or weren't selected by TIAF.
        """
        return []

    def _compile_s3_top_level_dir_name(self, dir_name: str):
        """
        Function to build our s3_top_level_dir name. Reads the argument from our dictionary and then appends runtime_type to the end.
        If s3_top_level_dir name is not provided in args, we will default to "tiaf"+runtime_type.

        @param dir_name: Name of the directory to use as top level when compiling directory name.

        @return: Compiled s3_top_level_dir name
        """
        if dir_name:
            dir_name = f"{dir_name}/{self.runtime_type}"
            return dir_name
        raise SystemError(
            "s3_top_level_dir not set while trying to access s3 instance.")

    def _extract_test_runs_from_test_run_report(self, report: dict):
        """
        Extract all test runs from a test run report and store in one list
        @param report: The test run report.
        """
        test_runs = []
        test_runs += report[constants.PASSING_TEST_RUNS_KEY]
        test_runs += report[constants.FAILING_TEST_RUNS_KEY]
        test_runs += report[constants.EXECUTION_FAILURE_TEST_RUNS_KEY]
        test_runs += report[constants.TIMED_OUT_TEST_RUNS_KEY]
        test_runs += report[constants.UNEXECUTED_TEST_RUNS_KEY]
        return test_runs

    def _extract_test_runs_from_sequence_report(self, report):
        """
        Extract all relevant test runs from the report generated by the TIAF runtime.
        @param report: The report generated by the TIAF runtime, in dict format.
        """
        report_type = report[constants.SEQUENCE_TYPE_KEY]
        test_runs = self._extract_test_runs_from_test_run_report(
            report[constants.SELECTED_TEST_RUN_REPORT_KEY])

        if report_type == constants.IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY or report_type == constants.SAFE_IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY:
            test_runs = test_runs + \
                self._extract_test_runs_from_test_run_report(
                    report[constants.DRAFTED_TEST_RUN_REPORT_KEY])
            if report_type == constants.SAFE_IMPACT_ANALYSIS_SEQUENCE_TYPE_KEY:
                test_runs = test_runs + self._extract_test_runs_from_test_run_report(
                    report[constants.DISCARDED_TEST_RUN_REPORT_KEY])
        return test_runs

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
        args = [str(self._tiaf_bin)] + self._runtime_args
        runtime_result = subprocess.run(args, shell=True)
        report = None
        # If the sequence completed (with or without failures) we will update the historical meta-data
        if runtime_result.returncode == 0 or runtime_result.returncode == 7:
            logger.info("Test impact analysis runtime returned successfully.")

            # Get the sequence report the runtime generated
            with open(self._report_file) as json_file:
                report = json.load(json_file)

            # Grab the list of failing test targets for this sequence
            test_runs = self._extract_test_runs_from_sequence_report(report)

            # Attempt to store the historic data and artifacts for this branch and sequence
            if self._persistent_storage:
                if self._is_source_of_truth_branch:
                    self._persistent_storage.update_and_store_historic_data(test_runs)
                self._persistent_storage.store_artifacts(self._runtime_artifact_directory, self._runtime_coverage_directory)
        else:
            logger.error(
                f"The test impact analysis runtime returned with error: '{runtime_result.returncode}'.")

        return self._generate_result(runtime_result.returncode, report)

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

    @property
    def enabled(self):
        return self._enabled

    @property
    def source_branch(self):
        """
        The source branch for this TIAF run.
        """
        return self._src_branch

    @property
    def destination_branch(self):
        """
        The destination branch for this TIAF run.
        """
        return self._dst_branch

    @property
    def destination_commit(self):
        """
        The destination commit for this TIAF run.
        Destination commit is the commit that is being built.
        """
        return self._dst_commit

    @property
    def source_commit(self):
        """
        The source commit for this TIAF run.
        Source commit is the commit that we compare to for PR builds.
        """
        return self._src_commit

    @property
    def runtime_args(self):
        """
        The arguments to be passed to the TIAF runtime.
        """
        return self._runtime_args

    @property
    def has_change_list(self):
        """
        True if a change list has been generated for this TIAF run.
        """
        return self._has_change_list

    @property
    def instance_id(self):
        """
        The instance id of this TestImpact object.
        """
        return self._instance_id

    @property
    def test_suites(self):
        """
        The test suites being executed.
        """
        return self._suites

    @property
    def source_of_truth_branch(self):
        """
        The source of truth branch for this TIAF run.
        """
        return self._source_of_truth_branch

    @property
    @abstractmethod
    def runtime_type(self):
        """
        The runtime this TestImpact supports. Must be implemented by subclass.
        Current options are "native" or "python".
        """
        pass

    @property
    @abstractmethod
    def default_sequence_type(self):
        """
        The default sequence type for this TestImpact class. Must be implemented by subclass.
        """
        pass
