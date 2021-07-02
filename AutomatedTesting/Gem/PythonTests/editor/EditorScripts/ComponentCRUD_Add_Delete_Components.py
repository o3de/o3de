"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
C16929880: Add Delete Components
"""

import os
import sys
from PySide2 import QtWidgets, QtTest, QtCore
from PySide2.QtCore import Qt

import azlmbr.legacy.general as general
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.math as math
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
import editor_python_test_tools.hydra_editor_utils as hydra
import editor_python_test_tools.pyside_utils as pyside_utils
from editor_python_test_tools.editor_test_helper import EditorTestHelper


class AddDeleteComponentsTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="ComponentCRUD_Add_Delete_Components", args=["level"])

    @pyside_utils.wrap_async
    async def run_test(self):
        """
        Summary:
        Add/Delete Components to an entity.

        Expected Behavior:
        1) Components can be added to an entity.
        2) Multiple components can be added to a single entity.
        3) Components can be selected in the Entity Inspector.
        4) Components can be deleted from entities.
        5) Component deletion can be undone.

        Test Steps:
        1) Open level
        2) Create entity
        3) Select the newly created entity
        4) Add/verify Box Shape Component
        5) Add/verify Mesh component
        6) Delete Mesh Component
        7) Undo deletion of component

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        async def add_component(component_name):
            pyside_utils.click_button_async(add_comp_btn)
            popup = await pyside_utils.wait_for_popup_widget()
            tree = popup.findChild(QtWidgets.QTreeView, "Tree")
            component_index = pyside_utils.find_child_by_pattern(tree, component_name)
            if component_index.isValid():
                print(f"{component_name} found")
            tree.expand(component_index)
            tree.setCurrentIndex(component_index)
            QtTest.QTest.keyClick(tree, Qt.Key_Enter, Qt.NoModifier)

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=False,
        )

        # 2) Create entity
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, entity.EntityId()
        )
        if entity_id.IsValid():
            print("Entity Created")

        # 3) Select the newly created entity
        general.select_object("Entity2")

        # Give the Entity Inspector time to fully create its contents
        general.idle_wait(0.5)

        # 4) Add/verify Box Shape Component
        editor_window = pyside_utils.get_editor_main_window()
        entity_inspector = editor_window.findChild(QtWidgets.QDockWidget, "Entity Inspector")
        add_comp_btn = entity_inspector.findChild(QtWidgets.QPushButton, "m_addComponentButton")
        await add_component("Box Shape")
        print(f"Box Shape Component added: {hydra.has_components(entity_id, ['Box Shape'])}")

        # 5) Add/verify Mesh component
        await add_component("Mesh")
        print(f"Mesh Component added: {hydra.has_components(entity_id, ['Mesh'])}")

        # 6) Delete Mesh Component
        general.idle_wait(0.5)
        comp_list_contents = entity_inspector.findChild(QtWidgets.QWidget, "m_componentListContents")
        await pyside_utils.wait_for_condition(lambda: len(comp_list_contents.children()) > 3)
        # Mesh Component is the 3rd component added to the entity including the default Transform component
        # There is one QVBoxLayout child before the QFrames, making the Mesh component the 4th child in the list
        mesh_frame = comp_list_contents.children()[3]
        QtTest.QTest.mouseClick(mesh_frame, Qt.LeftButton, Qt.NoModifier)
        QtTest.QTest.keyClick(mesh_frame, Qt.Key_Delete, Qt.NoModifier)
        success = await pyside_utils.wait_for_condition(lambda: not hydra.has_components(entity_id, ['Mesh']), 5.0)
        if success:
            print(f"Mesh Component deleted: {not hydra.has_components(entity_id, ['Mesh'])}")

        # 7) Undo deletion of component
        QtTest.QTest.keyPress(entity_inspector, Qt.Key_Z, Qt.ControlModifier)
        success = await pyside_utils.wait_for_condition(lambda: hydra.has_components(entity_id, ['Mesh']), 5.0)
        if success:
            print(f"Mesh Component deletion undone: {hydra.has_components(entity_id, ['Mesh'])}")


test = AddDeleteComponentsTest()
test.run()
