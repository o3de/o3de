"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import unittest.mock as mock
import unittest

import pytest

import ly_test_tools.o3de.settings

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
                ly_test_tools.o3de.settings._edit_text_settings_file(self.file_name, self.search_for, self.replace_with)
        except ImportError:
            with mock.patch('builtins.open'):
                ly_test_tools.o3de.settings._edit_text_settings_file(self.file_name, self.search_for, self.replace_with)

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
                    ly_test_tools.o3de.settings._edit_text_settings_file(self.file_name, self.search_for, self.replace_with)
            except ImportError:
                with mock.patch('builtins.open'):
                    ly_test_tools.o3de.settings._edit_text_settings_file(self.file_name, self.search_for, self.replace_with)

    @mock.patch('fileinput.input')
    @mock.patch('os.path.isfile')
    def test_ReplaceLineInFile_FileFound_NoRaise(self, mock_path_isfile, mock_input):
        mock_path_isfile.return_value = True

        try:
            with mock.patch('__builtin__.open'):
                ly_test_tools.o3de.settings._edit_text_settings_file(self.file_name, self.search_for, self.replace_with)
        except ImportError:
            with mock.patch('builtins.open'):
                ly_test_tools.o3de.settings._edit_text_settings_file(self.file_name, self.search_for, self.replace_with)

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

        ly_test_tools.o3de.settings._edit_text_settings_file(self.file_name, 'Setting1', 'NewFoo')

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

        ly_test_tools.o3de.settings._edit_text_settings_file(self.file_name, 'Setting2', 'NewBar')

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

        ly_test_tools.o3de.settings._edit_text_settings_file(self.file_name, 'Setting4', 'NewContent')

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

        ly_test_tools.o3de.settings._edit_text_settings_file(self.file_name, 'Setting5', 'NewSetting!')

        mock_stdout.assert_has_calls(expected_print_lines)


class TestJsonSettings(unittest.TestCase):

    def setUp(self):
        self.test_file_name = 'something.json'
        self.mock_file_content = """
            {
                "name": "Foo",
                "weight": 30,
                "scale": {
                    "x": 1,
                    "y": 2,
                    "z": 3
                },
                " ":"secret",
                "": 0
                
            }"""

    def test_ReadJson_RetrieveKey_Success(self,):
        mock_open = mock.mock_open(read_data=self.mock_file_content)

        with mock.patch('builtins.open', mock_open):
            with ly_test_tools.o3de.settings.JsonSettings(self.test_file_name) as js:
                # get the whole document
                value = js.get_key('')
                assert len(value) == 5

                # get a nested key
                value = js.get_key('/scale/x')
                assert value == 1

                # get the " " key ad the root level
                value = js.get_key('/ ')
                assert value == 'secret'

                # get the "" key at the root level
                value = js.get_key('/')
                assert value == 0

    def test_ReadJson_RetrieveMissingKey_DefaultReturned(self):
        mock_open = mock.mock_open(read_data=self.mock_file_content)
        default_value = -10

        with mock.patch('builtins.open', mock_open):
            with ly_test_tools.o3de.settings.JsonSettings(self.test_file_name) as js:
                value = js.get_key('/scale/w', default_value)
                assert value == default_value

    def test_ReadJson_ModifyKey_KeyModified(self):
        mock_open = mock.mock_open(read_data=self.mock_file_content)
        expected = 100

        with mock.patch('builtins.open', mock_open):
            with ly_test_tools.o3de.settings.JsonSettings(self.test_file_name) as js:
                js.set_key('/scale/x', expected)
                value = js.get_key('/scale/x')
                assert value == expected

    @mock.patch('json.dump')
    def test_WriteJson_ModifyKey_KeyModified(self, json_dump):
        mock_open = mock.mock_open(read_data=self.mock_file_content)
        expected = "this is the new value"
        new_dict_content = None

        def _mock_dump(content, file_path, indent):
            nonlocal new_dict_content
            new_dict_content = content
        json_dump.side_effect = _mock_dump

        with mock.patch('builtins.open', mock_open):
            with ly_test_tools.o3de.settings.JsonSettings(self.test_file_name) as js:
                js.set_key('/name', expected)

        assert expected == new_dict_content['name']

    @mock.patch('json.dump')
    def test_WriteJson_RemoveKey_KeyRemoved(self, json_dump):
        mock_open = mock.mock_open(read_data=self.mock_file_content)
        new_dict_content = None

        def _mock_dump(content, file_path, indent):
            nonlocal new_dict_content
            new_dict_content = content
        json_dump.side_effect = _mock_dump

        with mock.patch('builtins.open', mock_open):
            with ly_test_tools.o3de.settings.JsonSettings(self.test_file_name) as js:
                js.remove_key('/scale/z')

        assert len(new_dict_content['scale']) == 2
