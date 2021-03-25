"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C16798661: Component List Contents
https://testrail.agscollab.com/index.php?/cases/view/16798661
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as EntityId
import azlmbr.math as math
from PySide2 import QtWidgets, QtTest, QtCore
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class ComponentListContentsTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="component_list_contents", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Select an entity. Click "Add Component" in the Entity Inspector.
        Verify components are sorted into categories.
        Expand / collapse component categories.

        Expected Behavior:
        Component categories are expanded / collapsed as expected.

        Test Steps:
        1) Open level
        2) Create entity
        3) Select the newly created entity
        4) Click "Add Component" in the Entity Inspector.
        5) Verify list of components are sorted into categories.
        6) Collapse the first category.
        7) Expand the category which was just collapsed.
        8) Add a valid component to the entity.

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # Make sure the Entity Inspector is open
        general.open_pane('Entity Inspector')

        # 2) Create entity
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
        )
        if entity_id.IsValid():
            print("Entity Created")

        # 3) Select the newly created entity
        general.clear_selection()
        general.select_object("Entity2")

        # Get the component type ID for our Box Shape
        component_name = "Box Shape"
        type_ids = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', [component_name],
                                                EntityId.EntityType().Game)
        component_type_id = type_ids[0]

        # Sanity check to make sure our Entity didn't already have the Box Shape component for some reason
        had_component = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entity_id, component_type_id)
        if had_component:
            print("Box Shape already existed on Entity.")

        # 4) Click Add Component
        general.idle_wait(0.5)
        add_comp_btn = pyside_utils.find_child_by_hierarchy(
            pyside_utils.get_editor_main_window(), ..., "Entity Inspector", ..., "m_addComponentButton")
        add_comp_btn.click()
        # Wait 0.5s to ensure component is added and rendered, especially when adding components
        # continuously to the entity
        general.idle_wait(0.5)

        popup = None
        search_frame = None
        def hasPopup():
            nonlocal popup
            nonlocal search_frame
            popup = QtWidgets.QApplication.activePopupWidget()
            focus = QtWidgets.QApplication.focusWidget()
            if isinstance(focus, QtWidgets.QLineEdit) and popup:
                search_frame = popup.findChild(QtWidgets.QFrame, "SearchFrame")
                if search_frame is not None:
                    return True
        await pyside_utils.wait_for_condition(hasPopup)

        # To ensure we get an updated tree after the component name is set
        general.idle_wait(0)
        tree = popup.findChild(QtWidgets.QTreeView, "Tree")
        QtTest.QTest.keyClick(tree, Qt.Key_Down, Qt.NoModifier)
        while tree.indexBelow(tree.currentIndex()) != QtCore.QModelIndex():
            # 6) Collapse the first category (which is "AI")
            # "Behavior Tree" is the first component in category "AI"
            if tree.currentIndex().data(Qt.DisplayRole) == "Behavior Tree":
                QtTest.QTest.keyClick(tree, Qt.Key_Left, Qt.NoModifier)
                QtTest.QTest.keyClick(tree, Qt.Key_Left, Qt.NoModifier)
                if tree.currentIndex().data(Qt.DisplayRole) == "AI":
                    print("Successfully collapsed category.")

                    # 7) Expand the category which was just collapsed.
                    QtTest.QTest.keyClick(tree, Qt.Key_Right, Qt.NoModifier)
                    QtTest.QTest.keyClick(tree, Qt.Key_Right, Qt.NoModifier)
                    if tree.currentIndex().data(Qt.DisplayRole) == "Behavior Tree":
                        print("Successfully expanded category.")
                        break

            QtTest.QTest.keyClick(tree, Qt.Key_Down, Qt.NoModifier)

        # 8) Add the component (based on passed "component_name" variable) to entity
        component_index = pyside_utils.find_child_by_pattern(tree, component_name)
        tree.setCurrentIndex(component_index)
        QtTest.QTest.keyClick(tree, Qt.Key_Enter, Qt.NoModifier)

        # After selecting the Box Shape from the component palette, make sure the component exists on our Entity now
        has_component = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entity_id, component_type_id)
        if has_component:
            print("Box Shape successfully added.")

test = ComponentListContentsTest()
test.run()
