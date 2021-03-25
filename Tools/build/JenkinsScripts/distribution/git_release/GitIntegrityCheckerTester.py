"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import unittest
import GitIntegrityChecker


class GitIntegrityCheckerTester(unittest.TestCase):

    def test_hashCountMatch_withEqualCounts_returnsTrue(self):
        list_1 = ["A", "B", "C"]
        list_2 = ["A", "B", "C"]
        self.assertTrue(GitIntegrityChecker.validate_hash_counts_match(list_1, list_2))

    def test_hashCountMatch_withBiggerList1_returnsFalse(self):
        list_1 = ["A", "B", "C", "D"]
        list_2 = ["A", "B", "C"]
        self.assertFalse(GitIntegrityChecker.validate_hash_counts_match(list_1, list_2))

    def test_hashCountMatch_withBiggerList2_returnsFalse(self):
        list_1 = ["A", "B", "C"]
        list_2 = ["A", "B", "C", "D"]
        self.assertFalse(GitIntegrityChecker.validate_hash_counts_match(list_1, list_2))

    def test_hashMatch_withIdenticalLists_returnsTrue(self):
        list_1 = ["A", "B", "C"]
        list_2 = ["A", "B", "C"]
        match_results, list_1_out, list_2_out = GitIntegrityChecker.validate_hashes_match(list_1, list_2)
        self.assertTrue(match_results)

    def test_hashMatch_withDifferentLists_returnsFalse(self):
        list_1 = ["A", "B", "D"]
        list_2 = ["A", "B", "C"]
        match_results, list_1_out, list_2_out = GitIntegrityChecker.validate_hashes_match(list_1, list_2)
        self.assertFalse(match_results)

if __name__ == '__main__':
    unittest.main()
