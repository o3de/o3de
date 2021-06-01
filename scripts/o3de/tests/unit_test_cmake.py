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

import io
import json
import logging
import pytest
import pathlib
from unittest.mock import patch

from o3de import cmake


class TestGetEnabledGems:
    @pytest.mark.parametrize(
        "enable_gems_cmake_data, expected_set", [
            pytest.param("""
                # Comment
                set(ENABLED_GEMS foo bar baz)
            """, set(['foo', 'bar', 'baz'])),
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo
                            bar
                            baz
                        )
                    """, set(['foo', 'bar', 'baz'])),
            pytest.param("""
                    # Comment
                    set(ENABLED_GEMS
                        foo
                        bar
                        baz)
                """, set(['foo', 'bar', 'baz'])),
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo bar
                            baz)
                    """, set(['foo', 'bar', 'baz'])),
            pytest.param("""
                        # Comment
                        set(RANDOM_VARIABLE TestGame, TestProject Test Engine)
                        set(ENABLED_GEMS HelloWorld IceCream
                            foo
                            baz bar
                            baz baz baz baz baz morebaz lessbaz
                        )
                        Random Text
                    """, set(['HelloWorld', 'IceCream', 'foo', 'bar', 'baz', 'morebaz', 'lessbaz'])),
        ]
    )
    def test_get_enabled_gems(self, enable_gems_cmake_data, expected_set):
        enabled_gems_set = set()
        with patch('pathlib.Path.resolve', return_value=pathlib.Path('enabled_gems.cmake')) as pathlib_is_resolve_mock,\
                patch('pathlib.Path.is_file', return_value=True) as pathlib_is_file_mock,\
                patch('pathlib.Path.open', return_value=io.StringIO(enable_gems_cmake_data)) as pathlib_open_mock:
            enabled_gems_set = cmake.get_enabled_gems(pathlib.Path('enabled_gems.cmake'))

        assert enabled_gems_set == expected_set
