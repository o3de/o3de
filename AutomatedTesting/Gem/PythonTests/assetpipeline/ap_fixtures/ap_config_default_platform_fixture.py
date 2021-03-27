"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Fixture for backing up and restoring dev/AssetProcessorPlatformConfig.ini

"""

# Import builtin libraries
import pytest

from . import asset_processor_fixture as asset_processor
from . import ap_config_backup_fixture as ap_config_backup_fixture

@pytest.fixture
def ap_config_default_platform_fixture(request, workspace, asset_processor, ap_config_backup_fixture) -> None:
    asset_processor.disable_all_asset_processor_platforms()
