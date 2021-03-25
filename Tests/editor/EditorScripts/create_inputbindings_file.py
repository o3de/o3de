"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
C1506874: Create Input Bindings File
https://testrail.agscollab.com/index.php?/cases/view/1506874
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.asset as asset
import azlmbr.editor as editor
import azlmbr.bus as bus
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class CreateInputBindingFileTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="create_inputbindings_file", args=["level"])

    def run_test(self):
        """
        Summary:
        Create a new inputbinding file

        Expected Behavior:
        The newly created inputbindings file is loaded into the Asset Editor.

        Test Steps:
        1) Open a new level
        2) Create new inputbindings file
        3) Verify if file is created

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        INPUTBINDING_FILE_PATH = "SamplesProject/temp.inputbindings"

        # 1) Open a new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        # 2) Create new inputbindings file
        input_bindings_type_id = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetTypeByDisplayName", "Input Bindings"
        )
        editor.AssetEditorRequestsBus(bus.Broadcast, "CreateNewAsset", input_bindings_type_id)
        editor.AssetEditorWidgetRequestsBus(bus.Broadcast, "SaveAssetAs", INPUTBINDING_FILE_PATH)

        # 3) Verify if file is created
        self.wait_for_condition(lambda: os.path.exists(INPUTBINDING_FILE_PATH), 0.5)
        print(f"New inputbindings file created: {os.path.exists(INPUTBINDING_FILE_PATH)}")


test = CreateInputBindingFileTest()
test.run_test()
