#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import pytest

from . import utils

@pytest.mark.parametrize(
    "value, expected_result", [
        pytest.param('Game1', True),
        pytest.param('0Game1', False),
        pytest.param('the/Game1', False),
        pytest.param('', False),
        pytest.param('-test', False),
        pytest.param('test-', True),
    ]
)
def test_validate_identifier(value, expected_result):
    result = utils.validate_identifier(value)
    assert result == expected_result

@pytest.mark.parametrize(
    "value, expected_result", [
        pytest.param('{018427ae-cd08-4ff1-ad3b-9b95256c17ca}', False),
        pytest.param('', False),
        pytest.param('{018427aecd084ff1ad3b9b95256c17ca}', False),
        pytest.param('018427ae-cd08-4ff1-ad3b-9b95256c17ca', True),
        pytest.param('018427aecd084ff1ad3b9b95256c17ca', False),
        pytest.param('018427aecd084ff1ad3b9', False),
    ]
)
def test_validate_uuid4(value, expected_result):
    result = utils.validate_uuid4(value)
    assert result == expected_result
