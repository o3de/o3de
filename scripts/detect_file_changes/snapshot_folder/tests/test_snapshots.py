#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import unittest
from unittest.mock import patch
from snapshot_folder.snapshot_folder import FolderSnapshot

class empty_object():
    pass

def fakestat(file_path):
    f = empty_object()
    f.st_mtime = 12345
    return f

@patch('os.stat', side_effect=fakestat)
@patch('os.walk')
def test_CreateSnapshot_sanity(mock_os_walk, mock_os_stat):
    mock_os_walk.return_value = [
        # this mocks os.walk, which always returns a tuple of (root name, folder names, file names)
        ( '',  # root name
          ['subfolder1', 'subfolder2', 'subfolder3'], # folders in here
          ['file1.cpp'], # files in here
        ),
        ( 'subfolder1', 
          [],
          ['file1.cpp', 'file2.cpp'], # file1 is same name as above, but different folder name!
        ),
        ( 'subfolder2', 
          [''],
          [], # empty folders still get tracked
        ),
        ( 'subfolder3', 
          ['subfolder4'], # folders only containing folders
          [],
        ),
        ( 'subfolder3/subfolder4', 
          [],
          ['file4.cpp'],
        )
    ]
    snap = FolderSnapshot.CreateSnapshot('.', ignore_patterns=[])
    
    assert 'subfolder1' in snap.folder_paths
    assert 'subfolder2' in snap.folder_paths
    assert 'subfolder3' in snap.folder_paths
    assert 'subfolder3/subfolder4' in snap.folder_paths
    assert 'file1.cpp' in snap.file_modtimes
    assert 'subfolder1/file1.cpp' in snap.file_modtimes
    assert 'subfolder1/file2.cpp' in snap.file_modtimes
    assert 'subfolder3/subfolder4/file4.cpp' in snap.file_modtimes
    
@patch('os.stat', side_effect=fakestat)
@patch('os.walk')
def test_CreateSnapshot_obeys_exclusions(mock_os_walk, mock_os_stat):
    mock_os_walk.return_value = [
        (   '.', 
            ['sub_buildfolder', 'build', 'build_mac', 'normal_subfolder'], # sneaky trap, sub_buildfolder should not be ignored
            ['file.tif', 'file_not_a_tif.bmp'],
        ),
        (   'sub_buildfolder',  # should not be ignored, its not a match to build_*
            [],
            ['file2.cpp', 'file2.tif'],
        ),
        (   'build_mac', 
            ['normalfolder'], # does not have build in its name but should still be ignored because parent is
            ['file3.cpp'],  # even though it doesnt match a rule itself, it should be omitted since its in build
        ),
        (   'build_mac/normalfolder', 
            [],
            ['file4.cpp'],  # even though it doesnt match a rule itself, it should be omitted since its in build
        ),
        (   'build', 
            [],
            ['file4.cpp'],  # even though it doesnt match a rule itself, it should be omitted since its in build
        ),
        (   'normal_subfolder', 
            ['build'], # a matching folder in a subfolder
            [],
        ),
        (   'normal_subfolder/build', 
            ['file.txt'], # a matching folder in a subfolder
            [],
        )
    ]
    snap = FolderSnapshot.CreateSnapshot('.', ignore_patterns=['*.tif', 'build', 'build_*'])
    
    assert 'sub_buildfolder' in snap.folder_paths
    assert 'file_not_a_tif.bmp' in snap.file_modtimes
    assert 'sub_buildfolder/file2.cpp' in snap.file_modtimes
    assert 'normal_subfolder' in snap.folder_paths

    assert 'build' not in snap.folder_paths
    assert 'build_mac' not in snap.folder_paths
    assert 'normal_subfolder/build' not in snap.folder_paths
    assert 'build_mac/normalfolder' not in snap.folder_paths

    assert 'file1.tif' not in snap.file_modtimes
    assert 'sub_buildfolder/file2.tif' not in snap.file_modtimes
    assert 'build/file3.cpp' not in snap.file_modtimes
    assert 'build_mac/file4.cpp' not in snap.file_modtimes
    assert 'build_mac/normalfolder/file4.cpp' not in snap.file_modtimes
    assert 'normal_subfolder/build/file.txt' not in snap.file_modtimes
    


def test_CompareSnapshots_identical_snapshots_nodiffs():
    # emulate identical snapshots
    snap1 = FolderSnapshot()
    snap1.folder_paths = ['myfolder1', 'myfolder2']
    snap1.file_modtimes = {
        'rootfile.txt' : 12345,
        'myfolder1/file.txt' : 12345
    }
    
    snap2 = FolderSnapshot()
    snap2.folder_paths = ['myfolder1', 'myfolder2']
    snap2.file_modtimes = {
        'rootfile.txt' : 12345,
        'myfolder1/file.txt' : 12345
    }

    changes = FolderSnapshot.CompareSnapshots(snap1, snap2)
    assert not changes.any_changed()
    changed_things = [c for c in changes.enumerate_changes()]
    assert not changed_things

def test_CompareSnapshots_two_of_each_kind_of_change():
    # emulate identical snapshots
    snap1 = FolderSnapshot()
    snap1.folder_paths = ['myfolder1', 'myfolder2', 'myfolder3', 'myfolder4']
    snap1.file_modtimes = {
        'rootfile.txt' : 12345,
        'rootfile2.txt' : 12345,
        'rootfile3.txt' : 12345,
        'myfolder1/file.txt' : 12345,
        'myfolder2/file2.txt' : 12345,
        'myfolder2/file3.txt' : 12345,
    }
    
    snap2 = FolderSnapshot()
    # myfolder1 deleted
    # myfolder2 unchanged
    # myfolder3 unchanged
    # myfolder4 deleted
    # myfolder5 as well as its subfolder, myfolder6 added
    snap2.folder_paths = ['myfolder3', 'myfolder2', 'myfolder5', 'myfolder5/myfolder6']
    snap2.file_modtimes = {
        'rootfile2.txt' : 12345,    # unchanged, in root
        'rootfile3.txt' : 33333,    # modified
        'rootfile4.txt' : 12345,    # a new file
        # myfolder1 is deleted, so myfolder1/file.txt should show up as deleted
        'myfolder2/file2.txt' : 12345, # unchanged, but in subfolder
        'myfolder2/file3.txt' : 33333, # modified in subfolder
        'myfolder5/file4.txt' : 12345, # a new file in a new folder
    }

    changes = FolderSnapshot.CompareSnapshots(snap1, snap2)
    assert changes.any_changed()
    changed_things = [c for c in changes.enumerate_changes()]
    # folders

    for expected_element in [
            ('FOLDER_ADDED', 'myfolder5'),
            ('FOLDER_ADDED', 'myfolder5/myfolder6'),
            ('FOLDER_DELETED', 'myfolder1'),
            ('FOLDER_DELETED', 'myfolder4'),
            ('DELETED', 'rootfile.txt'),
            ('DELETED', 'myfolder1/file.txt'),
            ('ADDED',   'rootfile4.txt'),
            ('ADDED',   'myfolder5/file4.txt'),
            ('CHANGED',   'rootfile3.txt'),
            ('CHANGED',   'myfolder2/file3.txt')
    ]:
        assert expected_element in changed_things
        changed_things.remove(expected_element)

    # every time we did an assert above, we removed the matching element
    # from the change list.  This means that the change list should now be empty 
    # since everything that changed has been accounted for
    assert not changed_things, f"Unexpected change {changed_things}"
