"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for utils.py
"""
import pytest
import os
import atom_rpi_tools.utils as utils


def test_FindOrCopyFile_DestFileNotExist_CopySuccess(tmpdir):
    # created dir and copied
    filename = 'pass_requests.json'
    source_path = os.path.join(os.path.dirname(__file__), 'testdata/', filename)
    dest_path = os.path.join(tmpdir, 'testdata/', 'pass_requests.json')
    assert not os.path.exists(dest_path)
    utils.find_or_copy_file(dest_path, source_path)
    assert os.path.exists(dest_path)
    source_size = os.path.getsize(source_path)
    dest_size = os.path.getsize(dest_path)
    assert source_size == dest_size

def test_FindOrCopyFile_DestFileAlreadyExists_Skip(tmpdir):
    # copy %cur_dir%/testdata/pass_requests.json to tempdir/testdata/pass_requests.json
    filename = 'pass_requests.json'
    source_path = os.path.join(os.path.dirname(__file__), 'testdata/', filename)
    dest_path = os.path.join(tmpdir, 'testdata/', 'pass_requests.json')
    utils.find_or_copy_file(dest_path, source_path)
    
    # skip if dest_path already exists
    assert os.path.exists(dest_path)
    before_size = os.path.getsize(dest_path)
    source_path = os.path.join(os.path.dirname(__file__), 'testdata/', 'pass_slots.json')
    before_source_size = os.path.getsize(source_path)
    assert before_size != source_path
    utils.find_or_copy_file(dest_path, source_path)
    after_size = os.path.getsize(dest_path)
    assert before_size == after_size

    
def test_FindOrCopyFile_SourceFileNotExists_ExceptionThrown(tmpdir):
    # report error if source doesn't exist
    bad_source_path = 'notexist.dat'
    dest_path = os.path.join(tmpdir, 'notexist.dat')
    with pytest.raises(ValueError):
        utils.find_or_copy_file(dest_path, bad_source_path)
