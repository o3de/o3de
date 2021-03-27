"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import unittest.mock as mock
import unittest

import pytest

import ly_test_tools.lumberyard.settings

pytestmark = pytest.mark.SUITE_smoke


class MockDocumentInput(object):
    def __init__(self, lines):
        self._lines = lines

    def __iter__(self):
        return (line for line in self._lines)

    def close(self):
        pass


class TestReplaceLineInFile(unittest.TestCase):

    def setUp(self):
        self.file_name = 'file1'
        self.search_for = 'search_for'
        self.replace_with = 'replace_with'
        self.mock_file_content = [
            "Setting1=Foo",
            ";Setting2=Bar",
            "Setting3=Baz ",
            "Setting4="]

    @mock.patch('fileinput.input')
    @mock.patch('os.path.isfile')
    @mock.patch('logging.Logger.warning')
    def test_ReplaceLineInFile_FileInUse_NoRaise(self, mock_log_warning, mock_path_isfile, mock_input):
        mock_path_isfile.return_value = True
        mock_input.side_effect = PermissionError()

        try:
            with mock.patch('__builtin__.open'):
                ly_test_tools.lumberyard.settings._edit_text_settings_file(self.file_name, self.search_for, self.replace_with)
        except ImportError:
            with mock.patch('builtins.open'):
                ly_test_tools.lumberyard.settings._edit_text_settings_file(self.file_name, self.search_for, self.replace_with)

        mock_log_warning.assert_called_once()

    @mock.patch('fileinput.input')
    @mock.patch('os.path.isfile')
    @mock.patch('logging.Logger.error')
    def test_ReplaceLineInFile_Error_Raises(self, mock_log_error, mock_path_isfile, mock_input):
        mock_path_isfile.return_value = True
        mock_input.side_effect = NotImplementedError()

        with pytest.raises(NotImplementedError):
            try:
                with mock.patch('__builtin__.open'):
                    ly_test_tools.lumberyard.settings._edit_text_settings_file(self.file_name, self.search_for, self.replace_with)
            except ImportError:
                with mock.patch('builtins.open'):
                    ly_test_tools.lumberyard.settings._edit_text_settings_file(self.file_name, self.search_for, self.replace_with)

    @mock.patch('fileinput.input')
    @mock.patch('os.path.isfile')
    def test_ReplaceLineInFile_FileFound_NoRaise(self, mock_path_isfile, mock_input):
        mock_path_isfile.return_value = True

        try:
            with mock.patch('__builtin__.open'):
                ly_test_tools.lumberyard.settings._edit_text_settings_file(self.file_name, self.search_for, self.replace_with)
        except ImportError:
            with mock.patch('builtins.open'):
                ly_test_tools.lumberyard.settings._edit_text_settings_file(self.file_name, self.search_for, self.replace_with)

        mock_input.return_value.close.assert_called_once_with()

    @mock.patch('os.path.isfile')
    @mock.patch('fileinput.input')
    @mock.patch('sys.stdout')
    def test_ReplaceLineInFile_SettingMatch_SettingReplaced(self, mock_stdout, mock_input, mock_isfile):
        mock_isfile.return_value = True
        mock_input.return_value = MockDocumentInput(self.mock_file_content)

        expected_print_lines = [
            mock.call.write("Setting1=NewFoo"), mock.call.write('\n'),
            mock.call.write(";Setting2=Bar"), mock.call.write('\n'),
            mock.call.write("Setting3=Baz"), mock.call.write('\n'),
            mock.call.write("Setting4="), mock.call.write('\n'),
        ]

        ly_test_tools.lumberyard.settings._edit_text_settings_file(self.file_name, 'Setting1', 'NewFoo')

        mock_stdout.assert_has_calls(expected_print_lines)

    @mock.patch('os.path.isfile')
    @mock.patch('fileinput.input')
    @mock.patch('sys.stdout')
    def test_ReplaceLineInFile_CommentedSettingMatch_SettingReplaced(self, mock_stdout, mock_input, mock_isfile):
        mock_isfile.return_value = True
        mock_input.return_value = MockDocumentInput(self.mock_file_content)

        expected_print_lines = [
            mock.call.write("Setting1=Foo"), mock.call.write('\n'),
            mock.call.write("Setting2=NewBar"), mock.call.write('\n'),
            mock.call.write("Setting3=Baz"), mock.call.write('\n'),
            mock.call.write("Setting4="), mock.call.write('\n'),
        ]

        ly_test_tools.lumberyard.settings._edit_text_settings_file(self.file_name, 'Setting2', 'NewBar')

        mock_stdout.assert_has_calls(expected_print_lines)

    @mock.patch('os.path.isfile')
    @mock.patch('fileinput.input')
    @mock.patch('sys.stdout')
    def test_ReplaceLineInFile_EmptySettingMatch_SettingReplaced(self, mock_stdout, mock_input, mock_isfile):
        mock_isfile.return_value = True
        mock_input.return_value = MockDocumentInput(self.mock_file_content)

        expected_print_lines = [
            mock.call.write("Setting1=Foo"), mock.call.write('\n'),
            mock.call.write(";Setting2=Bar"), mock.call.write('\n'),
            mock.call.write("Setting3=Baz"), mock.call.write('\n'),
            mock.call.write("Setting4=NewContent"), mock.call.write('\n'),
        ]

        ly_test_tools.lumberyard.settings._edit_text_settings_file(self.file_name, 'Setting4', 'NewContent')

        mock_stdout.assert_has_calls(expected_print_lines)

    @mock.patch('os.path.isfile')
    @mock.patch('fileinput.input')
    @mock.patch('sys.stdout')
    def test_ReplaceLineInFile_NoMatch_SettingAppended(self, mock_stdout, mock_input, mock_isfile):
        mock_isfile.return_value = True
        mock_input.return_value = MockDocumentInput(self.mock_file_content)

        expected_print_lines = [
            mock.call.write("Setting1=Foo"), mock.call.write('\n'),
            mock.call.write(";Setting2=Bar"), mock.call.write('\n'),
            mock.call.write("Setting3=Baz"), mock.call.write('\n'),
            mock.call.write("Setting4="), mock.call.write('\n'),
        ]

        ly_test_tools.lumberyard.settings._edit_text_settings_file(self.file_name, 'Setting5', 'NewSetting!')

        mock_stdout.assert_has_calls(expected_print_lines)
