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

from o3de import add_gem_project

TEST_WITHOUT_NO_GEM_CONTENT = """
# {BEGIN_LICENSE}
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# {END_LICENSE}

set(GEM_DEPENDENCIES
)
"""

TEST_WITHOUT_ONLY_GEM_CONTENT = """
# {BEGIN_LICENSE}
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# {END_LICENSE}

set(GEM_DEPENDENCIES
    Gem::TestGem
)
"""

TEST_WITHOUT_ADDED_GEM_CONTENT = """
# {BEGIN_LICENSE}
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# {END_LICENSE}

set(GEM_DEPENDENCIES
    Gem::ExistingGem
)
"""

TEST_WITH_ADDED_GEM_CONTENT = """
# {BEGIN_LICENSE}
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# {END_LICENSE}

set(GEM_DEPENDENCIES
    Gem::TestGem
    Gem::ExistingGem
)
"""


@pytest.mark.parametrize(
    "contents, gem, expected_result, runtime_present, expect_failure", [
        pytest.param(TEST_WITHOUT_ADDED_GEM_CONTENT, "TestGem", TEST_WITH_ADDED_GEM_CONTENT, True, False),
        pytest.param(TEST_WITHOUT_ADDED_GEM_CONTENT, "TestGem", TEST_WITH_ADDED_GEM_CONTENT, False, True),
        pytest.param(TEST_WITHOUT_ADDED_GEM_CONTENT, "/TestGem", TEST_WITH_ADDED_GEM_CONTENT, True, True),
        pytest.param(TEST_WITHOUT_NO_GEM_CONTENT, "TestGem", TEST_WITHOUT_ONLY_GEM_CONTENT, True, False),
    ]
)
def test_add_gem_dependency(tmpdir, contents, gem, expected_result, runtime_present, expect_failure):
    dev_root = str(tmpdir.join('dev').realpath()).replace('\\', '/')
    os.makedirs(dev_root, exist_ok=True)

    dev_project_gem_code = f'{dev_root}/TestProject/Gem/Code'
    os.makedirs(dev_project_gem_code, exist_ok=True)

    runtime_dependencies_cmake_file = f'{dev_project_gem_code}/runtime_dependencies.cmake'
    if runtime_present:
        if os.path.isfile(runtime_dependencies_cmake_file):
            os.unlink(runtime_dependencies_cmake_file)
        with open(runtime_dependencies_cmake_file, 'a') as s:
            s.write(contents)

    result = add_gem_project.add_gem_dependency(runtime_dependencies_cmake_file, gem)

    if expect_failure:
        assert result != 0
    else:
        assert result == 0
        with open(runtime_dependencies_cmake_file, 'r') as s:
            s_data = s.read()
        assert s_data == expected_result


@pytest.mark.parametrize(
    "contents, gem, expected_result, runtime_present, expect_failure", [
        pytest.param(TEST_WITH_ADDED_GEM_CONTENT, "TestGem", TEST_WITHOUT_ADDED_GEM_CONTENT, True, False),
        pytest.param(TEST_WITH_ADDED_GEM_CONTENT, "TestGem", TEST_WITHOUT_ADDED_GEM_CONTENT, False, True),
        pytest.param(TEST_WITHOUT_ADDED_GEM_CONTENT, "TestGem", TEST_WITHOUT_ADDED_GEM_CONTENT, True, True)
    ]
)
def test_remove_gem_dependency(tmpdir, contents, gem, expected_result, runtime_present, expect_failure):
    dev_root = str(tmpdir.join('dev').realpath()).replace('\\', '/')
    os.makedirs(dev_root, exist_ok=True)

    dev_project_gem_code = f'{dev_root}/TestProject/Gem/Code'
    os.makedirs(dev_project_gem_code, exist_ok=True)

    runtime_dependencies_cmake_file = f'{dev_project_gem_code}/runtime_dependencies.cmake'
    if runtime_present:
        if os.path.isfile(runtime_dependencies_cmake_file):
            os.unlink(runtime_dependencies_cmake_file)
        with open(runtime_dependencies_cmake_file, 'a') as s:
            s.write(contents)

    result = add_remove_gem.remove_gem_dependency(runtime_dependencies_cmake_file, gem)

    if expect_failure:
        assert result != 0
    else:
        assert result == 0
        with open(runtime_dependencies_cmake_file, 'r') as s:
            s_data = s.read()
        assert s_data == expected_result


@pytest.mark.parametrize("add,"
                         " contents, gem, project, expected_result,"
                         " runtime_present, tool_present,"
                         " ask_for_runtime, ask_for_tool,"
                         " expect_failure", [
                             pytest.param(True,
                                          TEST_WITHOUT_ADDED_GEM_CONTENT, "TestGem", "TestProject",
                                          TEST_WITH_ADDED_GEM_CONTENT,
                                          True, True,
                                          True, True,
                                          False),
                             pytest.param(True,
                                          TEST_WITHOUT_ADDED_GEM_CONTENT, "TestGem", "TestProject",
                                          TEST_WITH_ADDED_GEM_CONTENT,
                                          True, False,
                                          True, True,
                                          True),
                             pytest.param(True,
                                          TEST_WITHOUT_ADDED_GEM_CONTENT, "TestGem", "TestProject",
                                          TEST_WITH_ADDED_GEM_CONTENT,
                                          False, True,
                                          True, True,
                                          True),
                             pytest.param(True,
                                          TEST_WITHOUT_ADDED_GEM_CONTENT, "TestGem", "TestProject",
                                          TEST_WITH_ADDED_GEM_CONTENT,
                                          False, False,
                                          True, True,
                                          True),

                             pytest.param(False,
                                          TEST_WITH_ADDED_GEM_CONTENT, "TestGem", "TestProject",
                                          TEST_WITHOUT_ADDED_GEM_CONTENT,
                                          True, True,
                                          True, True,
                                          False),
                             pytest.param(False,
                                          TEST_WITH_ADDED_GEM_CONTENT, "TestGem", "TestProject",
                                          TEST_WITHOUT_ADDED_GEM_CONTENT,
                                          True, False,
                                          True, True,
                                          True),
                             pytest.param(False,
                                          TEST_WITH_ADDED_GEM_CONTENT, "TestGem", "TestProject",
                                          TEST_WITHOUT_ADDED_GEM_CONTENT,
                                          False, True,
                                          True, True,
                                          True),
                             pytest.param(False,
                                          TEST_WITH_ADDED_GEM_CONTENT, "TestGem", "TestProject",
                                          TEST_WITHOUT_ADDED_GEM_CONTENT,
                                          False, False,
                                          True, True,
                                          True)
                         ]
                         )
def test_add_remove_gem(tmpdir,
                        add,
                        contents, gem, project,
                        expected_result,
                        runtime_present, tool_present,
                        ask_for_runtime, ask_for_tool,
                        expect_failure):
    dev_root = str(tmpdir.join('dev').realpath()).replace('\\', '/')
    os.makedirs(dev_root, exist_ok=True)

    dev_project_gem_code = f'{dev_root}/TestProject/Gem/Code'
    os.makedirs(dev_project_gem_code, exist_ok=True)

    runtime_dependencies_cmake_file = f'{dev_project_gem_code}/runtime_dependencies.cmake'
    if runtime_present:
        if os.path.isfile(runtime_dependencies_cmake_file):
            os.unlink(runtime_dependencies_cmake_file)
        with open(runtime_dependencies_cmake_file, 'a') as s:
            s.write(contents)

    tool_dependencies_cmake_file = f'{dev_project_gem_code}/tool_dependencies.cmake'
    os.makedirs(dev_project_gem_code, exist_ok=True)

    if tool_present:
        if os.path.isfile(tool_dependencies_cmake_file):
            os.unlink(tool_dependencies_cmake_file)
        with open(tool_dependencies_cmake_file, 'w') as s:
            s.write(contents)

    project_folder = f'{dev_root}/TestProject'
    os.makedirs(project_folder, exist_ok=True)

    gems_folder = f'{dev_root}/Gems'
    os.makedirs(gems_folder, exist_ok=True)

    gem_folder = f'{gems_folder}/{gem}'
    os.makedirs(gem_folder, exist_ok=True)

    result = add_remove_gem.add_remove_gem(add, dev_root, gem, project, ask_for_runtime, ask_for_tool)

    if expect_failure:
        assert result != 0
    else:
        assert result == 0
        if runtime_present:
            with open(runtime_dependencies_cmake_file, 'r') as s:
                s_data = s.read()
            assert s_data == expected_result
        if tool_present:
            with open(tool_dependencies_cmake_file, 'r') as s:
                s_data = s.read()
            assert s_data == expected_result

