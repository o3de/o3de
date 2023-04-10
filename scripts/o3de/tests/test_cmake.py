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
from unittest.mock import patch

from o3de import cmake, manifest

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
                """, 'TestGem', set(['foo', 'bar', 'baz']), 1)
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
            enabled_gems_set = manifest.get_enabled_gems(pathlib.Path('enabled_gems.cmake'))

        assert add_gem_return == expected_return
        assert enabled_gems_set == expected_set


class TestResolveGemDependencyPaths:

    @staticmethod
    def resolve(self):
        return self

    @pytest.mark.parametrize(
        "engine_gem_names, project_gem_names, all_gems_json_data, expected_gem_paths, expected_result", [
            # When no version specifiers are provided expect the gem dependency is found
            pytest.param([], ["gemA"],{
                "gemA":[
                    {"gem_name":"gemA", "path":pathlib.Path('gemAPath')},
                ]
            }, 'gemA;gemAPath', 0),
            # When multiple gem versions exists, expect the one with the highest version is selected
            pytest.param([], ["gemA","gemB"],{
                "gemA":[
                    {"gem_name":"gemA", "path":pathlib.Path('gemAPath')},
                ],
                "gemB":[
                    {"gem_name":"gemB", "version":"0.0.0","path":pathlib.Path('gemB0Path')},
                    {"gem_name":"gemB", "version":"2.0.0","path":pathlib.Path('gemB2Path')},
                    {"gem_name":"gemB", "version":"1.0.0","path":pathlib.Path('gemB1Path')}
                ]
            }, 'gemA;gemAPath;gemB;gemB2Path', 0),
            # When a specific version is requested expect it is found
            pytest.param([], ["gemA==1.2.3","gemB==2.3.4"],{
                "gemA":[
                    {"gem_name":"gemA", "version":"1.2.3", "path":pathlib.Path('gemA1Path')},
                    {"gem_name":"gemA", "version":"2.3.4", "path":pathlib.Path('gemA2Path')},
                ],
                "gemB":[
                    {"gem_name":"gemB", "version":"1.2.3","path":pathlib.Path('gemB1Path')},
                    {"gem_name":"gemB", "version":"2.3.4","path":pathlib.Path('gemB2Path')},
                ]
            }, 'gemA;gemA1Path;gemB;gemB2Path', 0),
            # When no project gems are provided expect engine gems are used
            pytest.param(["gemA==1.2.3"], [],{
                "gemA":[
                    {"gem_name":"gemA", "version":"1.2.3", "path":pathlib.Path('gemA1Path')},
                    {"gem_name":"gemA", "version":"2.3.4", "path":pathlib.Path('gemA2Path')},
                ]
            }, 'gemA;gemA1Path', 0) 
        ]
    )
    def test_resolve_gem_dependency_paths(self, engine_gem_names, project_gem_names, all_gems_json_data, expected_gem_paths, expected_result):
        engine_path = pathlib.PurePath('c:/o3de')
        project_path = pathlib.PurePath('c:/o3de')
        resolved_gem_paths = ''

        def get_project_engine_path(project_path:str or pathlib.Path) -> pathlib.Path or None:
            return engine_path

        def get_this_engine_path():
            return engine_path

        def get_engine_json_data(engine_name:str = None, engine_path:pathlib.Path = None):
            return {
                "engine_name":"o3de",
                "gem_names": engine_gem_names
            } 

        def get_project_json_data(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                user: bool = False) -> dict or None:
            return {
                "project_name":"o3de_project",
                "engine":"o3de",
                "gem_names": project_gem_names
            } 

        class StringBufferIOWrapper(io.StringIO):
            def __exit__(self, exc_type, exc_val, exc_tb):
                nonlocal resolved_gem_paths
                resolved_gem_paths = super().getvalue()
                super().__exit__(exc_tb, exc_val, exc_tb)

        def get_enabled_gem_cmake_file(project_name: str = None,
                                project_path: str or pathlib.Path = None,
                                platform: str = 'Common'):
            return pathlib.Path() 

        def get_enabled_gems(cmake_file: pathlib.Path) -> set:
            return set() 

        def get_gems_json_data_by_name(engine_path:pathlib.Path = None, 
                                       project_path: pathlib.Path = None, 
                                       include_manifest_gems: bool = False,
                                       include_engine_gems: bool = False,
                                       external_subdirectories: list = None
                                       ) -> dict:
            return all_gems_json_data

        with patch('pathlib.Path.is_file', return_value=True) as pathlib_is_file_mock,\
                patch('pathlib.Path.resolve', self.resolve) as pathlib_is_resolve_mock, \
                patch('o3de.manifest.get_project_engine_path', side_effect=get_project_engine_path) as get_project_engine_path_patch,\
                patch('o3de.manifest.get_this_engine_path', side_effect=get_this_engine_path) as get_this_engine_path_patch,\
                patch('o3de.manifest.get_engine_json_data', side_effect=get_engine_json_data) as get_engine_json_data_patch,\
                patch('o3de.manifest.get_project_json_data', side_effect=get_project_json_data) as get_project_json_data_patch,\
                patch('o3de.manifest.get_gems_json_data_by_name', side_effect=get_gems_json_data_by_name) as get_gems_json_data_by_name_patch,\
                patch('o3de.manifest.get_enabled_gem_cmake_file', side_effect=get_enabled_gem_cmake_file) as get_enabled_gem_cmake_patch, \
                patch('o3de.manifest.get_enabled_gems', side_effect=get_enabled_gems) as get_enabled_gems_patch,\
                patch('pathlib.Path.open', side_effect=lambda mode: StringBufferIOWrapper()) as pathlib_open_mock:
            result = cmake.resolve_gem_dependency_paths(
                                        engine_path=engine_path if engine_gem_names else None, 
                                        project_path=project_path if project_gem_names else None,
                                        external_subdirectories=None,
                                        resolved_gem_dependencies_output_path=pathlib.Path('out'))

        assert result == expected_result
        assert resolved_gem_paths == expected_gem_paths
