"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging

from . import testrail_api_connector

log = logging.getLogger(__name__)


class TestRailConnection(object):
    """Wrapper for TestRailApiConnector object."""

    def __init__(self, url, user, password):
        self.client = testrail_api_connector.TestRailApiConnector(url, user, password)
        self.client.user = user
        self.client.password = password

    def create_run(self,
                   project_id,
                   testrun_name,
                   suite_id=None,
                   milestone_id=None,
                   assigned_to=None,
                   include_all=False,
                   case_ids=None):
        """
        Creates a new Test Run with the parameters provided.
        :param project_id: Id of the Project to which the run will be posted.
        :param testrun_name: The name assigned to the test run.
        :param suite_id: Id of the project suite for the test run.
            Required if the project listed uses multiple test suites.
        :param milestone_id: Id of the milestone the test run is posted against.
        :param assigned_to: TestRail ID of the user to which the run is assigned.
        :param include_all: If True - Includes all test cases in the current project/suite to the test run.
        :param case_ids: A list of test case Ids to be included in the test run.
        :return: The newly created Test Run ID as a string.
        """
        log.info('TestRailConnection: Creating new TestRail test run with info: '
                 'project_id: {} | '
                 'testrun_name: {} | '
                 'suite_id: {} | '
                 'milestone_id: {} | '
                 'assigned_to: {} | '
                 'include_all: {} | '
                 'case_ids: {} | '.format(project_id,
                                          testrun_name,
                                          suite_id,
                                          milestone_id,
                                          assigned_to,
                                          include_all,
                                          case_ids))
        if case_ids is None:
            case_ids = []

        run = self.client.send_post(
            "add_run/{}".format(project_id),
            {"suite_id": suite_id,
             "milestone_id": milestone_id,
             "name": str(testrun_name),
             "assigned_to": assigned_to,
             "include_all": include_all,
             "case_ids": case_ids})
        log.info("TestRailConnection: Test Run ID #{} created - URL: {}".format(
            run["id"], run["url"]))

        return run["id"]

    def get_run_list(self, project_id):
        """
        Used to generate a list of test runs for the selected project.
        Api return limit is 250 results per get.
        This function will repeat gets until all test runs and plans until all results are returned.
        :param project_id: Id of the project containing the test runs to be returned.
        :return: A list of all test runs and test plans for the specified project.
        """
        offset = 0
        api_return_limit = 250
        run_count_done = False
        runs = []

        while not run_count_done:
            run_iteration = self.client.send_get("get_runs/{}&offset={}".format(project_id, offset))

            if run_iteration:
                for run in run_iteration:
                    runs.append(run)

            if run_iteration and len(run_iteration) >= api_return_limit:
                offset += api_return_limit
            else:
                offset = 0
                run_count_done = True

        return runs

    def get_plan_list(self, project_id):
        """
        Used to generate a list of test plans for the selected project.
        Api return limit is 250 results per get.
        This function will repeat gets until all test runs and plans until all results are returned.
        :param project_id: Id of the project containing the test runs to be returned.
        :return: A list of all test runs and test plans for the specified project.
        """
        offset = 0
        api_return_limit = 250
        plan_count_done = False
        plans = []

        while not plan_count_done:
            plan_iteration = self.client.send_get("get_plans/{}&offset={}".format(project_id, offset))

            if plan_iteration:
                for plan in plan_iteration:
                    plans.append(plan)

            if plan_iteration and len(plan_iteration) >= api_return_limit:
                offset += api_return_limit
            else:
                offset = 0
                plan_count_done = True

        return plans

    def get_run_data(self, testrun_id):
        """
        Used to generate a dictionary of all values and statistics for a test run.
        :param testrun_id: Id of the test run to be returned.
        :return: A dictionary of values and statistics for the run.
        """
        return self.client.send_get('get_run/{}'.format(testrun_id))

    def get_tests(self, testrun_id):
        """
        Used to generate a list of all tests present in a test run.
        :param testrun_id: Id of the test run containing the tests to be returned.
        :return: A list of dictionaries containing information for every test case in the test run.
        """
        return self.client.send_get("get_tests/{}".format(testrun_id))

    def build_case_id_list(self, testrun_id):
        """
        Used to generate a list of test case ids for all tests present in a test run.
        :param testrun_id: Id of the test run containing the tests to be returned.
        :return: A list of test case Ids for all the tests in the test run.
        """
        current_case_ids = []
        current_tests = self.get_tests(testrun_id)

        for test in current_tests:
            current_case_ids.append(test["case_id"])

        return current_case_ids

    def update_tests(self, testrun_id, include_all=False, case_ids=None):
        """
        Updates a test run to contain the test cases specified.
        When passing in a list of case ids, existing tests must be included or they will be removed from the run.
        Existing test cases with results will not be modified when updating the test run.
        :param testrun_id: Id of the test run to be modified.
        :param include_all: If True, includes all test cases in the runs project/suite.
        :param case_ids: A list of test case ids to be included in the test run.
        """
        return self.client.send_post(
            "update_run/{}".format(testrun_id),
            {"include_all": include_all,
             "case_ids": case_ids})

    def add_tests(self, testrun_id, case_ids):
        """
        Adds test cases to an existing test run.
        If a list of test cases (or blank list) is not supplied, will perform a get to build the existing list.
        :param testrun_id: Id of the test run to be modified.
        :param case_ids: Accepts a list of test cases to add to the test run.
        :return: A list of test case Ids in the test run after the addition of the new cases.
        """
        case_list = self.build_case_id_list(testrun_id)

        for case_id in case_ids:
            if case_id not in case_list:
                case_list.append(case_id)
        self.update_tests(testrun_id, case_ids=case_list)

        return case_list

    def remove_tests(self, testrun_id, case_ids, existing_cases=None):
        """
        Removes test cases from an existing test run.
        If a list of test cases (or blank list) is not supplied, will perform a get to build the existing list.
        :param testrun_id: Id of the test run to be modified.
        :param case_ids: Accepts a list of test cases to remove from the test run.
        :param existing_cases: Accepts a list of test case ids (or blank list) that are already present in a test run.
        :return: A list of test case Ids in the test run after the removal of cases.
        """
        if existing_cases:
            case_list = existing_cases
        else:
            case_list = self.build_case_id_list(testrun_id)

        for case_id in case_ids:
            case_list.remove(case_id)
        self.update_tests(testrun_id, case_ids=case_list)

        return case_list

    def post_test_results(self, run_results):
        """
        Updates the test run match ID run_results['testrun_id'] with results from run_results['results'].
        :param run_results: list of dictionary of results to post.
        :return: list of dictionaries containing test case result data for all updated test cases.
        """
        http_results = self.client.send_post(
            "add_results_for_cases/{}".format(run_results['testrun_id']),
            {
                "results": run_results['results']
            })

        return http_results

    def get_run_results(self, testrun_id):
        """
        Returns results for all completed tests in the specified run
        :param testrun_id: Id of the test run for returning results.
        :return: A list of dictionaries containing test case result data for completed tests in the run.
        """
        return self.client.send_get("get_results_for_run/{}".format(testrun_id))

    def close_run(self, testrun_id):
        """
        Closes the specified test run.
        This action cannot be undone.
        :param testrun_id: Id of the test run to close.
        """
        self.client.send_post("close_run/{}".format(testrun_id), {})

    def delete_run(self, testrun_id):
        """
        Deletes the specified test run.
        This action cannot be undone.
        :param testrun_id: Id of the test run delete.
        """
        self.client.send_post("delete_run/{}".format(testrun_id), {})

    def get_projects(self):
        """
        Used to generate a list project data in TestRail.
        :return: A list of dictionaries containing project data for all projects in TestRail.
        """
        return self.client.send_get("get_projects/")

    def get_cases(self, project_id, suite_id=None):
        """
        Used to generate a list of test cases present in a project/suite.
        :param project_id: ID of the project containing the test cases or suite to be queried.
        :param suite_id: ID of the suite to be queried.
            Required parameter if the TestRail project can contain multiple suites, otherwise optional.
        :return: A list of dictionaries containing data for all test cases in the specified project/suite.
        """
        suite = ""

        if suite_id:
            log.warn('TestRailConnection: Optional Test '
                     'Suite ID value detected: "{}" - adding '
                     'to request URL.'.format(suite_id))
            suite = "&suite_id={}".format(suite_id)

        return self.client.send_get("get_cases/{}{}".format(project_id, suite))

    def close_test_run(self, testrun_id):
        """
        Closes the specified test run.
        This action cannot be undone.
        :param testrun_id: Id of the test run to close.
        """
        self.client.send_post("close_run/{}".format(testrun_id), {})

    def get_suites(self, project_id):
        """
        Get the current project's suites and return them as a list of dicts.
        :param project_id: ID of the TestRail project to query for suites.
        :return: list of dicts representing the suites, or None if the project has no suites.
        """
        return self.client.send_get("get_suites/{}".format(project_id))

    def get_valid_test_case_ids(self, case_ids):
        """
        Parses the list of case_ids and returns a new list of only test case IDs that are on TestRail.
        Emits error logging at run-time and also when the script finishing running that displays the mismatched IDs
        between the XML report and the TestRail site the request was sent to.
        :param case_ids: set of test case IDs to check for on TestRail.
        :return: list of all case_ids found that match TestRail test case IDs.
        """
        valid_testrail_test_case_ids = []

        for case_id in case_ids.copy():
            try:
                if self.get_case(case_id):  # Raises TestRailAPIError if case ID is not on TestRail.
                    valid_testrail_test_case_ids.append(case_id)
            except testrail_api_connector.TestRailAPIError as error:
                log.warning(
                    'TestRailConnection: The following test case ID does NOT exist on TestRail: {}'.format(case_id))
                log.warning('TestRailAPIError: Original API error: {}'.format(str(error)))
                case_ids.remove(case_id)

        return valid_testrail_test_case_ids

    def get_case_types(self):
        """
        Used to get a list of available case types
        :return: A list of dictionaries containing data for all the given test case types
        """
        return self.client.send_get("get_case_types/")

    def get_case_fields(self):
        """
        Used to get a list of available test case custom fields
        :return: A list of dictionaries containing data for all the test case custom fields
        """
        return self.client.send_get("get_case_fields/")

    def get_case(self, case_id):
        """
        Used to get the details of a specific test case
        :param case_id: ID of the test case to be queried
        :return: A list of dictionaries containing data for all the given test case
        """
        return self.client.send_get("get_case/{}".format(case_id))
