"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ~/TestRailImporter/lib/testrail_importer/testrail_importer.py
"""
try:  # py2
    import mock
except ImportError:  # py3
    import unittest.mock as mock
try:  # py2
    import __builtin__
    builtin_module = '__builtin__'
except ImportError:  #py3
    import builtins
    builtin_module = 'builtins'
import argparse
import os
import pytest
import sys
import lib.testrail_importer.testrail_importer as testrail_importer


DEFAULT_ARGS = [
    {'name': "--testrail-xml",
     'nargs': '?',
     'required': True,
     'help': 'help text',
     },
    {'name': "--testrail-url",
     'nargs': '?',
     'required': True,
     'help': 'help text',
     },
    {'name': "--testrail-user",
     'nargs': '?',
     'required': True,
     'help': 'help text',
     },
    {'name': "--testrail-project-id",
     'required': True,
     'help': 'help text',
     },
    {'name': "--testrail-password",
     'help': 'help text',
     },
    {'name': "--testrun-name",
     'help': 'help text',
     'default': 'Automated TestRail Report'
     },
    {'name': "--logs-folder",
     'help': 'help text',
     'default': 'reports'
     },
    {'name': "--testrun-id",
     'help': 'help text'
     },
    {'name': "--testrail-suite-id",
     'help': 'help text',
     },

]

DEFAULT_LAUNCH_CMD = ['--testrail-xml', 'xml-path',
                      '--testrail-url', 'https://url-string.dummyurl',
                      '--testrail-user', 'user@email.email',
                      '--testrail-project-id', '1234567890']


def create_parser(arg_values):
    parser = argparse.ArgumentParser(description="Mocked")

    for value in arg_values:
        parser.add_argument(value.get('name', None), nargs=value.get('nargs', None),
                            required=value.get('required', None), help=value.get('help', None),
                            default=value.get('default', None), )

    return parser


class MockedTTY(object):
    """Python2 cannot mock sys.stdin.isatty(), so this proxy object is required."""
    def __init__(self, underyling, tty_status):
        self.__underlying = underyling
        self.tty_status = tty_status

    def __getattr__(self, name):
        return getattr(self.__underlying, name)

    def isatty(self):
        return self.tty_status


@mock.patch('os.path.realpath', mock.MagicMock(return_value='dummy_realpath'))
@mock.patch('os.path.dirname', mock.MagicMock(return_value='dummy_dirname'))
@mock.patch("{}.open".format(builtin_module), mock.mock_open())
@pytest.mark.unit
class TestImporter:

    @mock.patch('lib.testrail_importer.testrail_importer.TestRailImporter.cli_args')
    def test_Init_ValidArgParse_SetsCliArgs(self, mock_cli_args):
        mock_parser = create_parser(DEFAULT_ARGS)
        mock_args = mock_parser.parse_args(DEFAULT_LAUNCH_CMD)
        mock_cli_args.return_value = mock_args

        under_test = testrail_importer.TestRailImporter()

        assert under_test._cli_args == mock_args

    @mock.patch('lib.testrail_importer.testrail_importer.TestRailImporter.cli_args')
    def test_FilterTestCases_HasCaseIDs_ReturnsList(self, mock_cli_args):
        mock_parser = create_parser(DEFAULT_ARGS)
        mock_args = mock_parser.parse_args(DEFAULT_LAUNCH_CMD)
        mock_cli_args.return_value = mock_args
        testrun_results = [{'case_ids': [12345]}, {'case_ids': [11111]}]

        under_test = testrail_importer.TestRailImporter()._TestRailImporter__filter_test_cases(
            testrun_results)

        assert under_test == [{'case_id': 12345, 'case_ids': [12345]},
                              {'case_id': 11111, 'case_ids': [11111]}]

    @mock.patch('lib.testrail_importer.testrail_importer.TestRailImporter.cli_args')
    def test_FilterTestCaseIDs_HasCaseIDs_ReturnsListWithValues(self, mock_cli_args):
        mock_parser = create_parser(DEFAULT_ARGS)
        mock_args = mock_parser.parse_args(DEFAULT_LAUNCH_CMD)
        mock_cli_args.return_value = mock_args
        test_result = {'random_key_from_xml': 'test', 'case_ids': [12345, 11111]}

        under_test = testrail_importer.TestRailImporter()._TestRailImporter__filter_test_case_ids(
            test_result)

        assert under_test == [
            {'random_key_from_xml': 'test', 'case_id': 12345, 'case_ids': [12345, 11111]},
            {'random_key_from_xml': 'test', 'case_id': 11111, 'case_ids': [12345, 11111]}
        ]

    @mock.patch('lib.testrail_importer.testrail_importer.TestRailImporter.cli_args')
    def test_FilterTestCaseIDs_NoCaseIDs_ReturnsEmptyList(self, mock_cli_args):
        mock_parser = create_parser(DEFAULT_ARGS)
        mock_args = mock_parser.parse_args(DEFAULT_LAUNCH_CMD)
        mock_cli_args.return_value = mock_args
        test_result = {'random_key_from_xml': 'test', 'some_other_key': 'something'}

        under_test = testrail_importer.TestRailImporter()._TestRailImporter__filter_test_case_ids(
            test_result)

        assert under_test == []

    @mock.patch('lib.testrail_importer.testrail_importer.argparse.ArgumentParser')
    def test_CliArgs_AllCLIArgs_ReturnsParsedArgs(self, mock_arg_parser):
        expected_attribute_values = {
            'testrail_xml': 'xml-path',
            'testrail_url': 'https://url.url.asdf',
            'testrail_user': 'user@email.email',
            'testrail_project_id': '1234567890',
            'testrail_password': 'always_censor_me',
            'testrun_name': 'Test Run Name',
            'logs_folder': 'dummy_name',
            'testrun_id': '11111111111',
            'testrail_suite_id': '1212121212',
        }
        mock_arg_parser.return_value = mock.MagicMock()
        for mock_attribute in expected_attribute_values.keys():
            mock_arg_parser.add_argument.side_effect = [
                setattr(mock_arg_parser, mock_attribute, expected_attribute_values[mock_attribute]),
                setattr(mock_arg_parser, mock_attribute, expected_attribute_values[mock_attribute]),
                setattr(mock_arg_parser, mock_attribute, expected_attribute_values[mock_attribute]),
                setattr(mock_arg_parser, mock_attribute, expected_attribute_values[mock_attribute]),
                setattr(mock_arg_parser, mock_attribute, expected_attribute_values[mock_attribute]),
                setattr(mock_arg_parser, mock_attribute, expected_attribute_values[mock_attribute]),
                setattr(mock_arg_parser, mock_attribute, expected_attribute_values[mock_attribute]),
                setattr(mock_arg_parser, mock_attribute, expected_attribute_values[mock_attribute]),
                setattr(mock_arg_parser, mock_attribute, expected_attribute_values[mock_attribute])
            ]

        testrail_importer.TestRailImporter()  # cli_args() called in __init__()

        for expected_attribute in expected_attribute_values.keys():
            assert getattr(mock_arg_parser, expected_attribute) == expected_attribute_values[expected_attribute]

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection')
    @mock.patch('lib.testrail_importer.testrail_importer.TestRailImporter.cli_args')
    def test_ProjectSuiteCheck_HasSuitesHasSuiteIDHasRunID_ReturnsTrue(self, mock_cli_args, mock_testrail_connection):
        mock_testrail_connection.get_suites.return_value = [{u'id': 123}, {u'id': 456}]
        mock_parser = create_parser(DEFAULT_ARGS)
        new_launch_cmd = []
        new_launch_cmd.extend(DEFAULT_LAUNCH_CMD)
        new_launch_cmd.extend(['--testrail-suite-id', '123'])
        mock_args = mock_parser.parse_args(new_launch_cmd)
        mock_cli_args.return_value = mock_args

        under_test = testrail_importer.TestRailImporter().project_suite_check(mock_testrail_connection, '1')

        assert under_test is True

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection')
    @mock.patch('lib.testrail_importer.testrail_importer.TestRailImporter.cli_args')
    def test_ProjectSuiteCheck_HasSuitesNoSuiteIDNoRunID_RaisesTestRailImporterError(self, mock_cli_args,
                                                                                     mock_testrail_connection):
        mock_testrail_connection.get_suites.return_value = [{u'id': 123}, {u'id': 456}]
        mock_parser = create_parser(DEFAULT_ARGS)
        mock_args = mock_parser.parse_args(DEFAULT_LAUNCH_CMD)
        mock_cli_args.return_value = mock_args

        with pytest.raises(testrail_importer.TestRailImporterError):
            testrail_importer.TestRailImporter().project_suite_check(mock_testrail_connection, '1')

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection')
    @mock.patch('lib.testrail_importer.testrail_importer.TestRailImporter.cli_args')
    def test_ProjectSuiteCheck_HasSuitesNoSuiteIDHasRunID_ReturnsTrue(self, mock_cli_args, mock_testrail_connection):
        mock_testrail_connection.get_suites.return_value = []
        mock_parser = create_parser(DEFAULT_ARGS)
        new_launch_cmd = []
        new_launch_cmd.extend(DEFAULT_LAUNCH_CMD)
        new_launch_cmd.extend(['--testrun-id', '123'])
        mock_args = mock_parser.parse_args(new_launch_cmd)
        mock_cli_args.return_value = mock_args

        under_test = testrail_importer.TestRailImporter().project_suite_check(mock_testrail_connection, '1')

        assert under_test is True

    @mock.patch('os.path.isdir')
    @mock.patch('lib.testrail_importer.testrail_importer.logging')
    @mock.patch('lib.testrail_importer.testrail_importer.TestRailImporter.cli_args')
    def test_StartLog_HasLogDirectory_SetsLogging(self, mock_cli_args, mock_logging, mock_isdir):
        mock_parser = create_parser(DEFAULT_ARGS)
        mock_args = mock_parser.parse_args(DEFAULT_LAUNCH_CMD)
        mock_cli_args.return_value = mock_args
        logs_folder = 'dummy_logs_folder'
        mock_isdir.return_value = True

        testrail_importer.TestRailImporter().start_log(logs_folder)

        assert mock_logging.Formatter.call_count == 2
        mock_logging.FileHandler.assert_called_once()
        mock_logging.StreamHandler.assert_called_once()

    @mock.patch('lib.testrail_importer.testrail_importer.logging', mock.MagicMock())
    @mock.patch('os.makedirs')
    @mock.patch('os.path.isdir')
    @mock.patch('lib.testrail_importer.testrail_importer.TestRailImporter.cli_args')
    def test_StartLog_NoLogDirectory_CreatesLogDirectorySetsLogging(self, mock_cli_args, mock_isdir, mock_makedirs):
        mock_parser = create_parser(DEFAULT_ARGS)
        mock_args = mock_parser.parse_args(DEFAULT_LAUNCH_CMD)
        mock_cli_args.return_value = mock_args
        logs_folder = 'dummy_logs_folder'
        mock_isdir.return_value = False

        testrail_importer.TestRailImporter().start_log(logs_folder)

        mock_makedirs.assert_called_with(
            os.path.join(
                'dummy_dirname',
                '..',
                '..',
                'dummy_logs_folder'))

    @mock.patch(
        'lib.testrail_importer.testrail_tools.testrail_report_converter.TestRailReportConverter', mock.MagicMock())
    @mock.patch('getpass.getpass', mock.MagicMock(return_value='password'))
    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection.TestRailConnection')
    @mock.patch('lib.testrail_importer.testrail_importer.TestRailImporter.project_suite_check')
    @mock.patch('lib.testrail_importer.testrail_importer.TestRailImporter.cli_args')
    def test_Main_HasTTYNoID_PostsTestResults(self, mock_cli_args, mock_suite_check, mock_testrail_connection):
        sys.stdin = MockedTTY(sys.stdin, True)
        mock_parser = create_parser(DEFAULT_ARGS)
        mock_args = mock_parser.parse_args(DEFAULT_LAUNCH_CMD)
        mock_cli_args.return_value = mock_args

        testrail_importer.TestRailImporter().main()

        mock_testrail_connection.assert_called_with(password='password',
                                                    url='https://url-string.dummyurl',
                                                    user='user@email.email')
        mock_suite_check.assert_called_once()

        sys.stdin = sys.stdin
