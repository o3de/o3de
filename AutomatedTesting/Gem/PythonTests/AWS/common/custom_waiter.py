"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from enum import Enum
import botocore.client
import botocore.waiter
import logging

logging.getLogger('boto').setLevel(logging.CRITICAL)


class WaitState(Enum):
    SUCCESS = 'success'
    FAILURE = 'failure'


class CustomWaiter:
    """
    Base class for a custom waiter.

    Modified from:
    https://docs.aws.amazon.com/code-samples/latest/catalog/python-demo_tools-custom_waiter.py.html
    """
    def __init__(
            self, name: str, operation: str, argument: str,
            acceptors: dict, client: botocore.client, delay: int = 30, max_tries: int = 10,
            matcher='path'):
        """
        Subclasses should pass specific operations, arguments, and acceptors to
        their superclass.

        :param name: The name of the waiter. This can be any descriptive string.
        :param operation: The operation to wait for. This must match the casing of
                          the underlying operation model, which is typically in
                          CamelCase.
        :param argument: The dict keys used to access the result of the operation, in
                         dot notation. For example, 'Job.Status' will access
                         result['Job']['Status'].
        :param acceptors: The list of acceptors that indicate the wait is over. These
                          can indicate either success or failure. The acceptor values
                          are compared to the result of the operation after the
                          argument keys are applied.
        :param client: The Boto3 client.
        :param delay: The number of seconds to wait between each call to the operation. Default to 30 seconds.
        :param max_tries: The maximum number of tries before exiting. Default to 10.
        :param matcher: The kind of matcher to use. Default to 'path'.
        """
        self.name = name
        self.operation = operation
        self.argument = argument
        self.client = client
        self.waiter_model = botocore.waiter.WaiterModel({
            'version': 2,
            'waiters': {
                name: {
                    "delay": delay,
                    "operation": operation,
                    "maxAttempts": max_tries,
                    "acceptors": [{
                        "state": state.value,
                        "matcher": matcher,
                        "argument": argument,
                        "expected": expected
                    } for expected, state in acceptors.items()]
                }}})
        self.waiter = botocore.waiter.create_waiter_with_client(
            self.name, self.waiter_model, self.client)

        self._timeout = delay * max_tries

    def _wait(self, **kwargs):
        """
        Starts the botocore wait loop.

        :param kwargs: Keyword arguments that are passed to the operation being polled.
        """
        self.waiter.wait(**kwargs)

    @property
    def timeout(self):
        return self._timeout


