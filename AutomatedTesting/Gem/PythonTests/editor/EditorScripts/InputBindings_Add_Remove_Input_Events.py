"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    asset_editor_opened = (
        "Successfully opened the Asset Editor",
        "Failed to open the Asset Editor"
    )
    event_groups_added = (
        "Successfully added event groups via +",
        "Failed to add event groups"
    )
    single_event_group_deleted = (
        "Successfully deleted an event group",
        "Failed to delete event group"
    )
    all_event_groups_deleted = (
        "Successfully deleted all event groups",
        "Failed to delete all event groups"
    )
    asset_editor_closed = (
        "Successfully closed the Asset Editor",
        "Failed to close the Asset Editor"
    )


def InputBindings_Add_Remove_Input_Events():

    import editor_python_test_tools.pyside_utils as pyside_utils

    @pyside_utils.wrap_async
    async def run_test():
        """
        Summary:
        Verify if we are able add/remove input events in inputbindings file.

        Expected Behavior:
        New Events are created with the "+" button.
        Individual Events are removed with the "x" button.
        All of the Events under the Input Event Groups are removed with the square button.


        Test Steps:
        1) Open an existing level
        2) Open Asset Editor
        3) Access Asset Editor
        4) Create a new .inputbindings file and add event groups
        5) Verify if there are 3 elements in the Input Event Groups label
        6) Delete one event group
        7) Ensure one event group is deleted
        8) Click on Delete button to delete all the Event Groups
        9) Verify if all the elements are deleted
        10) Close Asset Editor

        Note:
        - This test file must be called from the Open 3D Engine Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        from PySide2 import QtWidgets

        import azlmbr.legacy.general as general

        from editor_python_test_tools.utils import Report
        from editor_python_test_tools.utils import TestHelper as helper

        def open_asset_editor():
            general.open_pane("Asset Editor")
            return general.is_pane_visible("Asset Editor")

        def close_asset_editor():
            general.close_pane("Asset Editor")
            return not general.is_pane_visible("Asset Editor")

        # 1) Open an existing simple level
        helper.init_idle()
        helper.open_level("Physics", "Base")

        # 2) Open Asset Editor
        Report.result(Tests.asset_editor_opened, open_asset_editor())

        # 3) Access Asset Editor
        editor_window = pyside_utils.get_editor_main_window()
        app = QtWidgets.QApplication.instance()
        asset_editor = editor_window.findChild(QtWidgets.QDockWidget, "Asset Editor")
        asset_editor_widget = asset_editor.findChild(QtWidgets.QWidget, "m_assetEditorWidget")

        # 4) Create a new .inputbindings file and add event groups
        # Get the action File->New->Input Bindings and trigger it
        action = pyside_utils.find_child_by_pattern(asset_editor_widget, {"text": "Input Bindings"})
        action.trigger()

        # Add event groups
        input_event_groups = asset_editor_widget.findChild(QtWidgets.QFrame, "Input Event Groups")
        # First QToolButton is +, Second QToolButton is Delete
        add_button = input_event_groups.findChildren(QtWidgets.QToolButton, "")[0]
        add_button.click()
        add_button.click()
        add_button.click()

        # 5) Verify if there are 3 elements in the Input Event Groups label
        no_of_elements_label = input_event_groups.findChild(QtWidgets.QLabel, "DefaultLabel")
        success = await pyside_utils.wait_for_condition(lambda: "3 elements" in no_of_elements_label.text(), 2.0)
        Report.result(Tests.event_groups_added, success)

        # 6) Delete one event group
        event = asset_editor_widget.findChildren(QtWidgets.QFrame, "<Unspecified Event>")[0]
        delete_button = event.findChildren(QtWidgets.QToolButton, "")[0]
        delete_button.click()

        # 7) Ensure one event group is deleted
        # NOTE: Here when we delete an input group a new "Input Event Groups" QFrame is generated from which
        # the QLabel showing the no of elements is being rendered, so we are taking the 2nd QFrame
        # with name "Input Event Groups" to verify the number of elements
        def get_elements_label_text(asset_editor_widget):
            input_event_groups = asset_editor_widget.findChildren(QtWidgets.QFrame, "Input Event Groups")
            if len(input_event_groups) > 1:
                input_event_group = input_event_groups[1]
                no_of_elements_label = input_event_group.findChild(QtWidgets.QLabel, "DefaultLabel")
                return no_of_elements_label.text()
            return ""

        success = await pyside_utils.wait_for_condition(lambda: "2 elements" in
                                                                get_elements_label_text(asset_editor_widget), 2.0)
        Report.result(Tests.single_event_group_deleted, success)

        # 8) Click on Delete button to delete all the Event Groups
        # First QToolButton child of active input_event_groups is +, Second QToolButton is Delete
        input_event_groups = asset_editor_widget.findChildren(QtWidgets.QFrame, "Input Event Groups")[1]
        delete_all_button = input_event_groups.findChildren(QtWidgets.QToolButton, "")[1]
        pyside_utils.click_button_async(delete_all_button)

        # Clicking the Delete All button will prompt the user if they are sure they want to
        # delete all the entries, so we wait for this modal dialog and then accept it
        active_modal_widget = await pyside_utils.wait_for_modal_widget()
        message_box = active_modal_widget.findChild(QtWidgets.QMessageBox)
        yes_button = message_box.button(QtWidgets.QMessageBox.Yes)
        yes_button.click()

        # 9) Verify if all the elements are deleted
        success = await pyside_utils.wait_for_condition(lambda: "0 elements" in
                                                                get_elements_label_text(asset_editor_widget), 2.0)
        Report.result(Tests.all_event_groups_deleted, success)

        # 10) Close Asset Editor
        Report.result(Tests.asset_editor_closed, close_asset_editor())

    run_test()


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(InputBindings_Add_Remove_Input_Events)
