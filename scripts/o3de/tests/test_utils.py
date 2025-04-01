#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pytest
import pathlib
# import subprocess
import logging
import unittest.mock as mock
from unittest.mock import patch

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
        pytest.param('new', 'new_'),
        pytest.param('default', 'default_'),
        pytest.param('Default', 'Default'),
        pytest.param('test-', 'test_'),
        
    ]
)
def test_sanitize_identifier_for_cpp(value, expected_result):
    assert utils.sanitize_identifier_for_cpp(value) == expected_result

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

@pytest.mark.parametrize("args, expected_return_code", [
    pytest.param(['cmake', '--version'], 0),
    pytest.param(['cmake'], 0),
    pytest.param(['cmake', '-B'], 1)
])
def test_cli_command(args, expected_return_code):
    process = mock.Mock()
    process.returncode = expected_return_code
    process.poll.return_value = expected_return_code
    with patch("subprocess.Popen", return_value = process) as subproc_popen_patch:
        new_command = utils.CLICommand(args, None, logging.getLogger())
        result = new_command.run()
        assert result == expected_return_code

@pytest.mark.parametrize("test_path", [
    pytest.param(__file__),
    pytest.param(pathlib.Path(__file__)),
    pytest.param(pathlib.Path(__file__).parent)
])
def test_prepend_to_system_path(test_path):
    with patch("sys.path") as path_patch:
        utils.prepend_to_system_path(test_path)
        assert path_patch.insert.called
        result_path = pathlib.Path(path_patch.insert.call_args.args[1])
        assert result_path.is_dir()

        if isinstance(test_path, str):
            test_path = pathlib.Path(test_path)

        if test_path.is_dir():
            assert test_path == result_path    
        else:
            assert test_path.parent == result_path


@pytest.mark.parametrize("input_file, supplied_project_path, ancestor_path, can_validate_project_path, expected_path",[
    # successful cases
    pytest.param(pathlib.Path(__file__), None, pathlib.Path(__file__).parent, True,  pathlib.Path(__file__).parent),
    pytest.param(pathlib.Path(__file__), pathlib.Path(__file__).parent, None, True,  pathlib.Path(__file__).parent),
    pytest.param(pathlib.Path(__file__), pathlib.Path(__file__).parent, pathlib.PurePath("Somewhere/Else"), True,  pathlib.Path(__file__).parent),
    pytest.param(pathlib.Path(__file__), pathlib.PurePath("Somewhere/Else"), None, True,  pathlib.PurePath("Somewhere/Else")),
    
    # failure cases
    pytest.param(pathlib.Path(__file__), None, pathlib.Path(__file__).parent, False,  None),
    pytest.param(pathlib.Path(__file__), None, pathlib.PurePath("Somewhere/Else"), False,  None),
    pytest.param(pathlib.Path(__file__), pathlib.Path(__file__).parent, None, False,  None),
    pytest.param(pathlib.Path(__file__), pathlib.Path(__file__).parent, pathlib.PurePath("Somewhere/Else"), False,  None),
    pytest.param(pathlib.Path(__file__), None, None, True,  None),
    pytest.param(pathlib.Path(__file__), None, None, False,  None),
])
def test_get_project_path_from_file(input_file, supplied_project_path, ancestor_path, can_validate_project_path, expected_path):

    with patch("o3de.validation.valid_o3de_project_json", return_value = can_validate_project_path), \
        patch("o3de.utils.find_ancestor_dir_containing_file", return_value = ancestor_path):

        result_path = utils.get_project_path_from_file(input_file, supplied_project_path)
        assert result_path == expected_path

@pytest.mark.parametrize("input_script_path, context_vars_dict, raisedException, expected_result", [
    # successful cases
    pytest.param(__file__, {}, None, 0),
    pytest.param(pathlib.Path(__file__), {}, None, 0),
    pytest.param(__file__, {"test":"value", "key":12}, None, 0),
    pytest.param(pathlib.Path(__file__), {"test":"value", "key":12}, None, 0),
    
    # failure cases
    pytest.param(__file__, {}, RuntimeError, 1),
    pytest.param(pathlib.Path(__file__), {}, RuntimeError, 1),
    pytest.param(__file__, {"test":"value", "key":12}, RuntimeError, 1),
    pytest.param(pathlib.Path(__file__), {"test":"value", "key":12}, RuntimeError, 1),
])
def test_load_and_execute_script(input_script_path, context_vars_dict, raisedException, expected_result):
    def mock_error():
        if raisedException:
            raise raisedException

    mock_spec = mock.Mock()
    mock_spec.loader.exec_module.return_value = None
    if raisedException:
        mock_spec.loader.exec_module.side_effect = mock_error

    mock_module = mock.Mock()

    with patch("importlib.util.spec_from_file_location", return_value = mock_spec) as spec_from_file_patch,\
        patch("importlib.util.module_from_spec", return_value = mock_module) as module_from_spec_patch,\
        patch("sys.modules") as sys_modules_patch:

        result = utils.load_and_execute_script(input_script_path, **context_vars_dict)

        for key, value in context_vars_dict.items():
            assert hasattr(mock_module, key)
            assert getattr(mock_module, key) == value

        assert result == expected_result



@pytest.mark.parametrize("process_obj, raisedException",[
    # the successful case
    pytest.param({"args":['cmake', '--version'], "pid":0}, None),
    # these raise exceptions, but safe_kill_processes should intercept and log instead
    pytest.param({"args":['cmake', '--version'], "pid":0}, RuntimeError)
])
def test_safe_kill_processes(process_obj, raisedException):
    exceptionCaught = False
    def mock_kill():
        nonlocal exceptionCaught
        if raisedException:
            exceptionCaught = True
            raise raisedException

    process = mock.Mock()
    process.configure_mock(**process_obj)
    process.kill.side_effect = mock_kill

    with patch("subprocess.Popen", return_value = process) as subproc_popen_patch:

        utils.safe_kill_processes(subproc_popen_patch(process_obj["args"]))


        assert (subproc_popen_patch.called)
        assert (process.kill.called)
        assert (not raisedException or exceptionCaught)

