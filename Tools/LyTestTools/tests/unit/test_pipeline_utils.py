"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest
import functools
import ly_test_tools.o3de.pipeline_utils as pipeline_utils

pytestmark = pytest.mark.SUITE_smoke


def lists_are_equal(first_list, second_list):
    first_list.sort()
    second_list.sort()
    return functools.reduce(lambda x, y : x and y, map(lambda p, q: p == q, first_list, second_list))


class TestPipelineUtils(object):
    def test_ListDiff_EmptyLists_ReturnsEmptyLists(self):
        first_list = []
        second_list = []
        diff_first, diff_second = pipeline_utils.get_differences_between_lists(first_list, second_list)
        assert len(diff_first) == 0
        assert len(diff_first) == len(diff_second)
        

    def test_ListDiff_SecondListNotEmpty_ReturnsContentsOfFirstList(self):
        first_list = ["SomeEntry"]
        second_list = []
        diff_first, diff_second = pipeline_utils.get_differences_between_lists(first_list, second_list)
        assert len(diff_first) == 1
        assert len(diff_second) == 0
        assert lists_are_equal(diff_first, first_list)
        

    def test_ListDiff_IdenticalListsSameOrder_ReturnsEmptyLists(self):
        first_list = ["SomeEntry", "AnotherEntry"]
        second_list = ["SomeEntry", "AnotherEntry"]
        diff_first, diff_second = pipeline_utils.get_differences_between_lists(first_list, second_list)
        assert len(diff_first) == 0
        assert len(diff_first) == len(diff_second)
        

    def test_ListDiff_IdenticalListsDifferentOrder_ReturnsEmptyLists(self):
        first_list = ["OutOfOrderEntry", "SomeEntry", "AnotherEntry"]
        second_list = ["SomeEntry", "AnotherEntry", "OutOfOrderEntry"]
        diff_first, diff_second = pipeline_utils.get_differences_between_lists(first_list, second_list)
        assert len(diff_first) == 0
        assert len(diff_first) == len(diff_second)
        

    def test_ListDiff_DifferentLists_ReturnsDifferences(self):
        first_list = [ "ListOneUniqueEntry", "OutOfOrderEntry", "SomeEntry", "AnotherEntry"]
        second_list = ["SomeEntry", "AnotherEntry", "ListTwoUniqueEntry", "OutOfOrderEntry", "SecondUniqueEntry"]
        diff_first, diff_second = pipeline_utils.get_differences_between_lists(first_list, second_list)
        assert lists_are_equal(diff_first, ["ListOneUniqueEntry"])
        assert lists_are_equal(diff_second, ["ListTwoUniqueEntry", "SecondUniqueEntry"])


    def test_ListCompare_EmptyLists_ReturnsTrue(self):
        first_list = []
        second_list = []
        assert pipeline_utils.compare_lists(first_list, second_list) == True
        

    def test_ListCompare_SecondListNotEmpty_ReturnsFalse(self):
        first_list = ["SomeEntry"]
        second_list = []
        assert pipeline_utils.compare_lists(first_list, second_list) == False
        

    def test_ListCompare_IdenticalListsSameOrder_ReturnsTrue(self):
        first_list = ["SomeEntry", "AnotherEntry"]
        second_list = ["SomeEntry", "AnotherEntry"]
        assert pipeline_utils.compare_lists(first_list, second_list) == True
        

    def test_ListCompare_IdenticalListsDifferentOrder_ReturnsTrue(self):
        first_list = ["OutOfOrderEntry", "SomeEntry", "AnotherEntry"]
        second_list = ["SomeEntry", "AnotherEntry", "OutOfOrderEntry"]
        assert pipeline_utils.compare_lists(first_list, second_list) == True
        

    def test_ListCompare_DifferentLists_ReturnsFalse(self):
        first_list = [ "ListOneUniqueEntry", "OutOfOrderEntry", "SomeEntry", "AnotherEntry"]
        second_list = ["SomeEntry", "AnotherEntry", "ListTwoUniqueEntry", "OutOfOrderEntry", "SecondUniqueEntry"]
        assert pipeline_utils.compare_lists(first_list, second_list) == False
