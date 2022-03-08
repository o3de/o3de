"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os

import pytest

import editor_python_test_tools.hydra_test_utils as hydra
from ly_test_tools.benchmark.data_aggregator import BenchmarkDataAggregator

logger = logging.getLogger(__name__)


@pytest.mark.parametrize('rhi', ['dx12', 'vulkan'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ["windows_editor"])
@pytest.mark.parametrize("level", ["AtomFeatureIntegrationBenchmark"])
class TestPerformanceBenchmarkSuite(object):
    def test_AtomFeatureIntegrationBenchmarkTest_UploadMetrics(
            self, request, editor, workspace, rhi, project, launcher_platform, level):
        """
        Please review the hydra script run by this test for more specific test info.
        Tests the performance of the Simple level.
        """
        expected_lines = [
            "Benchmark metadata captured.",
            "Pass timestamps captured.",
            "CPU frame time captured.",
            "Captured data successfully.",
            "Exited game mode"
        ]

        unexpected_lines = [
            "Failed to capture data.",
            "Failed to capture pass timestamps.",
            "Failed to capture CPU frame time.",
            "Failed to capture benchmark metadata."
        ]

        hydra.launch_and_validate_results(
            request,
            os.path.join(os.path.dirname(__file__), "tests"),
            editor,
            "hydra_GPUTest_AtomFeatureIntegrationBenchmark.py",
            timeout=600,
            expected_lines=expected_lines,
            unexpected_lines=unexpected_lines,
            halt_on_unexpected=True,
            cfg_args=[level],
            null_renderer=False,
        )

        aggregator = BenchmarkDataAggregator(workspace, logger, 'periodic')
        aggregator.upload_metrics(rhi)
