#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pytest
import pathlib
import unittest.mock as mock
from unittest.mock import patch
from o3de.export_project import _export_script, process_command

TEST_PROJECT_JSON_PAYLOAD = '''
{
    "project_name": "TestProject",
    "project_id": "{24114e69-306d-4de6-b3b4-4cb1a3eca58e}",
    "version" : "0.0.0",
    "compatible_engines" : [
        "o3de-sdk==2205.01"
    ],
    "engine_api_dependencies" : [
        "framework==1.2.3"
    ],
    "origin": "The primary repo for TestProject goes here: i.e. http://www.mydomain.com",
    "license": "What license TestProject uses goes here: i.e. https://opensource.org/licenses/MIT",
    "display_name": "TestProject",
    "summary": "A short description of TestProject.",
    "canonical_tags": [
        "Project"
    ],
    "user_tags": [
        "TestProject"
    ],
    "icon_path": "preview.png",
    "engine": "o3de-install",
    "restricted_name": "projects",
    "external_subdirectories": [
        "D:/TestGem"
    ]
}
'''

# Note: the underlying command logic is found in CLICommand class object. That is tested in test_utils.py
@pytest.mark.parametrize("args, expected_result",[
    pytest.param(["cmake", "--version"], 0),
    pytest.param(["cmake"], 0),
    pytest.param(["cmake", "-B"], 1),
    pytest.param([], 1),
])
def test_process_command(args, expected_result):

    cli_command = mock.Mock()
    cli_command.run.return_value = expected_result

    with patch("o3de.utils.CLICommand", return_value=cli_command) as cli:
        result = process_command(args)
        assert result == expected_result



# The following functions will do integration tests of _export_script and execute_python_script, thereby testing all of script execution
TEST_PYTHON_SCRIPT = """
import pathlib
folder = pathlib.Path(__file__).parent
with open(folder / "test_output.txt", 'w') as test_file:
    test_file.write(f"This is a test for the following: {o3de_context.args[0]}")
    """

def check_for_o3de_context_arg(output_file_text, args):
    if len(args) > 0:
        assert output_file_text == f"This is a test for the following: {args[0]}"

TEST_ERR_PYTHON_SCRIPT = """
import pathlib
raise RuntimeError("Test export RuntimeError")
print("hi there")
    """

@pytest.mark.parametrize("input_script, args, should_pass_project_folder, project_folder_subpath, script_folder_subpath, output_filename, is_expecting_error, output_evaluation_func, expected_result", [
    # TEST_PYTHON_SCRIPT
    # successful cases
    pytest.param(TEST_PYTHON_SCRIPT, ['456'], False, pathlib.PurePath("test_project"), pathlib.PurePath("test_project/ExportScripts"), pathlib.PurePath("test_output.txt"), False, check_for_o3de_context_arg, 0),
    pytest.param(TEST_PYTHON_SCRIPT, ['456'], True, pathlib.PurePath("test_project"), pathlib.PurePath("test_project/ExportScripts"), pathlib.PurePath("test_output.txt"), False, check_for_o3de_context_arg, 0),
    pytest.param(TEST_PYTHON_SCRIPT, ['456'], True, pathlib.PurePath("test_project"), pathlib.PurePath("export_scripts"), pathlib.PurePath("test_output.txt"), False, check_for_o3de_context_arg, 0),
    pytest.param(TEST_PYTHON_SCRIPT, [456], True, pathlib.PurePath("test_project"), pathlib.PurePath("export_scripts"), pathlib.PurePath("test_output.txt"), False, check_for_o3de_context_arg, 0),
    # failure cases
    pytest.param(TEST_PYTHON_SCRIPT, [], True, pathlib.PurePath("test_project"), pathlib.PurePath("export_scripts"), pathlib.PurePath("test_output.txt"), False, check_for_o3de_context_arg, 1),
    pytest.param(TEST_PYTHON_SCRIPT, [456], False, pathlib.PurePath("test_project"), pathlib.PurePath("export_scripts"), pathlib.PurePath("test_output.txt"), False, check_for_o3de_context_arg, 1),
    # TEST_ERR_PYTHON_SCRIPT
    pytest.param(TEST_ERR_PYTHON_SCRIPT, [], True, pathlib.PurePath("test_project"), pathlib.PurePath("export_scripts"), None, True, None, 1)
])
def test_export_script(tmp_path,
                       input_script, 
                       args, 
                       should_pass_project_folder, 
                       project_folder_subpath, 
                       script_folder_subpath, 
                       output_filename, 
                       is_expecting_error, 
                       output_evaluation_func,
                       expected_result):
    import sys

    project_folder = tmp_path / project_folder_subpath
    project_folder.mkdir()

    script_folder = tmp_path / script_folder_subpath
    script_folder.mkdir()


    project_json = project_folder / "project.json"
    project_json.write_text(TEST_PROJECT_JSON_PAYLOAD)


    test_script = script_folder / "test.py"
    test_script.write_text(input_script)

    if output_filename:
        test_output = script_folder / output_filename

        assert not test_output.is_file()
    
    result = _export_script(test_script, project_folder if should_pass_project_folder else None, args)

    assert result == expected_result


    # only check for these if we're simulating a successful case
    if result == 0 and not is_expecting_error:
        assert str(script_folder) in sys.path

        if output_filename:
            assert test_output.is_file()

            with test_output.open('r') as t_out:
                output_text = t_out.read()
        
            if output_evaluation_func:
                output_evaluation_func(output_text, args)

        o3de_cli_folder = pathlib.Path(__file__).parent.parent / "o3de"

        assert o3de_cli_folder in [pathlib.Path(sysPath) for sysPath in sys.path]

