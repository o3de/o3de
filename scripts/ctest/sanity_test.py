"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT


Sanity tests to verify whether suite filtering is hooked up
"""
import pytest

def test_Sanity_Untagged_Pass(): # should happen in main pass also
    pass

@pytest.mark.SUITE_smoke
def test_Sanity_Smoke_Pass():
    pass

@pytest.mark.SUITE_main
def test_Sanity_Main_Pass(): # should happen in main pass
    pass

@pytest.mark.SUITE_periodic
def test_Sanity_Periodic_Pass():
    pass

@pytest.mark.SUITE_benchmark
def test_Sanity_Benchmark_Pass():
    pass

@pytest.mark.SUITE_sandbox
def test_Sanity_Sandbox_Pass():
    pass

@pytest.mark.SUITE_awsi
def test_Sanity_AWSI_Pass():
    pass

@pytest.mark.REQUIRES_gpu
def test_Sanity_RequireGpu_Pass():
    pass

@pytest.mark.skip
def test_Insanity():
    raise RuntimeError