"""
Copyright (c) Contributors to the Open 3D Engine Project

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest
import os
import sys

from ly_test_tools import LAUNCHERS
from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorSharedTest, EditorTestSuite

from base import TestAutomationBase
from editor_test import EditorSingleTest, EditorSharedTest, EditorTestSuite


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    class test_C100000(EditorSharedTest):
        from . import C100000_RigidBody_EnablingGravityWorksPoC as test_module

    class test_C111111(EditorSharedTest):
        from . import C111111_RigidBody_EnablingGravityWorksUsingNotificationsPoC as test_module