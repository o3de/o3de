"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Exceptions that can occur while interacting with a Launcher
"""


class EditorToolsFrameworkException(Exception):
    """ Indicates that an Exception resulted from inside the LyTestTools framework """
    pass


class LyTestToolsFrameworkException(Exception):
    """ Indicates that an Exception resulted from inside the LyTestTools framework """
    pass


class TestResultException(Exception):
    """Indicates that an unknown result was found during the tests"""
    pass
