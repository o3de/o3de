"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Converts a jUnitXML test report .xml file into a list of dicts for updating TestRail test results.
"""

import logging
import os
import six
import xml.etree.ElementTree as ElementTree

from . import testrail_settings

ID_KEY = 'test_case_id'

log = logging.getLogger(__name__)


class TestRailReportConverterError(Exception):
    """Raised when a conversion or XML parsing error occurs with TestRailReportConverter."""

    pass


class TestRailReportConverter(object):
    """Converts XML files into reports for TestRail updating."""

    def _empty_strings_to_none(self, dictionary):
        """
        Converts empty-string values in a dictionary to None.
        Useful for avoiding DynamoDB ValidationExceptions.
        :param dictionary: dict containing string values.
        :return: dict with '' string values converted to None.
        """
        try:  # py2
            return {k: v if v != '' else None for k, v in dictionary.iteritems()}
        except AttributeError:  # py3
            return {k: v if v != '' else None for k, v in dictionary.items()}

    def _process_single_test_suite(self, test_suite_xml_node):
        """
        Takes a xml.etree.ElementTree object representing a TestRail
        test suite & parses it, then returns results a list of dicts
        for updating the TestRail API with results.
        :param test_suite_xml_node: xml.etree.ElementTree object
        :return: list of xml.etree.ElementTree.Element objects converted from test_suite_xml_node
        """
        test_cases_without_id_marker = []
        results = []
        suite_name = test_suite_xml_node.attrib.get("name", "unknown")
        test_cases = test_suite_xml_node.findall('testcase')

        for case_node in test_cases:
            name = case_node.attrib.get("name", "unnamed")
            time = case_node.attrib.get("time", "0")
            log_nodes = case_node.findall("system-out")
            log_text = ""
            properties = case_node.getchildren()

            if not properties:
                test_cases_without_id_marker.append(case_node)
            else:
                properties = case_node.getchildren()[0].getchildren()

            for ln in log_nodes:
                log_text += ln.text

            result = self._parse_test_status_ids(case_node)
            result['case_ids'] = self._parse_test_case_ids(properties, ID_KEY)
            result['custom_automated_test_name'] = name.split('[')[0]
            result['custom_elapse'] = str(time)
            result['log'] = log_text
            result['custom_runner'] = suite_name
            results.append(self._empty_strings_to_none(result))

        if test_cases_without_id_marker:
            test_cases_without_id_marker = self._parse_test_case_names(test_cases)
            log.warning(
                'TestRailReportConverter: <testcase> elements must contain a <properties> child element that holds '
                'multiple <property> elements. One <property> element needs the "name" attribute "{}"'.format(
                    ID_KEY))
            log.warning(
                'TestRailReportConverter: Skipped parsing for the following <testcase> elements since they lack the '
                '"{}" <property>: {}'.format(ID_KEY, test_cases_without_id_marker))

        return results

    def _parse_test_status_ids(self, case_node):
        """
        Takes an xml.etree.ElementTree.Element object representing a
        test case node & returns a dict with a 'status_id' key containing
        an int value matching a pass/fail status for the TestRail API.
        :param case_node: xml.etree.ElementTree.Element object with pass/fail test statuses
        :return: dict with pass/fail key 'status_id' populated with int values as well as
            additional items for fail results (pass results only return `status_id` value).
        """
        result = {}
        error_node = case_node.find("error")
        failure_nodes = case_node.findall("failure")
        skipped_node = case_node.find("skipped")  # pytest uses skipped elements.
        not_run_node = case_node.attrib.get("status") == 'notrun'  # GTest uses "notrun" status.

        if error_node or failure_nodes:  # Both treated as a failure on TestRail.
            result = self._parse_failure_result(error_node, failure_nodes)
        elif skipped_node or not_run_node:
            result['status_id'] = testrail_settings.TESTRAIL_STATUS_IDS['na_id']
        else:
            result['status_id'] = testrail_settings.TESTRAIL_STATUS_IDS['pass_id']

        return result

    def _parse_test_case_ids(self, properties, id_key):
        """
        Takes a list of xml.etree.ElementTree.Element objects and parses for the
        ID_KEY attribute value to match TestRail test case IDs to the report.
        :param properties: list of xml.etree.ElementTree.Element objects
        :param id_key: string representing the name of the key that holds
            the test case ID value in the .xml file.
        :return: list of ints containing TestRail test case IDs.
        """
        case_ids = []

        for property_node in properties:
            prop_name = property_node.attrib.get("name")
            if prop_name == id_key:
                # TestRail test case IDs preface is 'C':
                formatted_case_id = property_node.attrib.get('value').replace('C', '')
                case_ids.append(int(formatted_case_id))

        return case_ids

    def _parse_test_case_names(self, test_cases):
        """
        Takes a list of xml.etree.ElementTree.Element objects and parses for the "name" attribute, then adds them
        to a new list, which it returns when it finishes parsing.
        :param test_cases: list of xml.etree.ElementTree.Element objects
        :return: list of strings containing the "name" attribute for each test case in the test_cases list
        """
        test_case_names = []

        for test_case in test_cases:
            test_case_names.append(test_case.attrib.get("name"))

        return test_case_names

    def _parse_failure_result(self, error_node=None, failure_nodes=None):
        """
        Takes a failure test case result and returns a dict with
        keys & values matching the failure test case for TestRail.
        :param error_node: xml.etree.ElementTree.Element object
            representing a failed test case result.
        :param failure_nodes: list of xml.etree.ElementTree.Element
            objects representing failed test case results.
        :return: dict containing failure values for updating TestRail
        """
        result = dict()
        result['status_id'] = testrail_settings.TESTRAIL_STATUS_IDS['fail_id']

        if error_node:
            result['custom_test_error_message'] = error_node.attrib.get("message", "unknown")
            result['custom_error_text_location'] = error_node.text
        elif failure_nodes:
            if len(failure_nodes) > 1:  # Add header to the report when there are multiple failures.
                failure_message_lines = []
                failure_lines = []
                for count, failure in enumerate(failure_nodes):
                    header = '-------------------- Failure {} --------------------'.format(count + 1)
                    failure_message_lines.extend([header, '', failure.attrib.get("message"), ''])
                    failure_lines.extend([header, '', failure.text, ''])
                result['failure_message'] = '\n'.join(failure_message_lines)
                result['failure_text_location'] = '\n'.join(failure_lines)
            else:  # Do not add a header to the report when there is 1 failure.
                result['custom_test_failure_message'] = failure_nodes[0].attrib.get("message", "unknown")
                result['custom_failure_text_location'] = failure_nodes[0].text

        return result

    def xml_to_list_of_dicts(self, xml_file):
        """
        Takes an XML file and converts it into a list of dicts for updating
            TestRail test results using a JSON payload.
        :param xml_file: str path to the .xml report file to convert.
        :return: list of dicts containing converted values for TestRail API json payload
        """
        try:
            tree = ElementTree.parse(os.path.abspath(xml_file))
        except ElementTree.ParseError as error:
            error_line = error.position[0]
            error_column = error.position[1]
            problem = TestRailReportConverterError(
                'XML file failed to be parsed - error occurred on line "{}" & column "{}"\n'
                'Please review the XML file for this line & column: {}'.format(error_line, error_column, xml_file))
            six.raise_from(problem, error)
        root = tree.getroot()
        test_suites = root.findall('testsuite')
        test_report_results = []

        if test_suites:  # root is <testsuites>
            test_suite_xml_nodes = test_suites
        else:  # root is <testsuite>
            test_suite_xml_nodes = [root]

        for test_suite in test_suite_xml_nodes:
            test_report_results.extend(self._process_single_test_suite(test_suite))

        return test_report_results
