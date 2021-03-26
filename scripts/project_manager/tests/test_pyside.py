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

import pytest
import sys
import os

pyside_path = os.path.realpath(os.path.join(os.path.dirname(__file__), '..'))
sys.path.append(pyside_path)

from pyside import add_pyside_environment, is_pyside_ready, is_configuration_valid

from ly_test_tools import WINDOWS

import logging
logger = logging.getLogger()

@pytest.mark.skipif(not WINDOWS, reason="PySide2 only works on windows currently")
@pytest.mark.parametrize('project', [''])  # Workspace wants a project, but this test is not project dependent
def test_add_pyside_environment(workspace):
    import_failed = False
    try:
        import PySide2
    except ImportError:
        import_failed = True

    if not import_failed:
        cur_path = sys.path
        logger.warning(f"Expected to fail initial import but passed.  Sys path was {cur_path}")

    assert is_pyside_ready() is False, "Expected pyside not to be initialized yet"
    add_pyside_environment(workspace.paths.build_directory())
    assert is_pyside_ready() is True, "Expected pyside to be initialized yet"

    try:
        import PySide2
        if not is_configuration_valid(workspace):
            return
        from PySide2.QtWidgets import QApplication
    except ImportError as e:
        assert False, f"Failed to import PySide2 with error {e}"
    try:
        from PySide2.QtWidgets import QApplication
    except ImportError as e:
        assert False, f"Failed to import QApplication from PySide2.QtWidgets with error {e}"

