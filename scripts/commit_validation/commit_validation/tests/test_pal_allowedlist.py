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

import unittest
from unittest.mock import patch, mock_open

import commit_validation.pal_allowedlist as pal_allowedlist


class PALAllowedTests(unittest.TestCase):
    @patch('builtins.open', mock_open(read_data='*/some/*\n'
                                                '*/set/*\n'
                                                '*/of/*\n'
                                                '*/patterns/*'))
    def setUp(self):
        self.allowedlist = pal_allowedlist.load()

    def test_load_allowedlistFile_patternsLoaded(self):
        self.assertEqual(self.allowedlist.patterns[0], '*/some/*')
        self.assertEqual(self.allowedlist.patterns[1], '*/set/*')
        self.assertEqual(self.allowedlist.patterns[2], '*/of/*')
        self.assertEqual(self.allowedlist.patterns[3], '*/patterns/*')

    def test_isMatch_pathMatchesPattern_returnsTrue(self):
        self.assertTrue(self.allowedlist.is_match('/path/to/some/file.cpp'))

    def test_isMatch_pathDoesNotMatchPattern_returnsFalse(self):
        self.assertFalse(self.allowedlist.is_match('/path/to/another/file.cpp'))


if __name__ == '__main__':
    unittest.main()
