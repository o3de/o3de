#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import unittest
from unittest.mock import patch, mock_open

from commit_validation import pal_allowedlist
from commit_validation.tests.mocks.mock_commit import MockCommit
from commit_validation.validators.az_trait_validator import AzTraitValidator

import pytest


class Test_AzTraitValidatorTests():
    def test_fileDoesntCheckAzTraitIsDefined_passes(self):
        commit = MockCommit(
                files=['someCppFile.cpp'], 
                file_diffs={ 'someCppFile.cpp' : ''}
        )
        error_list = []
        assert AzTraitValidator().run(commit, error_list)
        assert len(error_list) == 0, f"Unexpected errors: {error_list}"

    @pytest.mark.parametrize(
        'file_diffs,expect_success', [
            pytest.param('+This file does contain\n'
                         '+a trait existence check\n'
                         '+#ifdef AZ_TRAIT_USED_INCORRECTLY\n',
                         False,
                         id="AZ_TRAIT_inside_ifdef_fails" ), # gives the test a friendly name!
            
            pytest.param('+This file does contain\n'
                         '+a trait existence check\n'
                         '+#if defined(AZ_TRAIT_USED_INCORRECTLY)\n',
                         False,
                         id="AZ_TRAIT_inside_if_defined_fails" ),
            
            pytest.param('+This file does contain\n'
                         '+a trait existence check\n'
                         '+#ifndef AZ_TRAIT_USED_INCORRECTLY\n',
                        False,
                        id="AZ_TRAIT_inside_ifndef_fails" ),
            
            pytest.param('+This file contains a diff which REMOVES an incorrect usage\n'
                         '-#ifndef AZ_TRAIT_USED_INCORRECTLY\n',
                         True,
                         id="AZ_TRAIT_removed_in_diff_passes" ),
            
            pytest.param('+This file contains a diff which has an old already okayed usage\n'
                         '+which is not actually part of the diff.\n'
                         '#ifndef AZ_TRAIT_USED_INCORRECTLY\n',
                         True,
                         id="AZ_TRAIT_in_unmodified_section_passes"),
            
            pytest.param('+This file contains the correct usage\n'
                         '+#if AZ_TRAIT_USED_CORRECTLY\n',
                         True,
                         id="AZ_TRAIT_correct_usage_passes"),
        ])
    def test_fileChecksAzTraitIsDefined(self, file_diffs, expect_success):
        commit = MockCommit(
            files=['someCppFile.cpp'],
            file_diffs={ 'someCppFile.cpp' : file_diffs })

        error_list = []
        if expect_success:
            assert AzTraitValidator().run(commit, error_list)
            assert len(error_list) == 0, f"Unexpected errors: {error_list}"
        else:
            assert not AzTraitValidator().run(commit, error_list)
            assert len(error_list) != 0, f"Errors were expected but none were returned."
    
    def test_fileExtensionIgnored_passes(self):
        commit = MockCommit(files=['someCppFile.waf_files'])
        error_list = []
        assert AzTraitValidator().run(commit, error_list)
        assert len(error_list) == 0, f"Unexpected errors: {error_list}"

    @patch('commit_validation.pal_allowedlist.load', return_value=pal_allowedlist.PALAllowedlist(['*/some/path/*']))
    def test_fileAllowedlisted_passes(self, mocked_load):
        commit = MockCommit(files=['/path/to/some/path/someCppFile.cpp'])
        error_list = []
        assert AzTraitValidator().run(commit, error_list)
        assert len(error_list) == 0, f"Unexpected errors: {error_list}"

if __name__ == '__main__':
    unittest.main()
