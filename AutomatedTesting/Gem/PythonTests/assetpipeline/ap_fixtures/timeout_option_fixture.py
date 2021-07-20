"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Pytest fixture for accessing an optional "--timeout" argument used for specifying
the timeout (in seconds) for any one AP Batch call.
If not specified, defaults to 35 minutes
"""

import pytest
import logging

logger = logging.getLogger(__name__)


@pytest.fixture(scope="session")
def timeout_option_fixture(request) -> int:
    """
    Fixture for accessing the command line argument "--ap_timeout"
    Defaults to 35 minutes
    """
    default_timeout = 35 * 60  # 35 minutes (2100 seconds)
    timeout = request.config.getoption("--ap_timeout")
    if timeout is not None:
        try:
            return int(timeout)
        except ValueError:
            # failed casting timeout to integer
            # fmt:off
            logger.error(f"Failed parsing --ap_timeout cmdline argument {timeout} to integer. "
                         f"Using default value of {default_timeout} seconds")
            # fmt:on
    return default_timeout
