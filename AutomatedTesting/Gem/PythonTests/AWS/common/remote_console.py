"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import pytest
import ly_remote_console.remote_console_commands as remote_console_commands


@pytest.fixture
def remote_console(request: pytest.fixture) -> remote_console_commands.RemoteConsole:
    """
    Fixture for creating a remote console instance to send console commands to the
    Lumberyard client console.
    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :return: Remote console instance representing the Lumberyard remote console executable.
    """
    console = remote_console_commands.RemoteConsole()

    def teardown():
        console.stop()

    request.addfinalizer(teardown)
    return console
