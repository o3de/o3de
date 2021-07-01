"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ~/lib/testrail_importer/testrail_tools/testrail_api_connector.py
"""

try:  # py2
    import mock
except ImportError:  # py3
    import unittest.mock as mock
import pytest
import requests

import lib.testrail_importer.testrail_tools.testrail_api_connector as testrail_api_connector


DEFAULT_INFO = {
    'url': 'https://testrail.custom-url-goes-here.com/',
    'user': 'user@user.user',
    'password': 'censor this',
    'uri': '/some_endpoint&some_value=1',
    'data': {'random': 'data'},
    'retry': 3,
}

MOCKED_RESPONSE_DATA = {'id': 1, 'title': 'stuff', 'random': 'data'}


@pytest.mark.unit
class TestApiConnector:

    def test_Init_URLHasSlash_SetsAttributes(self):
        mocked_api_connector = testrail_api_connector.TestRailApiConnector(
            url=DEFAULT_INFO['url'],
            user=DEFAULT_INFO['user'],
            password=DEFAULT_INFO['password']
    )
        assert mocked_api_connector.user == DEFAULT_INFO['user']
        assert mocked_api_connector.password == DEFAULT_INFO['password']
        assert mocked_api_connector.testrail_url == DEFAULT_INFO['url'] + 'index.php?/api/v2/'

    def test_Init_URLNoSlash_SetsAttributes(self):
        test_info = DEFAULT_INFO.copy()
        test_info['url'] = 'https://testrail.no-slash.com'
        mocked_api_connector = testrail_api_connector.TestRailApiConnector(
            url=test_info['url'],
            user=test_info['user'],
            password=test_info['password']
    )
        assert mocked_api_connector.user == test_info['user']
        assert mocked_api_connector.password == test_info['password']
        assert mocked_api_connector.testrail_url == test_info['url'] + '/index.php?/api/v2/'

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector._send_request')
    def test_SendGet_HasValidParams_ReturnsResponseData(self, mock_send_request):
        mock_send_request.return_value = MOCKED_RESPONSE_DATA
        mocked_api_connector = testrail_api_connector.TestRailApiConnector(
            url=DEFAULT_INFO['url'],
            user=DEFAULT_INFO['user'],
            password=DEFAULT_INFO['password']
        )

        under_test = mocked_api_connector.send_get(uri=DEFAULT_INFO['uri'])

        assert under_test == MOCKED_RESPONSE_DATA
        mock_send_request.assert_called_with(
            method='GET', uri=DEFAULT_INFO['uri'], data={}, retry=DEFAULT_INFO['retry'])

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.TestRailApiConnector._send_request')
    def test_SendPost_HasValidParams_ReturnsResponseData(self, mock_send_request):
        mock_send_request.return_value = MOCKED_RESPONSE_DATA
        mocked_api_connector = testrail_api_connector.TestRailApiConnector(
            url=DEFAULT_INFO['url'],
            user=DEFAULT_INFO['user'],
            password=DEFAULT_INFO['password']
        )

        under_test = mocked_api_connector.send_post(uri=DEFAULT_INFO['uri'], data=DEFAULT_INFO['data'])

        assert under_test == MOCKED_RESPONSE_DATA
        mock_send_request.assert_called_with(
            method='POST', uri=DEFAULT_INFO['uri'], data=DEFAULT_INFO['data'], retry=DEFAULT_INFO['retry'])

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.requests.post')
    def test_PrivateSend_HasData200StatusCode_ReturnsResponseData(self, mock_request):
        mock_response = mock.MagicMock()
        mock_response.json.return_value = MOCKED_RESPONSE_DATA
        mock_response.status_code = 200
        mock_request.return_value = mock_response

        mocked_api_connector = testrail_api_connector.TestRailApiConnector(
            url=DEFAULT_INFO['url'],
            user=DEFAULT_INFO['user'],
            password=DEFAULT_INFO['password']
        )

        under_test = mocked_api_connector._send_request(
            method='POST', uri=DEFAULT_INFO['uri'], data=DEFAULT_INFO['data'], retry=DEFAULT_INFO['retry'])

        assert under_test == MOCKED_RESPONSE_DATA

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.json.loads')
    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.requests.post')
    def test_PrivateSend_TriggersHTTPErrorHasErrorResult_RaisesTestRailAPIError(self, mock_request, mock_json):
        requests.HTTPError.read = mock.MagicMock()
        requests.HTTPError.code = 1  # Not a real error code, used to trigger logic in test.
        mock_request.side_effect = requests.HTTPError
        mock_json.return_value = {'error': 'yes'}  # Value needs 'error' key to trigger logic.

        mocked_api_connector = testrail_api_connector.TestRailApiConnector(
                url=DEFAULT_INFO['url'],
                user=DEFAULT_INFO['user'],
                password=DEFAULT_INFO['password']
            )

        with pytest.raises(testrail_api_connector.TestRailAPIError):
            mocked_api_connector._send_request(
                method='POST', uri=DEFAULT_INFO['uri'], data=DEFAULT_INFO['data'], retry=DEFAULT_INFO['retry'])

    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.json.loads')
    @mock.patch('lib.testrail_importer.testrail_tools.testrail_api_connector.requests.post')
    def test_PrivateSend_TriggersHTTPErrorNoErrorResult_RaisesTestRailAPIError(self, mock_request, mock_json):
        requests.HTTPError.read = mock.MagicMock()
        requests.HTTPError.code = 1  # Not a real error code, used to trigger logic in test.
        mock_request.side_effect = requests.HTTPError
        mock_json.return_value = {'dummy': 'value'}  # Value must lack 'error' key for valid test.

        mocked_api_connector = testrail_api_connector.TestRailApiConnector(
                url=DEFAULT_INFO['url'],
                user=DEFAULT_INFO['user'],
                password=DEFAULT_INFO['password']
            )

        with pytest.raises(testrail_api_connector.TestRailAPIError):
            mocked_api_connector._send_request(
                method='POST', uri=DEFAULT_INFO['uri'], data=DEFAULT_INFO['data'], retry=DEFAULT_INFO['retry'])
