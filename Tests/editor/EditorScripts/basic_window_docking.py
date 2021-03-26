"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C18668804: Basic Window Docking System Tests
https://testrail.agscollab.com/index.php?/cases/view/18668804
"""

import azlmbr.legacy.general as general

from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper
import Tests.ly_shared.pyside_utils as pyside_utils

from PySide2 import QtCore, QtWidgets


class TestBasicWindowDocking(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="basic_window_docking: ")

    def run_test(self):
        """
        Summary:
        Test basic docking behavior by undocking the Entity Outliner, and
        then docking on the edges around the main Editor window.

        Expected Behavior:
        The window becomes undocked and floats on its own.
        The window can be docked to the main Editor's edges and is resized to fit.

        Test Steps:
        1) Click on the Entity Outliner's title bar and drag it away to undock it.
        2) Click and drag the Entity Outliner to empty borders along the main Editor (top/bottom/left/right)

        :return: None
        """

        # Make sure the Entity Outliner is open
        general.open_pane("Entity Outliner (PREVIEW)")

        editor_window = pyside_utils.get_editor_main_window()
        entity_outliner = editor_window.findChild(QtWidgets.QDockWidget, "Entity Outliner (PREVIEW)")

        # 1) Click on the Entity Outliner's title bar and drag it away to undock it.
        # We drag/drop it over the viewport since it doesn't allow docking, so this will undock it
        render_overlay = editor_window.findChild(QtWidgets.QWidget, "renderOverlay")
        pyside_utils.drag_and_drop(entity_outliner, render_overlay)

        # Make sure the Entity Outliner is in a different QMainWindow than the main Editor QMainWindow,
        # which means it has been properly undocked (in a floating window)
        main_window = editor_window.findChild(QtWidgets.QMainWindow)
        if entity_outliner.parentWidget() != main_window:
            print("Entity Outliner is in a floating window")

        # 2) Click and drag the Entity Outliner to empty borders along the main Editor (top/bottom/left/right)

        # We need to grab a new reference to the Entity Outliner QDockWidget because when it gets moved
        # to the floating window, its parent changes so the wrapped intance we had becomes invalid
        entity_outliner = editor_window.findChild(QtWidgets.QDockWidget, "Entity Outliner (PREVIEW)")

        # Dock to absolute top of main window
        edge_offset = 10 # The absolute drop zones are 25px wide, chose 10px as an in between
        main_window_rect = main_window.rect()
        main_window_center = main_window.rect().center()
        top_center = QtCore.QPoint(main_window_center.x(), edge_offset)
        pyside_utils.drag_and_drop(entity_outliner, main_window, QtCore.QPoint(), top_center)

        # Make sure the Entity Outliner is now in the top area of the main Editor window
        if main_window.dockWidgetArea(entity_outliner) == QtCore.Qt.DockWidgetArea.TopDockWidgetArea:
            print("Entity Outliner docked in top area")

        # Dock to absolute right of main window
        right_center = QtCore.QPoint(main_window_rect.right() - edge_offset, main_window_center.y())
        pyside_utils.drag_and_drop(entity_outliner, main_window, QtCore.QPoint(), right_center)

        # Make sure the Entity Outliner is now in the right area of the main Editor window
        if main_window.dockWidgetArea(entity_outliner) == QtCore.Qt.DockWidgetArea.RightDockWidgetArea:
            print("Entity Outliner docked in right area")

        # Dock to absolute bottom of main window
        bottom_center = QtCore.QPoint(main_window_center.x(), main_window_rect.bottom() - edge_offset)
        pyside_utils.drag_and_drop(entity_outliner, main_window, QtCore.QPoint(), bottom_center)

        # Make sure the Entity Outliner is now in the bottom area of the main Editor window
        if main_window.dockWidgetArea(entity_outliner) == QtCore.Qt.DockWidgetArea.BottomDockWidgetArea:
            print("Entity Outliner docked in bottom area")

        # Dock to absolute left of main window
        left_center = QtCore.QPoint(edge_offset, main_window_center.y())
        pyside_utils.drag_and_drop(entity_outliner, main_window, QtCore.QPoint(), left_center)

        # Make sure the Entity Outliner is now in the left area of the main Editor window
        if main_window.dockWidgetArea(entity_outliner) == QtCore.Qt.DockWidgetArea.LeftDockWidgetArea:
            print("Entity Outliner docked in left area")


test = TestBasicWindowDocking()
test.run()
