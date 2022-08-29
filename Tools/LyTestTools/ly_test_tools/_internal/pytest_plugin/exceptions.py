"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Exceptions that can occur while interacting with a Launcher
"""


class LyTestToolsFrameworkException(Exception):
    """ Indicates that an Exception resulted from inside the LyTestTools framework """

    def __init__(self, inner_exception=None):
        self.inner_exception = inner_exception

    def __str__(self):
        return f"This exception was generated from inside the LyTestTools framework. The inner exception is {self.inner_exception}"
