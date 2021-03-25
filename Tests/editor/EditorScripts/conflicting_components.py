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
16929884: Conflicting Components
https://testrail.agscollab.com/index.php?/cases/view/16929884
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.editor as editor
import azlmbr.bus as bus
import Tests.ly_shared.pyside_utils as pyside_utils
import azlmbr.entity as entity

from azlmbr.entity import EntityId
from PySide2 import QtWidgets, QtTest, QtCore
from PySide2.QtCore import Qt
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestConflictingComponents(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="conflicting_components: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Create conflicting components on an entity, test activating and deactivated each.

        Expected Behavior:
        Conflicting compoennts are deactivated when appropriate.

        Test Steps:
        1) Create new level
        2) Create an entity
        3) Create a box shape component on the entity
        4) Create a cylinder shape component on the entity
        5) Check both components have warnings and activate/delete buttons
        6) Select Activate on the cylinder shape
        7) Check that the cylinder is active and the box is not active
        8) Right click on the box shape and enable it
        9) Check delete/activate options exist on both components
        10) delete the cylinder shape
        11) Check that the cylinder is deleted and the box is enabled

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def get_component_type_id(component_type_name):
            # Generate List of Component Types
            component_list = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentTypeNameList')

            if len(component_list) == 0:
                print("Component List returned incorrectly.")
                return False, 0

            # Get Component Types for Mesh and Comment
            type_ids_list = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType',
                                                         [component_type_name], entity.EntityType().Game)

            if len(type_ids_list) == 0:
                print("Type Ids List returned incorrectly.")
                return False, 0

            return True, type_ids_list[0]

        def create_component_of_type(entity_id, component_type_name):
            result, component_type_id = get_component_type_id(component_type_name)
            if not result:
                print("Failed to get " + component_type_name + " component id.")
                return None

            has_component_before_add = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entity_id,
                                                                    component_type_id)

            if not has_component_before_add:
                print("Entity does not have " + component_type_name + " component before add.")

            # Add an ActorComponent to the entity.
            component_outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entity_id,
                                                             [component_type_id])

            if component_outcome.IsSuccess():
                print(component_type_name + " component added to entity.")
            else:
                return None

            components = component_outcome.GetValue()
            component = components[0]

            has_component_after_add = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entity_id,
                                                                   component_type_id)

            if has_component_after_add and len(components) == 1:
                print("Entity has " + component_type_name + " component after add.")
            else:
                return None

            return component

        def get_inspector_component_by_index(index):
            inspector_dock = editor_window.findChild(QtWidgets.QWidget, "Entity Inspector")
            entity_inspector = inspector_dock.findChild(QtWidgets.QMainWindow)

            child_widget = entity_inspector.findChild(QtWidgets.QWidget)
            if child_widget is None:
                return False, None

            general.idle_wait(1.0)
            comp_list_contents = (
                child_widget.findChild(QtWidgets.QScrollArea, "m_componentList")
                    .findChild(QtWidgets.QWidget, "qt_scrollarea_viewport")
                    .findChild(QtWidgets.QWidget, "m_componentListContents")
            )

            if comp_list_contents is None or index >= len(comp_list_contents.children()):
                return False, None

            return True, comp_list_contents.children()[index]

        def get_box_and_cylinder_component_inspector():
            result, box_frame = get_inspector_component_by_index(2)

            result, cylinder_frame = get_inspector_component_by_index(3)

            return box_frame, cylinder_frame

        def is_just_first_component_enabled(component1, component2):
            c1_active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', component1)
            c2_active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', component2)

            if c1_active and not c2_active:
                return True

            return False

        def fail_test(msg):
            print(msg)
            print("Test Failed.")
            sys.exit()

        create_level_result = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        if not create_level_result:
            fail_test("New level failed.")

        EditorTestHelper.after_level_load(self)

        # Create a new entity.
        new_entity_id = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())

        if not new_entity_id.IsValid():
            print("Failed to create new entity.")

        # Select the entity so that it appears in the inspector.
        editor.ToolsApplicationRequestBus(bus.Broadcast, 'SetSelectedEntities', [new_entity_id])

        # Create both shape components.
        box_component = create_component_of_type(new_entity_id, "Box Shape")
        if box_component is None:
            fail_test("Failed to create Box Shape component.")

        cylinder_component = create_component_of_type(new_entity_id, "Cylinder Shape")
        if cylinder_component is None:
            fail_test("Failed to create Box Shape component.")

        # Check inspector windows exist for both shapes
        box_frame, cylinder_frame = get_box_and_cylinder_component_inspector()
        if box_frame is None:
            fail_test("Could not find inspector for box shape.")
        if cylinder_frame is None:
            fail_test("Could not find inspector for cylinder shape.")

        # Check both shape inspectors have delete and activate buttons.
        box_del_btn = pyside_utils.find_child_by_pattern(box_frame, "Delete component")
        box_activate_btn = pyside_utils.find_child_by_pattern(box_frame, "Activate this component")
        if box_del_btn is None or box_activate_btn is None:
            fail_test("Failed to find buttons on box component.")

        cylinder_del_btn = pyside_utils.find_child_by_pattern(cylinder_frame, "Delete component")
        cylinder_activate_btn = pyside_utils.find_child_by_pattern(cylinder_frame, "Activate this component")
        if cylinder_del_btn is None or cylinder_activate_btn is None:
            fail_test("Failed to find buttons on cylinder component.")

        # Check that just the box shape is enabled.
        just_box_enabled = is_just_first_component_enabled(box_component, cylinder_component)
        if not just_box_enabled:
            fail_test("Incorrectly enabled components.")

        # Press Activate on cylinder
        QtTest.QTest.mouseClick(cylinder_activate_btn, Qt.LeftButton, Qt.NoModifier)

        # Re-get the inspector windows to ensure they're up to date.
        box_frame, cylinder_frame = get_box_and_cylinder_component_inspector()

        # Check that neither shape inspector now has delete or activate buttons.

        box_del_btn = pyside_utils.find_child_by_pattern(box_frame, "Delete component")
        box_activate_btn = pyside_utils.find_child_by_pattern(box_frame, "Activate this component")
        if box_del_btn is not None or box_activate_btn is not None:
            fail_test("Found buttons on box component when disabled.")

        cylinder_del_btn = pyside_utils.find_child_by_pattern(cylinder_frame, "Delete component")
        cylinder_activate_btn = pyside_utils.find_child_by_pattern(cylinder_frame, "Activate this component")
        if cylinder_del_btn is not None or cylinder_activate_btn is not None:
            fail_test("Found buttons on cylinder component when active.")

        # Check that the cylinder is now enabled and not the box.
        just_cylinder_enabled = is_just_first_component_enabled(cylinder_component, box_component)
        if not just_cylinder_enabled:
            fail_test("Incorrectly enabled components.")

        # Click on the box frame to activate it or we might get the wrong context menu
        QtTest.QTest.mouseClick(box_frame, Qt.LeftButton, Qt.NoModifier)

        general.idle_wait(1.0)

        # Pop up box component context menu and select the Enable component option
        pyside_utils.trigger_context_menu_entry(box_frame, "Enable component")

        # Check that both components have the buttons again
        box_frame, cylinder_frame = get_box_and_cylinder_component_inspector()

        box_del_btn = pyside_utils.find_child_by_pattern(box_frame, "Delete component")
        box_activate_btn = pyside_utils.find_child_by_pattern(box_frame, "Activate this component")
        if box_del_btn is None or box_activate_btn is None:
            fail_test("Failed to find buttons on box component.")

        cylinder_del_btn = pyside_utils.find_child_by_pattern(cylinder_frame, "Delete component")
        cylinder_activate_btn = pyside_utils.find_child_by_pattern(cylinder_frame, "Activate this component")
        if cylinder_del_btn is None or cylinder_activate_btn is None:
            fail_test("Failed to find buttons on cylinder component.")

        # Press "Delete component" on the Cylinder Shape.
        QtTest.QTest.mouseClick(cylinder_del_btn, Qt.LeftButton, Qt.NoModifier)

        general.idle_wait(1.0)

        # Check it's gone.
        result, component_type_id = get_component_type_id("Cylinder Shape")
        if not result:
            fail_test("Failed to get Cylinder Shape component id.")

        has_component_after_delete = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', new_entity_id,
                                                                  component_type_id)

        if has_component_after_delete:
            fail_test("Cylinder Shape still exists.")

        box_frame, cylinder_frame = get_box_and_cylinder_component_inspector()

        # Ensure the activate and delete buttons are gone from the box inspector.
        box_del_btn = pyside_utils.find_child_by_pattern(box_frame, "Delete component")
        box_activate_btn = pyside_utils.find_child_by_pattern(box_frame, "Activate this component")
        if box_del_btn is not None or box_activate_btn is not None:
            fail_test("Found buttons on box component when active.")

        # Check the box is now enabled.
        box_active = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', box_component)
        if not box_active:
            fail_test("Box not active.")

        print("Conflicting components test successful.")


editor_window = pyside_utils.get_editor_main_window()
main_window = editor_window.findChild(QtWidgets.QMainWindow)
app = QtWidgets.QApplication.instance()
test = TestConflictingComponents()
test.run()
