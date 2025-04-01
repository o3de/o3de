"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Fixture for backing up and restoring <EngineRoot>\Registry\AssetProcessorPlatformConfig.setreg

"""

# Import builtin libraries
import pytest


@pytest.fixture
def ap_config_backup_fixture(request, workspace) -> None:

    workspace.settings._backup_settings(workspace.paths.asset_processor_config_file(), None)

    def teardown():
        workspace.settings._restore_settings(workspace.paths.asset_processor_config_file(), None)

    request.addfinalizer(teardown)
