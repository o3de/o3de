"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os
import psutil
import sys

import pytest

import editor_python_test_tools.hydra_test_utils as hydra
from ly_test_tools.benchmark.data_aggregator import BenchmarkDataAggregator

logger = logging.getLogger(__name__)
WINDOWS = sys.platform.startswith('win')

if not WINDOWS:
    pytestmark = pytest.mark.skipif(
        not WINDOWS,
        reason="TestSuite_Benchmark_GPU.py currently only runs on Windows")


def filebeat_service_running():
    """
    Checks if the filebeat service is currently running on the OS.
    :return: True if filebeat service detected and running, False otherwise.
    """
    result = False
    try:
        filebeat_service = psutil.win_service_get('filebeat')
        filebeat_service_info = filebeat_service.as_dict()
        if filebeat_service_info['status'] == 'running':
            result = True
    except psutil.NoSuchProcess:
        return result

    return result


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ["windows_editor"])
@pytest.mark.parametrize("level", ["AtomFeatureIntegrationBenchmark"])
class TestPerformanceBenchmarkSuite(object):

    @pytest.mark.parametrize('rhi', ['-rhi=dx12'])
    def test_AtomFeatureIntegrationBenchmarkTest_GatherBenchmarkMetrics_DX12(
            self, request, editor, workspace, rhi, project, launcher_platform, level):
        """
        Please review the hydra script run by this test for more specific test info.
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
            cfg_args=[level, rhi],
            null_renderer=False,
        )

    @pytest.mark.skipif(not filebeat_service_running(), reason="filebeat service not running")
    def test_AtomFeatureIntegrationBenchmarkTest_SendBenchmarkMetrics_DX12(
            self, request, editor, workspace, project, launcher_platform, level):
        """
        Gathers the DX12 benchmark metrics and uses filebeat to send the metrics data.
        """
        aggregator = BenchmarkDataAggregator(workspace, logger, 'main_gpu')
        aggregator.upload_metrics('dx12')

    @pytest.mark.parametrize('rhi', ['-rhi=Vulkan'])
    def test_AtomFeatureIntegrationBenchmarkTest_GatherBenchmarkMetrics_Vulkan(
            self, request, editor, workspace, rhi, project, launcher_platform, level):
        """
        Please review the hydra script run by this test for more specific test info.
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
            cfg_args=[level, rhi],
            null_renderer=False,
        )

    @pytest.mark.skipif(not filebeat_service_running(), reason="filebeat service not running")
    def test_AtomFeatureIntegrationBenchmarkTest_SendBenchmarkMetrics_Vulkan(
            self, request, editor, workspace, project, launcher_platform, level):
        """
        Gathers the Vulkan benchmark metrics and uses filebeat to send the metrics data.
        """
        aggregator = BenchmarkDataAggregator(workspace, logger, 'main_gpu')
        aggregator.upload_metrics('Vulkan')
