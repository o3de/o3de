"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Fixture for backing up and restoring dev/AssetProcessorPlatformConfig.ini

"""

# Import builtin libraries
import pytest

from . import asset_processor_fixture as asset_processor
from . import ap_config_backup_fixture as ap_config_backup_fixture

@pytest.fixture
def ap_config_default_platform_fixture(request, workspace, asset_processor, ap_config_backup_fixture) -> None:
    asset_processor.disable_all_asset_processor_platforms()
