"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ~/lib/testrail_importer/testrail_tools/testrail_connection.py
"""

try:  # py2
    import mock
except ImportError:  # py3
    import unittest.mock as mock
import pytest
import unittest

import lib.testrail_importer.testrail_tools.testrail_connection as testrail_connection


DEFAULTS = {'url': 'https://dummyurl.dummyurl', 'user': 'user@user.user', 'password': 'censored'}


@mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector', mock.MagicMock())
@pytest.mark.unit
class TestConnection(unittest.TestCase):

    def setUp(self):
        self.mock_testrail_connection = testrail_connection.TestRailConnection(
            url=DEFAULTS['url'],
            user=DEFAULTS['user'],
            password=DEFAULTS['password']
        )
        self.project_id = '1'
        self.testrun_id = '12345'
        self.suite_id = '100'

    def test_Init_HasRequiredParams_ConnectsTestRailAPI(self):
        assert self.mock_testrail_connection.client.user == DEFAULTS['user']
        assert self.mock_testrail_connection.client.password == DEFAULTS['password']

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_post')
    def test_CreateRun_CalledWithAllParams_ReturnsTestRunID(self, mock_send_post):
        mock_send_post.return_value = {'id': '12345', 'url': 'https://dummy-url.dummy'}
        under_test = self.mock_testrail_connection.create_run(project_id=self.project_id,
                                                              testrun_name='dummy_name',
                                                              suite_id='1',
                                                              milestone_id='1',
                                                              assigned_to='1',
                                                              include_all=True,
                                                              case_ids=[1, 2, 3])

        assert under_test == '12345'
        assert mock_send_post.call_args_list == [
            mock.call('add_run/1', {
                'include_all': True,
                'name': 'dummy_name',
                'case_ids': [1, 2, 3],
                'suite_id': '1',
                'assigned_to': '1',
                'milestone_id': '1'})
        ]

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_post')
    def test_CreateRun_NoCaseIds_ReturnsTestRunID(self, mock_send_post):
        mock_send_post.return_value = {'id': '12345', 'url': 'https://dummy-url.dummy'}
        under_test = self.mock_testrail_connection.create_run(project_id=self.project_id,
                                                              testrun_name='dummy_name')

        assert under_test == '12345'

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection.len')
    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_get')
    def test_GetRunList_Over250Runs_ReturnsRunsList(self, mock_send_get, mock_len):
        mock_len.side_effect = [251, 250, 0]  # Spoofs the run list len() being >= 250
        mock_send_get.side_effect = [
            [{'failed_count': 1, 'id': 1, 'name': 'Test run 1', 'passed_count': 5},
             {'failed_count': 5, 'id': 2, 'name': 'Test run 2', 'passed_count': 1}],
            [{'failed_count': 0, 'id': 3, 'name': 'Test run 3', 'passed_count': 100},
             {'failed_count': 10, 'id': 4, 'name': 'Test run 4', 'passed_count': 50}],
            []
        ]
        under_test = self.mock_testrail_connection.get_run_list(project_id=self.project_id)

        assert under_test == [
            {'passed_count': 5, 'failed_count': 1, 'id': 1, 'name': 'Test run 1'},
            {'passed_count': 1, 'failed_count': 5, 'id': 2, 'name': 'Test run 2'},
            {'passed_count': 100, 'failed_count': 0, 'id': 3, 'name': 'Test run 3'},
            {'passed_count': 50, 'failed_count': 10, 'id': 4, 'name': 'Test run 4'}
        ]
        assert mock_send_get.call_args_list == [
            mock.call('get_runs/1&offset=0'),
            mock.call('get_runs/1&offset=250'),
            mock.call('get_runs/1&offset=500')
        ]

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection.len')
    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_get')
    def test_GetPlanList_Over250Plans_ReturnsPlansList(self, mock_send_get, mock_len):
        mock_len.side_effect = [251, 250, 0]  # Spoofs the plan list len() being >= 250
        mock_send_get.side_effect = [
            [{'failed_count': 0, 'id': 10, 'name': 'Test Plan 1', 'passed_count': 100},
             {'failed_count': 0, 'id': 20, 'name': 'Test Plan 2', 'passed_count': 200}],
            [{'failed_count': 0, 'id': 30, 'name': 'Test Plan 3', 'passed_count': 300},
             {'failed_count': 0, 'id': 40, 'name': 'Test Plan 4', 'passed_count': 400}],
            []
        ]
        under_test = self.mock_testrail_connection.get_plan_list(project_id=self.project_id)

        assert under_test == [
            {'passed_count': 100, 'failed_count': 0, 'id': 10, 'name': 'Test Plan 1'},
            {'passed_count': 200, 'failed_count': 0, 'id': 20, 'name': 'Test Plan 2'},
            {'passed_count': 300, 'failed_count': 0, 'id': 30, 'name': 'Test Plan 3'},
            {'passed_count': 400, 'failed_count': 0, 'id': 40, 'name': 'Test Plan 4'}
        ]
        assert mock_send_get.call_args_list == [
            mock.call('get_plans/1&offset=0'),
            mock.call('get_plans/1&offset=250'),
            mock.call('get_plans/1&offset=500')
        ]

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_get')
    def test_GetRunData_GetReturnsRun_ReturnsDict(self, mock_send_get):
        expected_run_data = {'passed_count': 5, 'failed_count': 1, 'id': 1, 'name': 'Test run 1'}
        mock_send_get.return_value = expected_run_data
        under_test = self.mock_testrail_connection.get_run_data(testrun_id=self.testrun_id)

        assert under_test == expected_run_data
        mock_send_get.assert_called_with('get_run/{}'.format(self.testrun_id))

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_get')
    def test_GetTests_GetReturnsTests_ReturnsList(self, mock_send_get):
        mock_send_get.return_value = [
            {'case_id': 1, 'id': 1, 'run_id': 1},
            {'case_id': 2, 'id': 2, 'run_id': 2},
            {'case_id': 3, 'id': 3, 'run_id': 3}
        ]
        under_test = self.mock_testrail_connection.get_tests(testrun_id=self.testrun_id)

        assert under_test == [
            {'case_id': 1, 'id': 1, 'run_id': 1},
            {'case_id': 2, 'id': 2, 'run_id': 2},
            {'case_id': 3, 'id': 3, 'run_id': 3}
        ]
        mock_send_get.assert_called_with('get_tests/{}'.format(self.testrun_id))

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection.TestRailConnection.get_tests')
    def test_BuildCaseIDList_GetReturnsTests_ReturnsList(self, mock_get_tests):
        expected_case_ids = []
        for test in [
            {'case_id': 1, 'id': 1, 'run_id': 1},
            {'case_id': 2, 'id': 2, 'run_id': 2},
            {'case_id': 3, 'id': 3, 'run_id': 3}
        ]:
            expected_case_ids.append(test['case_id'])
        mock_get_tests.return_value = [
            {'case_id': 1, 'id': 1, 'run_id': 1},
            {'case_id': 2, 'id': 2, 'run_id': 2},
            {'case_id': 3, 'id': 3, 'run_id': 3}
        ]
        under_test = self.mock_testrail_connection.build_case_id_list(testrun_id='1')

        assert under_test == expected_case_ids

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_post')
    def test_UpdateTests_PostReturnsDict_ReturnsDict(self, mock_send_post):
        mock_send_post.return_value = [
            {'failed_count': 1, 'id': 1, 'name': 'Test run 1', 'passed_count': 5},
            {'failed_count': 5, 'id': 2, 'name': 'Test run 2', 'passed_count': 1}
        ]

        under_test = self.mock_testrail_connection.update_tests(testrun_id='1')

        assert under_test == [
            {'passed_count': 5, 'failed_count': 1, 'id': 1, 'name': 'Test run 1'},
            {'passed_count': 1, 'failed_count': 5, 'id': 2, 'name': 'Test run 2'}
        ]

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection.TestRailConnection.get_tests')
    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection.TestRailConnection.update_tests')
    def test_AddTests_NoExistingCases_ReturnsCaseIDList(self, mock_update_tests, mock_get_tests):
        mock_update_tests.return_value = [
            {'case_id': 1, 'id': 1, 'run_id': 1},
            {'case_id': 2, 'id': 2, 'run_id': 2},
            {'case_id': 3, 'id': 3, 'run_id': 3},
            {'case_id': 4, 'id': 4, 'run_id': 4}
        ]
        mock_get_tests.return_value = [
            {'case_id': 1, 'id': 1, 'run_id': 1},
            {'case_id': 2, 'id': 2, 'run_id': 2},
            {'case_id': 3, 'id': 3, 'run_id': 3}
        ]

        under_test = self.mock_testrail_connection.add_tests(testrun_id='1',
                                                             case_ids=[1, 2, 3, 4, 5])

        assert under_test == [1, 2, 3, 4, 5]

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection.TestRailConnection.get_tests')
    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection.TestRailConnection.update_tests')
    def test_AddTests_HasExistingCases_ReturnsCaseIDList(self, mock_update_tests, mock_get_tests):
        mock_update_tests.return_value = [
            {'case_id': 1, 'id': 1, 'run_id': 1},
            {'case_id': 2, 'id': 2, 'run_id': 2},
            {'case_id': 3, 'id': 3, 'run_id': 3},
            {'case_id': 4, 'id': 4, 'run_id': 4}
        ]
        mock_get_tests.return_value = [
            {'case_id': 1, 'id': 1, 'run_id': 1},
            {'case_id': 2, 'id': 2, 'run_id': 2},
            {'case_id': 3, 'id': 3, 'run_id': 3}
        ]

        under_test = self.mock_testrail_connection.add_tests(testrun_id='1',
                                                             case_ids=[1, 2, 3, 4, 5, 6])

        assert under_test == [1, 2, 3, 4, 5, 6]

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection.TestRailConnection.get_tests')
    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection.TestRailConnection.update_tests')
    def test_RemoveTests_NoExistingCases_ReturnsCaseIDList(self, mock_update_tests, mock_get_tests):
        mock_update_tests.return_value = [
            {'case_id': 1, 'id': 1, 'run_id': 1},
            {'case_id': 2, 'id': 2, 'run_id': 2},
        ],
        mock_get_tests.return_value = [
            {'case_id': 1, 'id': 1, 'run_id': 1},
            {'case_id': 2, 'id': 2, 'run_id': 2},
            {'case_id': 3, 'id': 3, 'run_id': 3}
        ]

        under_test = self.mock_testrail_connection.remove_tests(testrun_id='1',
                                                                case_ids=[3])

        assert under_test == [1, 2]

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_connection.TestRailConnection.update_tests')
    def test_RemoveTests_HasExistingCases_ReturnsCaseIDList(self, mock_update_tests):
        mock_update_tests.return_value = [
            {'case_id': 1, 'id': 1, 'run_id': 1},
        ]

        under_test = self.mock_testrail_connection.remove_tests(testrun_id='1',
                                                                case_ids=[2, 3],
                                                                existing_cases=[1, 2, 3])

        assert under_test == [1]

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_post')
    def test_PostTestResults_HasRunResults_ReturnsListOfDicts(self, mock_send_post):
        mock_send_post.return_value = [
            {'assignedto_id': 1,
             'comment': 'This test failed: test_UnitTested_TestConditions_TestResults',
             'id': 1,
             'test_id': 1},
            {'assignedto_id': 2,
             'comment': 'This test failed: test_UnitTested_TestConditions_TestResults',
             'id': 2,
             'test_id': 2},
        ]
        dummy_run_results = {
            'results': [
                {'case_id': 1, 'status_id': 1, 'case_ids': [1]},
                {'case_id': 2, 'status_id': 1, 'case_ids': [2]},
            ],
            'testrun_id': 100
        }

        under_test = self.mock_testrail_connection.post_test_results(dummy_run_results)

        assert under_test == [
            {'assignedto_id': 1,
             'comment': 'This test failed: test_UnitTested_TestConditions_TestResults',
             'id': 1,
             'test_id': 1},
            {'assignedto_id': 2,
             'comment': 'This test failed: test_UnitTested_TestConditions_TestResults',
             'id': 2,
             'test_id': 2},
        ]
        mock_send_post.assert_called_with('add_results_for_cases/100', {'results': dummy_run_results['results']})

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_get')
    def test_GetRunResults_HasRunResults_ReturnsListOfDicts(self, mock_send_get):
        mock_send_get.return_value = [
            {'assignedto_id': 1,
             'comment': 'This test failed: test_UnitTested_TestConditions_TestResults',
             'id': 1,
             'test_id': 1},
            {'assignedto_id': 2,
             'comment': 'This test failed: test_UnitTested_TestConditions_TestResults',
             'id': 2,
             'test_id': 2},
        ]
        under_test = self.mock_testrail_connection.get_run_results(testrun_id=self.testrun_id)

        assert under_test == [
            {'assignedto_id': 1,
             'comment': 'This test failed: test_UnitTested_TestConditions_TestResults',
             'id': 1,
             'test_id': 1},
            {'assignedto_id': 2,
             'comment': 'This test failed: test_UnitTested_TestConditions_TestResults',
             'id': 2,
             'test_id': 2},
        ]
        mock_send_get.assert_called_with('get_results_for_run/{}'.format(self.testrun_id))

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_post')
    def test_CloseRun_ValidPostRequest_CallSucceeds(self, mock_send_post):
        self.mock_testrail_connection.close_run(testrun_id=self.testrun_id)

        mock_send_post.assert_called_with('close_run/{}'.format(self.testrun_id), {})

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_post')
    def test_DeleteRun_ValidPostRequest_CallSucceeds(self, mock_send_post):
        self.mock_testrail_connection.delete_run(testrun_id=self.testrun_id)

        mock_send_post.assert_called_with('delete_run/{}'.format(self.testrun_id), {})

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_get')
    def test_GetProjects_ValidPostRequest_CallSucceeds(self, mock_send_get):
        self.mock_testrail_connection.get_projects()

        mock_send_get.assert_called_with('get_projects/')

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_get')
    def test_GetCases_NoSuiteID_ReturnsListOfDicts(self, mock_send_get):
        mock_send_get.return_value = [
            {'title': 'dummy_title1', 'id': 123, 'type_id': 50, 'section_id': 5},
            {'title': 'dummy_title2', 'id': 124, 'type_id': 50, 'section_id': 5},
        ]

        under_test = self.mock_testrail_connection.get_cases(project_id=self.project_id)

        assert under_test == [
            {'title': 'dummy_title1', 'id': 123, 'type_id': 50, 'section_id': 5},
            {'title': 'dummy_title2', 'id': 124, 'type_id': 50, 'section_id': 5},
        ]
        mock_send_get.assert_called_with('get_cases/{}'.format(self.project_id))

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_get')
    def test_GetCases_HasSuiteID_ReturnsListOfDicts(self, mock_send_get):
        mock_send_get.return_value = [
            {'title': 'dummy_title1', 'id': 123, 'type_id': 50, 'section_id': 5},
            {'title': 'dummy_title2', 'id': 124, 'type_id': 50, 'section_id': 5},
        ]

        under_test = self.mock_testrail_connection.get_cases(project_id=self.project_id,
                                                             suite_id=self.suite_id)

        assert under_test == [
            {'title': 'dummy_title1', 'id': 123, 'type_id': 50, 'section_id': 5},
            {'title': 'dummy_title2', 'id': 124, 'type_id': 50, 'section_id': 5},
        ]
        mock_send_get.assert_called_with('get_cases/{}&suite_id={}'.format(
            self.project_id, self.suite_id))

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_post')
    def test_CloseTestRun_ValidPostRequest_CallSucceeds(self, mock_send_post):
        self.mock_testrail_connection.close_test_run(testrun_id=self.testrun_id)

        mock_send_post.assert_called_with('close_run/{}'.format(self.testrun_id), {})

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_get')
    def test_GetSuites_ValidGetRequest_CallSucceeds(self, mock_send_get):
        self.mock_testrail_connection.get_suites(project_id=self.project_id)

        mock_send_get.assert_called_with('get_suites/{}'.format(self.project_id))

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector.send_get')
    def test_GetCase_HasCaseID_ReturnsDict(self, mock_send_get):
        mock_send_get.return_value = {
            'type_id': 0,
            'refs': None,
            'priority_id': 0,
            'created_on': 0,
            'milestone_id': None,
            'title': 'foo bar',
            'created_by': 0,
            'id': 0,
            'custom_expected': None,
            'custom_steps': None,
            'custom_is_obsolete': False,
            'updated_by': 0,
            'custom_steps_separated': None,
            'section_id': 0,
            'custom_preconds': None,
            'suite_id': 0,
            'estimate_forecast': None,
            'custom_internal': False,
            'estimate': None,
            'custom_status': 0,
            'custom_field_notes': None,
            'updated_on': 0,
            'template_id': 0
        }
        under_test = self.mock_testrail_connection.get_case(case_id='1')

        assert under_test == mock_send_get.return_value
