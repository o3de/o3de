"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Functions that repeatedly run until a condition is met
"""
import time


def wait_for(fn_act, timeout=30, exc=None, interval=1):
    """
    Continues to execute a given function until the function returns True. Raises an exception if the function does
    not return True before the timeout

    :param fn_act: The target function to execute
    :param timeout: The set amount of time before raising an exception
    :param exc: The exception to raise. An assertion error is raised by default
    :param interval: The time to wait between subsequent function calls
    """
    timeout_end = time.time() + timeout
    while not fn_act():
        if time.time() > timeout_end:
            if exc is not None:
                raise exc
            else:
                assert False, 'Timeout waiting for {}() after {}s.'.format(fn_act.__name__, timeout)
        time.sleep(interval)


def wait_while(fn_act, timeout, exc=None, interval=1):
    """
    Continues to execute a given function until the specified amount of time has passed. Raises an exception if the
    function does not return True during this time.

    :param fn_act: The target function to execute
    :param timeout: The set amount of time to wait
    :param exc: The exception to raise. An assertion error is raised by default
    :param interval: The time to wait between subsequent function calls
    """
    timeout_end = time.time() + timeout
    while time.time() < timeout_end:
        if not fn_act():
            if exc is not None:
                raise exc
            else:
                assert False, '{}() failed while waiting for {}s'.format(fn_act.__name__, timeout)
        time.sleep(interval)
