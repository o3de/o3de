"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

API connector for updating TestRail with test results.
"""

import base64
import json
import logging
import requests
import six

log = logging.getLogger(__name__)


class TestRailAPIError(Exception):
    """Raised when bad responses received from TestRail API."""

    pass


class TestRailApiConnector(object):
    """
    Constructor for creating a TestRail API Connection.
    :param url: The base url string of your TestRail site.
        i.e. https://testrail.your-custom-url-here.com/
    :param user: The user name or e-mail string to log into TestRail with.
    :param password: The password string matching that user.
    """

    def __init__(self, url, user, password):
        self.user = user
        self.password = password
        if not url.endswith('/'):
            url += '/'
        self.testrail_url = url + 'index.php?/api/v2/'

    def send_get(self, uri, retry=3):
        # type: (str, int) -> json
        """
        GET data from the TestRail web API
        :param uri: Endpoint string to retrieve data from
        :param retry: int representing the amount of times to retry a request before raising a TestRailAPIError.
        :return: The returned response.json() data from the get request
        """
        return self._send_request(method='GET', uri=uri, data={}, retry=retry)

    def send_post(self, uri, data, retry=3):
        # type: (str, dict, int) -> json
        """
        POST data to the TestRail web API
        :param uri: Endpoint string to send the request to.
        :param data: Dict payload to send with the request (converts to json)
        :param retry: int representing the amount of times to retry a request before raising a TestRailAPIError.
        :return: The returned response.json() data from the post request
        """
        return self._send_request(method='POST', uri=uri, data=data, retry=retry)

    def _send_request(self, method, uri, data, retry):
        # type: (str, str, dict, int) -> json
        """
        Send a request using the supplied method, uri, and data to _get_response() to get a response object.
        It then parses & returns the JSON data using response.json().
        :param method: The request method string to use, i.e.'GET' or 'POST'
        :param uri: Endpoint string to send the request to.
        :param data: Dict payload to send with the request (converts to json), can be empty value or None if unused.
        :param retry: int representing the amount of times to retry a request before raising a TestRailAPIError.
        :return: response.json() data from the response object returned by _get_response(),
            otherwise it will raise a TestRailApiError or return an empty dict.
        """
        log.debug('TestRailApiConnector: Sending requesting using method "{}" to endpoint "{}"'.format(method, uri))
        url = self.testrail_url + uri
        auth_token = ('{}:{}'.format(self.user, self.password)).replace('\n', '').encode()
        auth = base64.b64encode(auth_token)
        headers = {
            "Authorization": "Basic " + auth.decode(),
            "Content-Type": "application/json"
        }
        request_method = getattr(requests, method.lower())
        result = {}

        # Get a valid response object first.
        response = self._get_response(request_method, url, headers, data, retry)
        log.info('TestRailApiConnector: Got response code: {}'.format(response.status_code))

        # Valid response received, decode the JSON and return.
        if response:
            try:
                result = response.json()
            except Exception as json_error:  # Purposefully broad.
                problem = TestRailAPIError('TestRail JSONDecode error - response.json() failed to be decoded')
                six.raise_from(problem, json_error)

        log.debug('TestRailApiConnector: Returning response.json(): {}'.format(result))
        return result

    def _get_response(self, request_method, url, headers, data, retry):
        # type: (..., str, dict, dict, int) -> requests.models.Response
        """
        Returns a validated request response object for a given request.
        :param request_method: function object to use with the requests module, i.e. getattr(requests, 'get').
        :param url: string for the fully constructed request URL to send the request to.
        :param headers: dict for any required headers the request needs to succeed.
        :param data: dict payload to send with the request, can be empty or None if not used.
        :param retry: int representing the amount of times to retry a request before raising a TestRailAPIError.
        :return: requests.models.Response object, None, or raises a TestRailApiError exception instead.
        """
        response = None
        payload = None
        if data:
            payload = json.dumps(data, separators=(',', ':')).encode()

        while retry > 0:  # Send response until retry is 0 or response succeeds.
            try:
                response = request_method(url=url, headers=headers, data=payload)
                response.raise_for_status()  # Triggers exceptions.
                break  # Break loop when no exceptions triggered.
            except Exception as error:  # purposefully broad
                log.debug(
                    'TestRailApiConnector: An error occurred with the request - retries remaining: {}'.format(retry),
                    exc_info=True)
                retry -= 1
                if retry <= 0:  # Retry limit reached.
                    log.exception(error)
                    problem = TestRailAPIError(
                        'Exception occurred when trying to get request response for {}'.format(url))
                    six.raise_from(problem, error)

        return response
