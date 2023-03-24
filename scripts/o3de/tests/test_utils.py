#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pytest

from o3de import utils


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


@pytest.mark.parametrize(
    "in_list, out_list", [
        pytest.param(['A', 'B', 'C'], ['A', 'B', 'C']),
        pytest.param(['A', 'B', 'C', 'A', 'C'], ['A', 'B', 'C']),
        pytest.param(['A', {'name': 'A', 'optional': True}], [{'name': 'A', 'optional': True}]),
        pytest.param([{'name': 'A', 'optional': True}, 'A'], ['A']),
        pytest.param([{'name': 'A'}], [{'name': 'A'}]),
        pytest.param([{'optional': False}], []),
    ]
)
def test_remove_gem_duplicates(in_list, out_list):
    result = utils.remove_gem_duplicates(in_list)
    assert result == out_list

@pytest.mark.parametrize(
    "gems, include_optional, expected_result", [
        pytest.param(['GemA', {'name':'GemC'}], True, ['GemA', 'GemC']),
        pytest.param(['GemA', {'name':'GemC','optional':True}], True, ['GemA', 'GemC']),
        pytest.param(['GemA', {'name':'GemC','optional':True}], False, ['GemA' ]),
        pytest.param(['GemA', {'name':'GemC','optional':False}], True, ['GemA', 'GemC'])
    ]
)
def test_get_gem_names_set(gems, include_optional, expected_result):
    result = utils.get_gem_names_set(gems=gems, include_optional=include_optional)
    assert result == set(expected_result)
