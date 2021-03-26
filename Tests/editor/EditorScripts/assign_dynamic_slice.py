"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C17211213: Assign Dynamic Slice
https://testrail.agscollab.com/index.php?/cases/view/17211213
"""
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.asset as asset
import azlmbr.bus as bus
import azlmbr.entity as entity
import Tests.ly_shared.pyside_utils as pyside_utils
import Tests.ly_shared.hydra_editor_utils as editor_utils
from PySide2 import QtWidgets
from PySide2.QtTest import QTest
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class AssignDynamicSlice(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="assign dynamic slice", args=["level"])

    def run_test(self):
        """
        Summary:
            Dynamic slice can be assigned to a spawner on an entity

        Expected Behavior:
            Dynamic slice is present in the file picker, can be added to
            components, and is visible in game mode when spawned

        Test Steps:
        0) Open temporary test level
        1) Create an entity with a spawner component
        2) Add dynamic slice to the spawner component C17211213_visual_thunderbolt
        3) Set Spawn on activate to enabled
        4) Enter game mode
        5) Verify that the dynamic slice spawned and is visible in game mode

        Note:
            - This test file must be called from the Lumberyard Editor command terminal
            - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def set_dynamic_slice_by_asset_path(entity_obj, asset_path):
            asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, 'GetAssetIdByPath', asset_path, math.Uuid(), False)
            entity_obj.get_set_test(0, "Dynamic slice", asset_id)

        def set_spawn_checkbox_enabled():
            general.idle_wait(0.5)
            editor_window = pyside_utils.get_editor_main_window()
            entity_inspector = editor_window.findChildren(QtWidgets.QDockWidget, "Entity Inspector")[0]
            spawn_activate_frame = entity_inspector.findChildren(QtWidgets.QFrame, "Spawn on activate")[0]
            checkbox = spawn_activate_frame.findChildren(QtWidgets.QCheckBox)[0]
            checkbox.click()

        # 0) Open temporary test level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # 1) Create an entity with a spawner component
        entity_postition = math.Vector3(125.0, 136.0, 32.0)
        components_to_add = ["Spawner"]

        spawner_entity = editor_utils.Entity("Spawner")
        spawner_entity.create_entity(entity_postition, components_to_add)

        # 2) Add dynamic slice to the spawner component C17211213_visual_thunderbolt
        thunderbolt_path = os.path.join("slices", "Test", "C17211213_visual_thunderbolt.dynamicslice")
        set_dynamic_slice_by_asset_path(spawner_entity, thunderbolt_path)

        # 3) Set Spawn on activate to enabled
        set_spawn_checkbox_enabled()

        # 4) Enter game mode
        general.enter_game_mode()
        general.idle_wait(0.5)

        # 5) Verify that the dynamic slice spawned and is visible in game mode
        slice_name = "thunderbolt"
        thunderbolt_id = general.find_game_entity(slice_name)
        check_valid = thunderbolt_id.IsValid()
        assert check_valid, f"No entity found for {slice_name}"
        if check_valid:
            print(f"Entity found for {slice_name}")

        general.exit_game_mode()



test = AssignDynamicSlice()
test.run()
