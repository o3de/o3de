#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import io
import pytest
import pathlib
import re
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


class TestAddGemDependency:
    regex = re.compile('(?P<pre>[\s\S]*set\s*\(\s*ENABLED_GEMS\s*)?(?P<gem_names>[^\)]+)(?P<post>\)[\s\S]*)?')

    @pytest.mark.parametrize(
        "enable_gems_cmake_data, gem_name, expected_set, expected_return", [
            # when an empty file is encountered, expect the new gem is added
            pytest.param("", 'TestGem', set(['TestGem']), 0),
            pytest.param("""
                # Comment
                set(ENABLED_GEMS foo bar baz)
            """, 'TestGem', set(['foo', 'bar', 'baz', 'TestGem']), 0),
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo
                            bar
                            baz
                        )
                    """, 'TestGem', set(['foo', 'bar', 'baz', 'TestGem']), 0),
            pytest.param("""
                    # Comment
                    set(ENABLED_GEMS
                        foo
                        bar
                        baz)
                """, 'TestGem', set(['foo', 'bar', 'baz', 'TestGem']), 0),
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo bar
                            baz)
                    """, 'TestGem', set(['foo', 'bar', 'baz', 'TestGem']), 0),
            pytest.param("""
                """, 'TestGem', set(['TestGem']), 0),
            pytest.param("""
                        # Comment
                        set(RANDOM_VARIABLE TestGame, TestProject Test Engine)
                        set(ENABLED_GEMS HelloWorld IceCream
                            foo
                            baz bar
                            baz baz baz baz baz morebaz lessbaz
                        )
                        Random Text
                    """, 'TestGem', set(['HelloWorld', 'IceCream', 'foo', 'bar', 'baz', 'morebaz', 'lessbaz', 'TestGem']),
                         0),
            pytest.param("""
                        set(ENABLED_GEMS foo bar baz
                """, 'TestGem', set(['foo', 'bar', 'baz']), 1),
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo bar
                            baz)
                    """, 'TestGem==1.0.0', set(['foo', 'bar', 'baz', 'TestGem==1.0.0']), 0),
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo bar
                            TestGem
                            baz)
                    """, 'TestGem==1.0.0', set(['foo', 'bar', 'baz', 'TestGem==1.0.0']), 0),
            # when a different version of the gem exists expect it is replaced with the new entry
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo bar
                            TestGem==1.0.0
                            baz)
                    """, 'TestGem==2.0.0', set(['foo', 'bar', 'baz', 'TestGem==2.0.0']), 0),
            # when multiple versions of the same gem are found expect all are replaced with the new entry
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo bar
                            TestGem==1.0.0 TestGem TestGem>=2.0.0
                            baz)
                    """, 'TestGem', set(['foo', 'bar', 'baz', 'TestGem']), 0),
            pytest.param("""
                    set(ENABLED_GEMS )
                    """, 'TestGem', set(['TestGem']), 0),
        ]
    )
    def test_add_gem_dependency(self, enable_gems_cmake_data, gem_name, expected_set, expected_return):
        enabled_gems_set = set()
        add_gem_return = None

        class StringBufferIOWrapper(io.StringIO):
            def __init__(self):
                nonlocal enable_gems_cmake_data
                super().__init__(enable_gems_cmake_data)
            def __enter__(self):
                return super().__enter__()
            def __exit__(self, exc_type, exc_val, exc_tb):
                nonlocal enable_gems_cmake_data
                enable_gems_cmake_data = super().getvalue()
                super().__exit__(exc_tb, exc_val, exc_tb)
            def read(self):
                return enable_gems_cmake_data


        with patch('pathlib.Path.resolve', return_value=pathlib.Path('enabled_gems.cmake')) as pathlib_is_resolve_mock,\
                patch('pathlib.Path.is_file', return_value=True) as pathlib_is_file_mock,\
                patch('pathlib.Path.open', side_effect=lambda mode: StringBufferIOWrapper()) as pathlib_open_mock:

            add_gem_return = cmake.add_gem_dependency(pathlib.Path('enabled_gems.cmake'), gem_name)
            enabled_gems_set = cmake.get_enabled_gems(pathlib.Path('enabled_gems.cmake'))

        assert add_gem_return == expected_return
        assert enabled_gems_set == expected_set
        if expected_return == 0:
            # sanity check that we have the correct structure 
            all_gem_names_matches = self.regex.search(enable_gems_cmake_data)
            assert all_gem_names_matches
            assert all_gem_names_matches.group('pre')
            assert all_gem_names_matches.group('gem_names')
            assert all_gem_names_matches.group('post')



class TestRemoveGemDependency:
    @pytest.mark.parametrize(
        "enable_gems_cmake_data, gem_name, expected_set, expected_return", [
            pytest.param("""
                # Comment
                set(ENABLED_GEMS foo bar baz TestGem)
            """, 'TestGem', set(['foo', 'bar', 'baz']), 0),
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo
                            bar
                            baz
                            TestGem
                        )
                    """, 'TestGem', set(['foo', 'bar', 'baz']), 0),
            pytest.param("""
                    # Comment
                    set(ENABLED_GEMS
                        foo
                        bar
                        baz
                        TestGem)
                """, 'TestGem', set(['foo', 'bar', 'baz']), 0),
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo bar
                            baz TestGem)
                    """, 'TestGem', set(['foo', 'bar', 'baz']), 0),
            pytest.param("""
                        # Comment
                        set(ENABLED_GEMS
                            foo
                            TestGem
                            bar
                            TestGem
                            baz
                        )
                        Random Text
                    """, 'TestGem', set(['foo', 'bar', 'baz']),
                         0),
            pytest.param("""
                        set(ENABLED_GEMS
                            foo
                            bar
                            baz
                            "TestGem"
                        )
                """, 'TestGem', set(['foo', 'bar', 'baz']), 0),
            pytest.param("""
            """, 'TestGem', set(), 1),
            pytest.param("""
                set(ENABLED_GEMS
                    foo
                    bar
                    baz
                )
                """, 'TestGem', set(['foo', 'bar', 'baz']), 1),
            pytest.param("""
                set(ENABLED_GEMS
                    foo
                    bar
                    baz
                    TestGem==1.0.0
                )
                """, 'TestGem==1.0.0', set(['foo', 'bar', 'baz']), 0),
            pytest.param("""
                set(ENABLED_GEMS
                    foo
                    bar
                    baz
                    TestGem
                )
                """, 'TestGem==1.0.0', set(['foo', 'bar', 'baz','TestGem']), 1),
        ]
    )
    def test_remove_gem_dependency(self, enable_gems_cmake_data, gem_name, expected_set, expected_return):
        enabled_gems_set = set()
        add_gem_return = None

        class StringBufferIOWrapper(io.StringIO):
            def __init__(self):
                nonlocal enable_gems_cmake_data
                super().__init__(enable_gems_cmake_data)
            def __enter__(self):
                return super().__enter__()
            def __exit__(self, exc_type, exc_val, exc_tb):
                nonlocal enable_gems_cmake_data
                enable_gems_cmake_data = super().getvalue()
                super().__exit__(exc_tb, exc_val, exc_tb)


        with patch('pathlib.Path.resolve', return_value=pathlib.Path('enabled_gems.cmake')) as pathlib_is_resolve_mock,\
                patch('pathlib.Path.is_file', return_value=True) as pathlib_is_file_mock,\
                patch('pathlib.Path.open', side_effect=lambda mode: StringBufferIOWrapper()) as pathlib_open_mock:

            add_gem_return = cmake.remove_gem_dependency(pathlib.Path('enabled_gems.cmake'), gem_name=gem_name)
            enabled_gems_set = cmake.get_enabled_gems(pathlib.Path('enabled_gems.cmake'))

        assert add_gem_return == expected_return
        assert enabled_gems_set == expected_set
