#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import io
import logging
import pytest
import pathlib
from unittest.mock import patch

from o3de import global_project


logger = logging.getLogger()
logging.basicConfig()

DEFAULT_BOOTSTRAP_SETREG = pathlib.Path('~/.o3de/Registry/bootstrap.setreg').expanduser()
PROJECT_PATH_KEY = ('Amazon', 'AzCore', 'Bootstrap', 'project_path')

class TestSetGlobalProject:
    @pytest.mark.parametrize(
        "output_path, project_path, force, expected_result", [
            pytest.param(pathlib.Path('~/.o3de/Registry/bootstrap.setreg').expanduser(), pathlib.Path('A:/'), False, False),
            pytest.param(pathlib.Path('~/.o3de/Registry/bootstrap.setreg').expanduser(), pathlib.Path('A:/'), True, True)
        ]
    )
    def test_set_global_project_non_existent_project_path(self, output_path, project_path, force, expected_result):
        with patch('pathlib.Path.open', return_value=io.StringIO()) as pathlib_open_mock:
            result = global_project.set_global_project(output_path, project_path=project_path, force=force) == 0


        assert result == expected_result
