"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def verify_docked_perspective_viewports():
    import azlmbr.legacy.general as general
    from PySide6 import QtCore, QtWidgets

    def position():
        camera = general.get_current_view_position()
        return tuple(round(camera.get_property(axis), 3) for axis in ("x", "y", "z"))

    original_layout = general.get_view_pane_layout()
    original_index = general.get_active_viewport()
    original_positions = []
    for index in range(general.get_viewport_count()):
        general.set_active_viewport(index)
        original_positions.append(position())
    try:
        for layout, count in ((1, 2), (7, 4)):
            general.set_view_pane_layout(layout)
            general.idle_wait_frames(2)
            assert general.get_viewport_count() == count
            for index in range(count):
                general.set_active_viewport(index)
                general.set_current_view_position(10.0 * index, 5.0, 3.0)
            for index in range(count):
                general.set_active_viewport(index)
                assert position() == (10.0 * index, 5.0, 3.0), f"Camera changed in pane {index}"

        dock = next(widget for widget in QtWidgets.QApplication.allWidgets()
                    if isinstance(widget, QtWidgets.QDockWidget) and widget.objectName() == "PerspectiveViewport3")
        viewport = next(child for child in dock.findChildren(QtWidgets.QWidget) if child.property("IsViewportWidget"))
        main_window = dock.window()
        for floating in (True, False):
            if floating:
                assert QtCore.QMetaObject.invokeMethod(dock, "undock", QtCore.Qt.ConnectionType.DirectConnection)
            else:
                general.set_view_pane_layout(7)
            general.idle_wait_frames(2)
            assert (dock.window() != main_window) == floating
            assert dock.isAncestorOf(viewport)
            general.set_active_viewport(3)
            assert position() == (30.0, 5.0, 3.0)
            general.set_active_viewport(0)
            assert position() == (0.0, 5.0, 3.0)

        host = dock.parentWidget()
        assert host.inherits("AzQtComponents::DockMainWindow")
        assert dock.inherits("AzQtComponents::StyledDockWidget")
        console_was_visible = general.is_pane_visible("Console")
        saved_state = host.saveState()
        try:
            general.open_pane("Console")
            general.idle_wait_frames(2)
            tool = host.findChild(QtWidgets.QDockWidget, "Console")
            assert tool is not None and tool.inherits("DockWidget")
            host.addDockWidget(QtCore.Qt.DockWidgetArea.BottomDockWidgetArea, tool)
            tool.setFloating(False)
            tool.show()
            host.splitDockWidget(dock, tool, QtCore.Qt.Orientation.Horizontal)
            tool.show()
            general.idle_wait_frames(2)
            assert tool not in host.tabifiedDockWidgets(dock)
            assert host.dockWidgetArea(dock) == host.dockWidgetArea(tool)
            assert dock.isVisible() and tool.isVisible()
            assert not dock.geometry().intersects(tool.geometry()), (dock.geometry(), tool.geometry())
            host.tabifyDockWidget(dock, tool)
            general.idle_wait_frames(2)
            assert tool in host.tabifiedDockWidgets(dock)
            general.set_active_viewport(3)
            assert position() == (30.0, 5.0, 3.0)
        finally:
            host.restoreState(saved_state)
            if not console_was_visible:
                general.close_pane("Console")

        general.set_active_viewport(3)
        dock.close()
        general.idle_wait_frames(2)
        assert general.get_viewport_count() == 3
        active = general.get_active_viewport()
        assert active < 3
        assert position() == (10.0 * active, 5.0, 3.0)
    finally:
        general.set_view_pane_layout(original_layout)
        for index, camera_position in enumerate(original_positions):
            general.set_active_viewport(index)
            general.set_current_view_position(*camera_position)
        general.set_active_viewport(original_index)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report, TestHelper

    def run():
        TestHelper.open_level("", "Base")
        verify_docked_perspective_viewports()
        Report.result(("Perspective panes retained their cameras", "Perspective pane camera changed"), True)

    Report.start_test(run)
