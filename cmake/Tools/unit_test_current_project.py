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

import os
import pytest

from . import current_project

TEST_BOOTSTRAP_CONTENT_1 = """
sys_game_folder = Game1
foo = bar
key1 = value1
key2 = value2
assets = pc
"""
TEST_BOOTSTRAP_CONTENT_2 = """
sys_game_folder=Game1
foo = bar
key1 = value1
key2 = value2
assets = pc
"""
TEST_BOOTSTRAP_CONTENT_3 = """
sys_game_folder= Game1
foo = bar
key1 = value1
key2 = value2
assets = pc
"""
TEST_BOOTSTRAP_CONTENT_4 = """
sys_game_folder =Game1
foo = bar
key1 = value1
key2 = value2
assets = pc
"""
TEST_BOOTSTRAP_CONTENT_5 = """
sys_game_folder           =                Game1
foo = bar
key1 = value1
key2 = value2
assets = pc
"""

@pytest.mark.parametrize(
    "contents, expected_result", [
        pytest.param(TEST_BOOTSTRAP_CONTENT_1, 'Game1'),
        pytest.param(TEST_BOOTSTRAP_CONTENT_2, 'Game1'),
        pytest.param(TEST_BOOTSTRAP_CONTENT_3, 'Game1'),
        pytest.param(TEST_BOOTSTRAP_CONTENT_4, 'Game1'),
        pytest.param(TEST_BOOTSTRAP_CONTENT_5, 'Game1'),
    ]
)
def test_get_current_project(tmpdir, contents, expected_result):
    dev_root = str(tmpdir.join('dev').realpath()).replace('\\', '/')
    os.makedirs(dev_root, exist_ok=True)

    bootstrap_file = f'{dev_root}/bootstrap.cfg'
    if os.path.isfile(bootstrap_file):
        os.unlink(bootstrap_file)
    with open(bootstrap_file, 'a') as s:
        s.write(contents)

    result = current_project.get_current_project(dev_root)
    assert expected_result == result


@pytest.mark.parametrize(
    "contents, project_to_set, expected_result", [
        pytest.param(TEST_BOOTSTRAP_CONTENT_1, 'Test1', 0),
        pytest.param(TEST_BOOTSTRAP_CONTENT_1, ' Test2', 0),
        pytest.param(TEST_BOOTSTRAP_CONTENT_1, 'Test3 ', 0),
        pytest.param(TEST_BOOTSTRAP_CONTENT_1, '/Test4', 1),
        pytest.param(TEST_BOOTSTRAP_CONTENT_1, '=Test5', 1),
    ]
)
def test_set_current_project(tmpdir, contents, project_to_set, expected_result):
    dev_root = str(tmpdir.join('dev').realpath()).replace('\\', '/')
    os.makedirs(dev_root, exist_ok=True)

    bootstrap_file = f'{dev_root}/bootstrap.cfg'
    if os.path.isfile(bootstrap_file):
        os.unlink(bootstrap_file)
    with open(bootstrap_file, 'a') as s:
        s.write(contents)

    result = current_project.set_current_project(dev_root, project_to_set)
    assert expected_result == result

    if result == 0:
        project_that_is_set = current_project.get_current_project(dev_root)
        print(project_that_is_set)
        print(project_to_set)
        assert project_to_set.strip() == project_that_is_set