"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    entity_created = (
        "Entity created successfully",
        "Failed to create entity"
    )
    box_component_added = (
        "Box Shape component added to entity",
        "Failed to add Box Shape component to entity"
    )
    mesh_component_added = (
        "Mesh component added to entity",
        "Failed to add Mesh component to entity"
    )
    mesh_component_deleted = (
        "Mesh component removed from entity",
        "Failed to remove Mesh component from entity"
    )
    mesh_component_delete_undo = (
        "Mesh component removal was successfully undone",
        "Failed to undo Mesh component removal"
    )


def ComponentCRUD_Add_Delete_Components():

    import pyside_utils

    @pyside_utils.wrap_async
    async def run_test():
        """
        Summary:
        Add/Delete Components to/from an entity.

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

        from PySide2 import QtWidgets, QtTest
        from PySide2.QtCore import Qt

        import azlmbr.legacy.general as general
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.entity as entity
        import azlmbr.math as math

        import editor_python_test_tools.hydra_editor_utils as hydra
        from editor_python_test_tools.utils import Report
        from editor_python_test_tools.wait_utils import PrefabWaiter

        async def add_component(component_name):
            pyside_utils.click_button_async(add_comp_btn)
            popup = await pyside_utils.wait_for_popup_widget()
            tree = popup.findChild(QtWidgets.QTreeView, "Tree")
            component_index = pyside_utils.find_child_by_pattern(tree, component_name)
            # Handle Mesh component which has both a named section of components along with a named Mesh component
            if component_name == "Mesh":
                mesh_component_index = pyside_utils.find_child_by_pattern(component_index, component_name)
                if mesh_component_index.isValid():
                    Report.info(f"{component_name} found")
                tree.expand(mesh_component_index)
                tree.setCurrentIndex(mesh_component_index)
                QtTest.QTest.keyClick(tree, Qt.Key_Enter, Qt.NoModifier)
            # Handle other components
            if component_index.isValid():
                Report.info(f"{component_name} found")
            tree.expand(component_index)
            tree.setCurrentIndex(component_index)
            QtTest.QTest.keyClick(tree, Qt.Key_Enter, Qt.NoModifier)
        
        def select_entity_by_name(entity_name):
            searchFilter = entity.SearchFilter()
            searchFilter.names = [entity_name]
            entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)
            editor.ToolsApplicationRequestBus(bus.Broadcast, 'MarkEntitySelected', entities[0])

        # 1) Open an existing simple level
        hydra.open_base_level()

        # 2) Create entity
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, entity.EntityId()
        )
        Report.critical_result(Tests.entity_created, entity_id.IsValid())

        # 3) Select the newly created entity
        select_entity_by_name("Entity1")

        # Give the Entity Inspector time to fully create its contents
        general.idle_wait_frames(3)

        # 4) Add/verify Box Shape Component
        editor_window = pyside_utils.get_editor_main_window()
        entity_inspector = editor_window.findChild(QtWidgets.QDockWidget, "Inspector")
        add_comp_btn = entity_inspector.findChild(QtWidgets.QPushButton, "m_addComponentButton")
        await add_component("Box Shape")
        Report.result(Tests.box_component_added, hydra.has_components(entity_id, ['Box Shape']))

        # 5) Add/verify Mesh component
        await add_component("Mesh")
        Report.result(Tests.mesh_component_added, hydra.has_components(entity_id, ['Mesh']))

        # 6) Delete Mesh Component
        general.idle_wait_frames(3)     # Wait for Inspector to refresh
        comp_list_contents = entity_inspector.findChild(QtWidgets.QWidget, "m_componentListContents")
        await pyside_utils.wait_for_condition(lambda: len(comp_list_contents.children()) > 3)
        # Mesh Component is the 3rd component added to the entity including the default Transform component
        # There is one QVBoxLayout child before the QFrames, making the Mesh component the 4th child in the list
        mesh_frame = comp_list_contents.children()[3]
        mesh_frame.activateWindow()
        mesh_frame.setFocus()
        QtTest.QTest.mouseClick(mesh_frame, Qt.LeftButton, Qt.NoModifier)
        QtTest.QTest.keyClick(mesh_frame, Qt.Key_Delete, Qt.NoModifier)
        general.idle_wait_frames(3)  # Wait for Inspector to refresh
        success = await pyside_utils.wait_for_condition(lambda: not hydra.has_components(entity_id, ['Mesh']), 5.0)
        Report.result(Tests.mesh_component_deleted, success)

        # 7) Undo deletion of component after waiting for the deletion to register
        PrefabWaiter.wait_for_propagation()
        QtTest.QTest.keyPress(entity_inspector, Qt.Key_Z, Qt.ControlModifier)
        success = await pyside_utils.wait_for_condition(lambda: hydra.has_components(entity_id, ['Mesh']), 5.0)
        Report.result(Tests.mesh_component_delete_undo, success)

    run_test()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(ComponentCRUD_Add_Delete_Components)
