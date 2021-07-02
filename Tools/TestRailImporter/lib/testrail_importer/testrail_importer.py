"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Converts a jUnitXML report so the TestRail API can digest it, then pushes the test
results to TestRail with pass/fail status.

For more info on the TestRail API, see their documentation:
    http://docs.gurock.com/testrail-api2/start
"""

import argparse
import datetime
import logging
import os
import getpass
import sys

try:  # py2
    import testrail_tools.testrail_connection as testrail_connection
    import testrail_tools.testrail_report_converter as testrail_report_converter
    import testrail_tools.testrail_api_connector as testrail_api_connector
except ImportError:  # py3
    import lib.testrail_importer.testrail_tools.testrail_connection as testrail_connection
    import lib.testrail_importer.testrail_tools.testrail_report_converter as testrail_report_converter
    import lib.testrail_importer.testrail_tools.testrail_api_connector as testrail_api_connector

log = logging.getLogger(__name__)


class TestRailImporterError(Exception):
    """Raised when an expected error occurs for the TestRailImporter() class."""

    pass


class TestRailImporter(object):
    """
    Parses a jUnitXML file and pushes the report results to TestRail.
    """

    def __init__(self):
        self._cli_args = self.cli_args()

    def __filter_test_cases(self, testrun_results):
        """
        Takes a list of pass/fail test results with multiple test case IDs
        and returns a list of pass/fail test results for each test case ID.
        :param testrun_results: list containing test run pass/fail results
            with test case IDs all grouped together
        :return: list containing a pass/fail result for each test case ID
        """
        updated_test_results = []

        for test_result in testrun_results:
            filtered_result = []
            case_ids = test_result.get('case_ids')
            if case_ids:
                filtered_result = self.__filter_test_case_ids(test_result)
            updated_test_results.extend(filtered_result)
            test_result.pop('case_ids')
        log.info('TestRailImporter: Test Case ID filtering '
                 'finished: {}'.format(updated_test_results))

        return updated_test_results

    def __filter_test_case_ids(self, test_result):
        """
        Takes a list of test case IDs and generates 1 test
        result per test case ID, appends it to a list, then
        returns the list.
        :param test_result: dict containing test result values
        :return: list of test result dicts, 1 test result dict
            per test case ID or None if no test case IDs found.
        """
        result_for_each_id = []
        case_ids = test_result.get('case_ids')

        if case_ids:
            for case_id in case_ids:
                new_result = test_result.copy()
                new_result['case_id'] = case_id
                result_for_each_id.append(new_result)

        return result_for_each_id

    def cli_args(self):
        """
        Parse for CLI args to use with the TestRailImporter.
        :return: argparse.ArgumentParser containing all CLI args passed.
        """
        parser = argparse.ArgumentParser(description="TestRail XML test result importer.")
        parser.add_argument("--testrail-xml",
                            nargs='?',
                            required=True,
                            help="REQUIRED: Path to jUnitXML file.")
        parser.add_argument("--testrail-url",
                            nargs='?',
                            required=True,
                            help="REQUIRED: URL of the TestRail site to use.")
        parser.add_argument("--testrail-user",
                            nargs='?',
                            required=True,
                            help="REQUIRED: The user to log in as on TestRail for API calls.")
        parser.add_argument("--testrail-project-id",
                            required=True,
                            help="ID for the TestRail project")
        parser.add_argument("--testrail-password",
                            help="TestRail password for the --testrail-user arg passed."
                                 "REQUIRED: If not manually executing the script (i.e. automation)."
                                 "NOTE: Be sure to use a secured method of passing this parameter if you use it.")
        parser.add_argument("--testrun-name",
                            help="Name of the test run",
                            default="Automated TestRail Report")
        parser.add_argument("--logs-folder",
                            help="Sub-folder within ~/TestRailImporter/ to store .log "
                                 "files for python logging.",
                            default='reports')
        parser.add_argument("--testrun-id",
                            help="ID for the TestRail test run. "
                                 "This value takes priority over all other identifiers for a test run. "
                                 "If left blank, the TestRailImporter will create a new test run in the "
                                 "project ID specified (and suite ID if required).")
        parser.add_argument("--testrail-suite-id",
                            help="Suite ID for the TestRail project: required if no '--testrun-id' CLI arg "
                                 "is passed and a project has suites. Not required if a project lacks suites.")
        args = parser.parse_args()

        return args

    def project_suite_check(self, client, project_id):
        """
        Sends a request to the TestRail API to determine if a project has suites or not.
        Projects with suites need a suite ID specified in the request to add a new test run.
        :param client: TestRailConnection client object for communicating with the TestRail API.
        :param project_id: string representing the ID for a TestRail project to target for the suites check.
        :return: True if the project has suites and a '--testrail-suite-id' CLI arg value,
            otherwise raise a TestRailImporterError exception.
        """
        has_multiple_suites = None
        project_suites = client.get_suites(project_id)
        if project_suites:
            has_multiple_suites = len(project_suites) > 1

        if has_multiple_suites and not self._cli_args.testrail_suite_id:
            if self._cli_args.testrun_id:
                return True
            raise TestRailImporterError(
                "TestRailImporter requires the '--testrail-suite-id' CLI arg if the project has more than 1 suite and "
                "no '--testrun-id' CLI arg is passed.")
        return True

    def start_log(self, logs_folder):
        """
        Configures the root Logger for all modules with log objects to inherit from.
        :param logs_folder: string representing the logs folder name, which is passed in by
            the "--logs-folder=" CLI arg and created inside of the ~/TestRailImporter/ directory.
        """
        # Logging format variables.
        timestamp_format = '%Y-%m-%dT%H-%M-%S-%f'  # ISO with colon and period replaced to dash
        log_format_string = '%(asctime)-15s  %(message)s'
        now = datetime.datetime.now().strftime(timestamp_format)

        # Logging file & folder path variables.
        full_path = os.path.realpath(__file__)
        log_directory = os.path.join(os.path.dirname(full_path),
                                     '..',
                                     '..',
                                     logs_folder)
        log_filename = "testrail_importer_log_{}.log".format(now)
        if not os.path.isdir(log_directory):
            log.warn('TestRailImporter: Logging directory not created. '
                     'Creating in: {}'.format(log_directory))
            os.makedirs(log_directory)
        full_log_file_path = os.path.join(log_directory, log_filename)

        # Stream handler for console output.
        stream_handler = logging.StreamHandler()
        stream_handler.setFormatter(logging.Formatter(log_format_string))
        stream_handler.setLevel(logging.INFO)

        # File handler for saving to a log file.
        file_handler = logging.FileHandler(full_log_file_path)
        file_handler.setFormatter(logging.Formatter(log_format_string))
        file_handler.setLevel(logging.INFO)

        # Root logger other module-level loggers will inherit from.
        root_logger = logging.getLogger('')
        root_logger.setLevel(logging.INFO)
        root_logger.addHandler(stream_handler)
        root_logger.addHandler(file_handler)

    def testrail_client(self, url, user, password):
        """
        TestRailConnection object for making API requests to the TestRail API.
        :param url: string representing the TestRail URL to target.
            i.e.: 'https://testrail.yourspecialurl.com/'
        :param user: string representing the TestRail user to access the API with.
        :param password: string representing the password to go with the TestRail user parameter.
        :return testrail_connection.TestRailConnection client.
        """
        client = testrail_connection.TestRailConnection(
            url=url,
            user=user,
            password=password
        )

        return client

    def testrail_converter(self):
        """
        TestRailReporterConverter object for converting jUnitXML reports into
            a TestRail readable format for importing test results.
        :return: testrail_report_converter.TestRailReportConverter()
        """
        return testrail_report_converter.TestRailReportConverter()

    def main(self):
        """
        Main executor for converting and pushing .xml test results to TestRail using TestRailImporter.
        :return: None
        """
        # Configure logging & start root logger.
        self.start_log(self._cli_args.logs_folder)

        # Sort out the TestRail account password first, for security reasons.
        password = self._cli_args.testrail_password
        if not password:
            if not sys.stdin.isatty():
                raise TestRailImporterError(
                    "No --testrail-password CLI arg passed and no TTY input detected. "
                    "Please pass the required CLI arg or use a TTY console/terminal for manual input with "
                    "@echo off enabled.")
            log.warn('--testrail-password CLI arg not passed, but TTY input detected.')
            password = getpass.getpass(
                "Enter the TestRail Password for TestRail account {}: ".format(self._cli_args.testrail_user))

        # Connect the TestRailImporter to the TestRail API using TestRailConnection object.
        client = self.testrail_client(url=self._cli_args.testrail_url,
                                      user=self._cli_args.testrail_user,
                                      password=password)

        # Check if '--testrail-suite-id' is required.
        self.project_suite_check(client=client, project_id=self._cli_args.testrail_project_id)

        # Convert the jUnitXML report for TestRail
        converted_test_case_ids = []
        testrun_results = dict()
        test_report_results = self.testrail_converter().xml_to_list_of_dicts(
            self._cli_args.testrail_xml)

        # Determine which XML test case IDs exist on the targeted TestRail project.
        for result in test_report_results:
            converted_test_case_ids.extend(result.get('case_ids'))
        test_case_ids = set(converted_test_case_ids)
        testrail_test_cases = client.get_valid_test_case_ids(test_case_ids)

        # Create the test run & add tests to the test run.
        if not self._cli_args.testrun_id:
            self._cli_args.testrun_id = client.create_run(project_id=self._cli_args.testrail_project_id,
                                                          suite_id=self._cli_args.testrail_suite_id,
                                                          testrun_name=self._cli_args.testrun_name,
                                                          case_ids=testrail_test_cases)
        else:
            client.add_tests(testrun_id=self._cli_args.testrun_id,
                             case_ids=testrail_test_cases)
        tests_added_to_run = client.get_tests(self._cli_args.testrun_id)
        testrun_results['results'] = self.__filter_test_cases(test_report_results)
        testrun_results['testrun_id'] = self._cli_args.testrun_id

        # Parse for TestRail test case IDs detected & XML test case IDs collected.
        testrail_case_ids_detected = []
        xml_case_ids_collected = []
        if tests_added_to_run:
            for test in tests_added_to_run:
                testrail_case_ids_detected.append(test['case_id'])
        for result in testrun_results['results']:
            xml_case_ids_collected.append(result['case_id'])

        # Post test results to the created test run.
        log.info('TestRailImporter: Posting results to TestRail: {}'.format(testrun_results))
        try:
            client.post_test_results(testrun_results)
        except testrail_api_connector.TestRailAPIError:  # Report on mismatched XML & TestRail IDs.
            log.warning(
                'TestRailImporter: For test run ID "{}" - collected XML test case IDs: "{}" - '
                '& detected TestRail test case IDs: "{}"\n'
                'If you fail to see results on TestRail, is it probably due to these ID mismatches.'.format(
                    self._cli_args.testrun_id, xml_case_ids_collected, testrail_case_ids_detected))


if __name__ == '__main__':
    TestRailImporter().main()
