"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

# This suite consists of all test cases that are passing and have been verified.
# Using batched mode (EditorBatchedTest) to run the tests sequentially because
# when they run in parallel mode (EditorParallelTest or EditorSharedTest) the
# editor crashes randomly.

import pytest
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorBatchedTest

@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):
    class test_ActorSplitsAfterCollision(EditorBatchedTest):
        from .tests import Blast_ActorSplitsAfterCollision as test_module

    class test_ActorSplitsAfterRadialDamage(EditorBatchedTest):
        from .tests import Blast_ActorSplitsAfterRadialDamage as test_module

    class test_ActorSplitsAfterCapsuleDamage(EditorBatchedTest):
        from .tests import Blast_ActorSplitsAfterCapsuleDamage as test_module

    class test_ActorSplitsAfterImpactSpreadDamage(EditorBatchedTest):
        from .tests import Blast_ActorSplitsAfterImpactSpreadDamage as test_module

    class test_ActorSplitsAfterShearDamage(EditorBatchedTest):
        from .tests import Blast_ActorSplitsAfterShearDamage as test_module

    class test_ActorSplitsAfterTriangleDamage(EditorBatchedTest):
        from .tests import Blast_ActorSplitsAfterTriangleDamage as test_module

    class test_ActorSplitsAfterStressDamage(EditorBatchedTest):
        from .tests import Blast_ActorSplitsAfterStressDamage as test_module
