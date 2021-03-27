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
C6384949: Basic Workflow: Layers
https://testrail.agscollab.com/index.php?/cases/view/6384949
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
import Tests.ly_shared.hydra_editor_utils as editor_utils
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.layers as layers
import azlmbr.math as math
from typing import Optional
from PySide2 import QtWidgets
from PySide2.QtGui import QContextMenuEvent
from PySide2.QtCore import QObject, QEvent, QPoint
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestBasicWorkflowLayers(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="basic_workflow_layers: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Basic Workflow: Layers.

        Test Steps:
        1) Open level
        2) Create layer in the Entity Outliner
        3) Create entity and Drag the entity into the layer
        4) Create another entity and Drag entities out of the layer
        5) Create a second layer and Drag the second layer into the first layer
        6) Save layer

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def get_entity_count_name(entity_name):
            searchFilter = entity.SearchFilter()
            searchFilter.names = [entity_name]
            entities = entity.SearchBus(bus.Broadcast, "SearchEntities", searchFilter)
            return len(entities)

        def verify_parent(
            entity_to_check, expected_parent_entity, child_name, parent_name, additional_pretext: Optional[str] = ""
        ):
            actual_parent_id = editor.EditorEntityInfoRequestBus(bus.Event, "GetParent", entity_to_check)
            if actual_parent_id.ToString() == expected_parent_entity.ToString():
                print(f"{additional_pretext} The parent entity of {child_name} is : {parent_name}")

        def set_parent(chilld_entity, parent_entity):
            editor.EditorEntityAPIBus(bus.Event, "SetParent", chilld_entity, parent_entity)

        class EventFilter(QObject):
            def eventFilter(self, obj, event):
                if event.type() == QEvent.Type.ChildPolished:
                    print("ChildPolished event received")
                    menu = obj.children()[-1]
                    action = pyside_utils.find_child_by_property(menu, QtWidgets.QAction, "text", "Save layer")
                    action.trigger()
                    menu.clear()
                    return False
                return False

        def on_focus_changed(old, new):
            active_modal_widget = QtWidgets.QApplication.activeModalWidget()
            general.idle_wait(1.0)
            button_box = active_modal_widget.findChild(QtWidgets.QDialogButtonBox, "qt_msgbox_buttonbox")
            button_box.button(QtWidgets.QDialogButtonBox.Save).click()

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # 2) Create layer in the Entity Outliner
        first_layer_entity_id = layers.EditorLayerComponent_CreateLayerEntityFromName("Layer1")
        if get_entity_count_name("Layer1*"):
            print("First Layer is created")

        # 3) Create entity and Drag the entity into the layer
        entity_postition = math.Vector3(125.0, 136.0, 32.0)
        first_entity = editor_utils.Entity("Entity1")
        first_entity.create_entity(entity_postition, [])
        if get_entity_count_name("Entity1"):
            print("First Entity is created")
        set_parent(first_entity.id, first_layer_entity_id)
        verify_parent(first_entity.id, first_layer_entity_id, first_entity.name, "Layer1", "Original Check:")

        # 4) Create another entity and Drag entities out of the layer
        second_entity = editor_utils.Entity("Entity2")
        second_entity.create_entity(entity_postition, [])
        if get_entity_count_name("Entity2"):
            print("Second Entity is created")
        set_parent(second_entity.id, first_layer_entity_id)
        verify_parent(second_entity.id, first_layer_entity_id, second_entity.name, "Layer1", "Original Check:")
        # Drag entities out of the layer
        set_parent(first_entity.id, entity.EntityId())
        set_parent(second_entity.id, entity.EntityId())
        verify_parent(first_entity.id, entity.EntityId(), first_entity.name, "", "After Drag:")
        verify_parent(second_entity.id, entity.EntityId(), second_entity.name, "", "After Drag:")

        # 5) Create a second layer and Drag the second layer into the first layer
        second_layer_entity_id = layers.EditorLayerComponent_CreateLayerEntityFromName("Layer2")
        if get_entity_count_name("Layer2*"):
            print("Second Layer is created")
        set_parent(second_layer_entity_id, first_layer_entity_id)
        verify_parent(second_layer_entity_id, first_layer_entity_id, "Layer2", "Layer1", "After Drag:")

        # 6) Save layer
        app = QtWidgets.QApplication.instance()
        editor_window = pyside_utils.get_editor_main_window()
        main_window = editor_window.findChild(QtWidgets.QMainWindow)
        entity_outliner = main_window.findChild(QtWidgets.QDockWidget, "Entity Outliner (PREVIEW)")
        outliner_main_window = entity_outliner.findChild(QtWidgets.QMainWindow)
        wid_1 = outliner_main_window.findChild(QtWidgets.QWidget)
        wid_2 = wid_1.findChild(QtWidgets.QWidget)
        tree = outliner_main_window.findChildren(QtWidgets.QTreeView, "m_objectTree")[0]
        general.clear_selection()
        general.select_object(editor.EditorEntityInfoRequestBus(bus.Event, "GetName", first_layer_entity_id))
        event_filter = EventFilter()
        try:
            app.focusChanged.connect(on_focus_changed)
            wid_2.installEventFilter(event_filter)
            context_menu_event = QContextMenuEvent(QContextMenuEvent.Mouse, QPoint(0, 0))
            app.sendEvent(tree, context_menu_event)
            general.idle_wait(1.0)
        except Exception as e:
            print(e)
        finally:
            app.focusChanged.disconnect(on_focus_changed)
            wid_2.removeEventFilter(event_filter)


test = TestBasicWorkflowLayers()
test.run()
