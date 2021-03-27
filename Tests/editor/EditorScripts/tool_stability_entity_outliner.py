"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


C6308161: Tool Stability: Entity Outliner
https://testrail.agscollab.com/index.php?/cases/view/6308161
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.entity as entity
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.slice as slice
import azlmbr.asset as asset
import azlmbr.math as math
import Tests.ly_shared.pyside_utils as pyside_utils
from re import compile as regex
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class ToolStabilityEntityOutlinerTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="tool_stability_entity_outliner", args=["level"])

    def run_test(self):
        """
        Summary:
        Tool Stability: Entity Outliner - This can be verified by creating entity, layer, slice and
        instantiating a slice.

        Expected Behavior:
        1) Entities can be created in the Entity Outliner.
        2) Created entities populate in other tools properly.
        3) Entity creation can be undone and related tools are updated.
        4) Entity creation can be redone and related tools are updated.
        5) Layers can be created in the Entity Outliner.
        5) Created layers populate in the Entity Inspector properly.
        6) Slices can be created inside the Entity Outliner.
        7) Slices can be instantiated inside the Entity Outliner.

        Test Steps:
        1) Open level
        2) Create Entity and verify if it is created
        3) Undo entity creation and verify if entity is deleted
        4) Redo the last operation
        5) Create layer and verify if it is created
        6) Create slice using the first entity
        7) Instantiate an existing slice

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def get_entities_by_name(entity_name="Entity2"):
            searchFilter = entity.SearchFilter()
            searchFilter.names = [entity_name]
            entities = entity.SearchBus(bus.Broadcast, "SearchEntities", searchFilter)
            return entities

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # Get the editor window object
        editor_window = pyside_utils.get_editor_main_window()
        
        # Get the outliner widget object based on the window title
        outliner_widget = pyside_utils.find_child_by_hierarchy(
            editor_window, ..., dict(windowTitle=regex("Entity Outliner.*")), ..., "m_objectList"
        ).parent()

        # Get the object tree in the entity outliner
        tree = pyside_utils.find_child_by_hierarchy(outliner_widget, ..., "m_objectTree")

        # Get the entity inspector object based on the window title
        entity_inspector_name_editor = pyside_utils.find_child_by_hierarchy(
            editor_window, ..., dict(windowTitle=regex("Entity Inspector.*")), ..., "m_entityNameEditor"
        )

        # 2) Create Entity and verify if it is created
        general.clear_selection()

        # find Create Entity action and trigger it
        action = pyside_utils.find_child_by_pattern(outliner_widget, "Create Entity")
        action.trigger()

        # ensure entity is created
        entities = get_entities_by_name()
        print(f"New entity created: {len(entities) == 1}")

        # get the newly create entity id and check if it is valid
        entity_id = entities[0]
        print(f"Entity Id is valid: {entity_id.isValid()}")

        # check in entity inspector that the newly created entity is present by verifying
        # the name populated in the name editor in entity inspector
        general.select_object("Entity2")
        general.idle_wait(1.0)
        self.wait_for_condition(lambda: entity_inspector_name_editor.text() == "Entity2", 1.0)
        print(f"Entity name populated in Entity Inspector: {entity_inspector_name_editor.text() == 'Entity2'}")

        # 3) Undo entity creation and verify if entity is deleted
        general.undo()
        self.wait_for_condition(lambda: len(get_entities_by_name()) == 0, 3.0)
        print(f"Entity creation undone: {len(get_entities_by_name())==0}")

        # 4) Redo the last operation
        general.idle_wait(1.0)
        general.redo()
        self.wait_for_condition(lambda: len(get_entities_by_name()) == 1, 3.5)
        print(f"Entity appeared after redo: {len(get_entities_by_name())==1}")

        # 5) Create layer and verify if it is created
        general.idle_wait(2.0)
        pyside_utils.trigger_context_menu_entry(tree, "Create layer")

        # ensure entity is created
        self.wait_for_condition(lambda: len(get_entities_by_name("Layer3*")) == 1, 1.0)
        print(f"New layer created: {len(get_entities_by_name('Layer3*'))==1}")
        
        # check in entity inspector that the newly created entity is present by verifying
        # the name populated in the name editor in entity inspector
        general.clear_selection()
        general.select_object("Layer3*")
        self.wait_for_condition(lambda: entity_inspector_name_editor.text() == "Layer3", 1.0)
        print(f"Layer name populated in Entity Inspector: {entity_inspector_name_editor.text() == 'Layer3'}")

        # 6) Create slice using the first entity
        current_level_name = editor.EditorToolsApplicationRequestBus(bus.Broadcast, "GetCurrentLevelName")
        slice_path = os.path.join("Levels", current_level_name, "TestSlice.slice")
        slice_created = slice.SliceRequestBus(bus.Broadcast, "CreateNewSlice", entity_id, slice_path)
        print(f"Slice created successfully: {slice_created}")

        # 7) Instantiate an existing slice
        # ensure we clear the selection so that the newly instantiated slice is selected after instantiating
        # so that we can verify the name of the object in the entity inspector
        general.clear_selection()
        slice_path = os.path.join("EngineAssets", "Slices", "DefaultLevelSetup.slice")
        asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", slice_path, math.Uuid(), False)
        transform = math.Transform_CreateIdentity()
        position = math.Vector3(64.0, 64.0, 32.0)
        transform.SetPosition(position)
        slice.SliceRequestBus(bus.Broadcast, "InstantiateSliceFromAssetId", asset_id, transform)
        self.wait_for_condition(lambda: entity_inspector_name_editor.text() == "DefaultLevelSetup", 2.0)
        print(
            f"Instantiated slice populated in Entity Inspector: {entity_inspector_name_editor.text() == 'DefaultLevelSetup'}"
        )


test = ToolStabilityEntityOutlinerTest()
test.run()
