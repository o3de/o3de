"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Pytest fixture for standardizing key external project file locations.
Houses a mock workspace for the external project. Only has enough information
to satisfy a ly_test_tools.o3de.asset_processor.AssetProcessor object.

"""

import pytest
import os
import tempfile
from typing import Dict


@pytest.fixture
def ap_external_project_setup_fixture(request, workspace) -> Dict:
    """
    Fixture for setting up important directories associated with creating a new project and running the AP
    """

    project_name = "ExternalProjectTest"
    bin_folder = workspace.paths._bin_dir
    external_project_dir = os.path.join(tempfile.gettempdir(), project_name)
    external_bin = os.path.join(external_project_dir, bin_folder)

    resources = {
        "project_name": project_name,
        "project_dir": external_project_dir,
        "ap_batch": "AssetProcessorBatch",
        "ap": "AssetProcessor",
        "ap_batch_path": os.path.join(external_bin, "AssetProcessorBatchLauncher"),
        "ap_path": os.path.join(external_bin, "AssetProcessorLauncher"),
    }

    # Set up a "mock" workspace for the external project to use with
    # ly_test_tools.asset_processor python object
    # asset_processor.py only uses platform and paths to asset processor executables and config file.
    # If it looks like a workspace and quacks like a workspace then it's a workspace, right?
    class mock:
        pass

    mock_workspace = mock()
    mock_workspace.platform = workspace.platform
    paths = mock()
    paths.asset_processor = lambda: resources["ap_path"]
    paths.asset_processor_batch = lambda: resources["ap_batch_path"]
    paths.asset_processor_config_file = lambda: os.path.join(resources["project_dir"], "AssetProcessorConfig.setreg")
    mock_workspace.paths = paths
    resources["external_workspace"] = mock_workspace

    return resources
