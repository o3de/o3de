# """
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.

# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

# Conftest file for providing additional configuration for Screenshot Comparison tests
# """



####################################
# Commented out due to need to shift to new LyTestTools, Python3 and new screenshot workflow
# Don't merge to Mainline
####################################


# import pytest

# def pytest_addoption(parser):
    # parser.addoption(
        # '--graphics_vendor', action='store', help='graphics vendor name: nvidia or amd', required=True
    # )
    # parser.addoption(
        # '--upload_results_to_s3', action='store_true', default=False, help='Specify if you need to upload screenshot artifacts to s3'
    # )

# @pytest.fixture
# def graphics_vendor(request):
    # return request.config.getoption('--graphics_vendor')

# @pytest.fixture
# def upload_results_to_s3(request):
    # return request.config.getoption('--upload_results_to_s3')

