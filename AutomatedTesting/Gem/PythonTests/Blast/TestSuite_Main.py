"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

# This suite consists of all test cases that are passing and have been verified.

import pytest
import os
import sys

from ly_test_tools import LAUNCHERS
sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase

@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(TestAutomationBase):
    def test_ActorSplitsAfterCollision(self, request, workspace, editor, launcher_platform):
        from .tests import Blast_ActorSplitsAfterCollision as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ActorSplitsAfterRadialDamage(self, request, workspace, editor, launcher_platform):
        from .tests import Blast_ActorSplitsAfterRadialDamage as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ActorSplitsAfterCapsuleDamage(self, request, workspace, editor, launcher_platform):
        from .tests import Blast_ActorSplitsAfterCapsuleDamage as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ActorSplitsAfterImpactSpreadDamage(self, request, workspace, editor, launcher_platform):
        from .tests import Blast_ActorSplitsAfterImpactSpreadDamage as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ActorSplitsAfterShearDamage(self, request, workspace, editor, launcher_platform):
        from .tests import Blast_ActorSplitsAfterShearDamage as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ActorSplitsAfterTriangleDamage(self, request, workspace, editor, launcher_platform):
        from .tests import Blast_ActorSplitsAfterTriangleDamage as test_module
        self._run_test(request, workspace, editor, test_module)

    def test_ActorSplitsAfterStressDamage(self, request, workspace, editor, launcher_platform):
        from .tests import Blast_ActorSplitsAfterStressDamage as test_module
        self._run_test(request, workspace, editor, test_module)
