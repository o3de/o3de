# -*- coding: utf-8 -*-

"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

This is a file to test basic functionality of the Base Viewer executable
"""

import os
import pytest
import subprocess
import time
import re
from ly_test_tools.environment.process_utils import kill_processes_named as kill_processes_named

dev_dir = os.path.abspath(os.path.join(os.path.abspath(__file__), '..', '..', '..', '..'))
bin_dir = 'Bin64vc141'


def gather_sample_names():
    """
    Gathers the currently eligible samples from the output of a single run of baseviewer.exe (with no sample argument).
    For use in the fixture parameters.
    """
    viewer_dir = os.path.join(dev_dir, bin_dir)
    os.chdir(viewer_dir)
    process = subprocess.Popen(['BaseViewer.exe', '-timeout', '5'], stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
    out = process.communicate()[0]
    log = out.splitlines()
    samples = []
    for line in log:
        line = str(line)
        if "SampleComponentManager" in line and '-' not in line and 'Not Supported' not in line:
            line_regexp = re.search('\[.*\]', line)
            line = line_regexp.group(0)
            sample = line.replace('[', '').replace(']', '')
            samples.append(sample)
    kill_processes_named('AssetProcessor', ignore_extensions=True)
    if samples is not None:
        return samples


@pytest.fixture
def kill_AP(request):
    def teardown():
        kill_processes_named('AssetProcessor', ignore_extensions=True)
    request.addfinalizer(teardown)


@pytest.mark.parametrize("samples", gather_sample_names())
class TestBaseViewerExe(object):

    def test_OpenSampleLevel_CorrectFormat_ShouldPass(self, samples, kill_AP):
        """
        Opens the specific BaseViewer samples individually and verifies they're stable for a few seconds and then exit
        cleanly
        """
        viewer_dir = os.path.join(dev_dir, bin_dir)
        os.chdir(viewer_dir)
        return_code = subprocess.check_call(['BaseViewer.exe', '-sample', samples, '-timeout', '20'], timeout=30)
        assert return_code == 0, "Sample '{}' did not exit properly with code '{}'".format(samples, str(returncode))

    def test_OpenSampleLevel_NoErrors_ShouldPass(self, samples, kill_AP):
        """
        Opens the specific BaseViewer samples individually and verifies there are no errors in the output while running
        """
        viewer_dir = os.path.join(dev_dir, bin_dir)
        os.chdir(viewer_dir)
        output = subprocess.check_output(['BaseViewer.exe', '-sample', samples, '-timeout', '20'], timeout=30)
        log = output.splitlines()
        errors = []
        assertions = []
        for i in range(len(log)):
            line = str(log[i])
            surrounding_lines = str(log[i:i+3])
            if "Trace::Error" in line:
                errors.append(surrounding_lines)
            if "Trace::Assert" in line:
                assertions.append(surrounding_lines)

        assert len(errors) == 0, "Sample '{}' had the following errors when run: {}".format(samples, "\n".join(errors))
        assert len(assertions) == 0, "Sample '{}' had the following assertions when run: {}".format(samples, "\n".join(assertions))

