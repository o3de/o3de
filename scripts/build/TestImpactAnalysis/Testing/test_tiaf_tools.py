#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


from logging import getLogger
from tiaf_tools import run
import os
import json
from pathlib import Path
logging = getLogger("tiaf_tools")

class TestTIAFToolsLocal():

    def test_no_args(self, caplog):
        # given:
        test_string = "You must define a repository to search in when searching locally"
        args = {}

        # when:
        run(args)

        # then:
        assert test_string in caplog.messages

    def test_local_search_in_has_invalid_file_path(self, caplog):
        # given:
        test_string = 'FileNotFoundError'
        args = {'search_in' : "Not a file path!"}

        # when:
        run(args)

        # then:
        assert test_string in caplog.messages

    def test_local_search_in_has_valid_file_path(self, caplog, tmp_path_factory):
        # given:
        test_string = 'FileNotFoundError'
        file = tmp_path_factory.mktemp("test")
        args = {'search_in': file}

        # when:
        run(args)

        # then:
        assert test_string not in caplog.messages

class TestTIAFToolsLocalJSON():
    def test_local_search_full_address_no_action_file_does_not_exist(self, caplog, tmp_path):
        # given:
        fn = tmp_path / "test_file.txt"
        full_address = fn
        test_string = f"File not found at {full_address}"
        args = {'full_address' : full_address, 'file_type' : 'json'}

        # when:
        run(args)

        # then:
        assert test_string in caplog.messages

    def test_local_search_full_address_file_does_exist(self, caplog, tmp_path_factory):
        # given:
        fn = tmp_path_factory.mktemp("test") / "test_file.txt"
        fn.write_text("Testing")
        full_address = fn
        test_string = f"File found at {full_address}"
        args = {'full_address' : full_address, 'file_type' : 'json'}

        # when:
        run(args)

        # then:
        assert test_string in caplog.messages

    def test_local_full_address_action_read_file_exists(self, caplog, tmp_path_factory, mocker):
        # given:
        fn = tmp_path_factory.mktemp("test") / "test_file.txt"
        fn.write_text("Testing")
        full_address = fn
        test_string = f"File found at {full_address}"
        args = {'full_address' : full_address, 'action': 'read', 'file_type' : 'json'}

        mocker.patch('os.startfile')

        # when:
        run(args)

        # then:
        assert test_string in caplog.messages
        os.startfile.assert_called_once_with(os.path.split(full_address)[0])

    def test_local_full_address_action_delete(self, caplog, tmp_path_factory):
        # given:
        fn = tmp_path_factory.mktemp("test") / "test_file.txt"
        fn.write_text("Testing")
        assert os.path.exists(fn)
        full_address = fn
        args = {'full_address' : full_address, 'action': 'delete', 'file_type' : 'json'}

        # when:
        run(args)

        # then:
        assert os.path.exists(fn) == False

    def test_local_full_address_action_create_valid_json(self, caplog, tmp_path_factory):
        # given:
        fn = tmp_path_factory.mktemp("test") / "test_file.txt"
        json_list = ['test']
        json_str = json.dumps(json_list)
        fn.write_text(json_str)
        assert os.path.exists(fn)
        target_dir = tmp_path_factory.mktemp("test")
        full_address = f"{target_dir}/historic_data.json"
        args = {'full_address' : full_address, 'action': 'create', 'file_in':fn, 'file_type' : 'json'}

        # when:
        run(args)

        # then:
        assert os.path.exists(full_address)

    def test_local_full_address_action_create_invalid_json(self, caplog, tmp_path_factory):
        # given:
        fn = tmp_path_factory.mktemp("test") / "test_file.txt"
        test_string = "The historic data does not contain valid json"
        fn.write_text("test!")
        assert os.path.exists(fn)
        target = tmp_path_factory.mktemp("test")
        full_address = target
        args = {'full_address' : full_address, 'action': 'create', 'file_in':fn, 'file_type' : 'json'}

        # when:
        run(args)

        # then:
        assert test_string in caplog.messages
        assert os.path.exists(str(target)+"/historic_data.json") == False

    def test_local_full_address_action_create_already_exists(self, caplog, tmp_path_factory):
        # given:
        file_in = tmp_path_factory.mktemp("test") / "test_file.txt"
        source_json_list = ['test']
        source_json_str = json.dumps(source_json_list)
        file_in.write_text(source_json_str)
        assert os.path.exists(file_in)

        test_string = "Cancelling create, as file already exists"
        
        target = tmp_path_factory.mktemp("test") / "historic_data.json"
        target_json_list = ['success']
        target_json_str = json.dumps(target_json_list)
        target.write_text(target_json_str)
        full_address = target
        args = {'full_address' : full_address, 'action': 'create', 'file_in':file_in, 'file_type' : 'json'}

        # when:
        run(args)

        # then:
        assert test_string in caplog.messages
        assert os.path.exists(target)
        assert target.read_text() == target_json_str
    
    def test_local_full_address_action_update_valid_json(self, caplog, tmp_path_factory):
        # given:
        file_in = tmp_path_factory.mktemp("test") / "test_file.txt"
        source_json_list = ['test']
        source_json_str = json.dumps(source_json_list)
        file_in.write_text(source_json_str)
        assert os.path.exists(file_in)

        target = tmp_path_factory.mktemp("test") / "historic_data.json"
        target_json_list = ['failure']
        target_json_str = json.dumps(target_json_list)
        target.write_text(target_json_str)
        full_address = target
        assert os.path.exists(full_address)
        args = {'full_address' : full_address, 'action': 'update', 'file_in':file_in, 'file_type' : 'json'}

        # when:
        run(args)

        # then:
        out_file_str = target.read_text()
        assert "test" in out_file_str
        assert not "failure" in out_file_str
        
    def test_local_full_address_action_update_invalid_json(self, caplog, tmp_path_factory):
        # given:
        file_in = tmp_path_factory.mktemp("test") / "test_file.txt"
        source_json_str = "failure"
        file_in.write_text(source_json_str)
        assert os.path.exists(file_in)

        target = tmp_path_factory.mktemp("test") / "historic_data.json"
        target_json_list = ['success']
        target_json_str = json.dumps(target_json_list)
        target.write_text(target_json_str)
        full_address = target
        assert os.path.exists(full_address)

        test_string = "The historic data does not contain valid json"
        args = {'full_address' : full_address, 'action': 'update', 'file_in':file_in, 'file_type' : 'json'}

        # when:
        run(args)

        # then:
        out_file_str = target.read_text()
        assert "success" in out_file_str
        assert not "failure" in out_file_str
        assert test_string in caplog.messages

    def test_local_full_address_action_update_file_does_not_exist(self, caplog, tmp_path_factory):
        # given:
        file_in = tmp_path_factory.mktemp("test") / "test_file.txt"
        source_json_list = ['test']
        source_json_str = json.dumps(source_json_list)
        file_in.write_text(source_json_str)
        assert os.path.exists(file_in)

        test_string = "Cancelling update, as file does not exist"
        
        target = tmp_path_factory.mktemp("test")
        full_address = target.parent
        args = {'full_address' : full_address, 'action': 'update', 'file_in':file_in, 'file_type' : 'json'}

        # when:
        run(args)

        # then:
        assert test_string in caplog.messages
        assert not os.path.exists(str(target)+"/historic_data.json")

    def test_local_display_directory(self, caplog, tmp_path_factory):
        # given:
        address_list = []
        for i in range(0, 2):
            file = tmp_path_factory.mktemp("folder" + str(i)) / "historic_data.json"
            file.write_text("test_data")
            assert os.path.exists(file)
            address_list.append(file)
        root_dir = address_list[0].parent.parent
        args = {"search_in" : root_dir, 'file_type' : 'json'}

        # when:
        run(args)

        #then
        i = 0
        addresses = Path.glob(Path(root_dir), "**/*.json")
        for address in addresses:
            absolute = str(address.absolute())
            assert absolute in caplog.messages[i]
            i += 1

    def test_display_full_address(self, caplog, tmp_path_factory):
        # given:
        file = tmp_path_factory.mktemp("test") / "historic_data.json"
        file.write_text("test_data")
        assert os.path.exists(file)

        args = {'full_address': file, 'file_type' : 'json'}
        test_string = f"File found at {file}"
        # when:

        run(args)

        # then:
        assert test_string in caplog.messages
