#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from pathlib import Path
import xml.etree.ElementTree as ET
import tiaf_report_constants as constants
from test_impact import BaseTestImpact, RuntimeArgs
from tiaf_logger import get_logger

logger = get_logger(__file__)


class PythonTestImpact(BaseTestImpact):

    _runtime_type = "python"
    _default_sequence_type = "regular"
    ARG_TEST_RUNNER_POLICY = RuntimeArgs.PYTHON_TEST_RUNNER.driver_argument

    def __init__(self, args: dict):
        """
        Initializes the test impact model with the commit, branches as runtime configuration.

        @param args: The arguments to be parsed and applied to this TestImpact object.
        """
        self._test_runner_policy = args.get(self.ARG_TEST_RUNNER_POLICY, None)
        super(PythonTestImpact, self).__init__(args)

    def _cross_check_tests(self, report: dict):
        """
        Function to compare our report with the report provided by another test runner. Will perform a comparison and return a list of any tests that weren't selected by TIAF that failed in the other test runner.
        Returns an empty list if not overloaded by a specialised test impact class.
        @param report: Dictionary containing the report provided by TIAF binary
        @return: List of tests that failed in test runner but did not fail in TIAF or weren't selected by TIAF.
        """
        mismatched_test_suites = []

        # If test runner policy is any other setting or not set, we will not have Pytest xml files to consume and comparison is not possible.
        if self._test_runner_policy == "on":
            ERRORS_KEY = 'errors'
            FAILURES_KEY = 'failures'

            xml_report_map = self._parse_xml_report(
                self._runtime_artifact_directory)

            for not_selected_test_name in report[constants.SELECTED_TEST_RUNS_KEY][constants.EXCLUDED_TEST_RUNS_KEY]:
                try:
                    xml_report = xml_report_map[not_selected_test_name]
                    # If either of these are greater than zero, then a test failed or errored out, and thus this test shouldn't have passed.
                    if int(xml_report[ERRORS_KEY]) > 0 or int(xml_report[FAILURES_KEY]) > 0:
                        logger.info(
                            f"Mismatch found between the XML report for {not_selected_test_name}, logging for reporting.")
                        mismatched_test_suites.append(not_selected_test_name)
                except KeyError as e:
                    logger.warning(
                        f"Error, {e} not found in our xml_reports. Maybe it wasn't run by the other test runner.")
                    continue

        return mismatched_test_suites

    def _parse_xml_report(self, path):
        """
        Parse JUnit xml reports at the specified path and return a dictionary mapping the test suite name to the xml properties of that test suite.
        @param path: Path to the folder containing Pytest JUnit artifacts.
        @return test_results: A dictionary mapping each test suite name to the corresponding properties(tests passed, failed etc) of that test suite.
        """
        test_results = {}
        path_to_report_dir = Path(path)
        for report in path_to_report_dir.iterdir():
            tree = ET.parse(report)
            root = tree.getroot()
            for child in root:
                test_results[report.stem] = child.attrib

        return test_results

    @property
    def default_sequence_type(self):
        """
        The default sequence type for this TestImpact class. Must be implemented by subclass.
        """
        return self._default_sequence_type

    @property
    def runtime_type(self):
        """
        The runtime this TestImpact supports. Must be implemented by subclass.
        Current options are "native" or "python".
        """
        return self._runtime_type
