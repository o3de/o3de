"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Object to house all the Qt Objects and behavior used in testing the script canvas node palette
"""
from editor_python_test_tools.utils import TestHelper as helper
from PySide2 import QtWidgets, QtTest, QtCore
import pyside_utils
from consts.scripting import (NODE_PALETTE_QT, TREE_VIEW_QT, SEARCH_FRAME_QT, SEARCH_FILTER_QT, NODE_PALETTE_CLEAR_BUTTON_QT)
from consts.general import (WAIT_TIME_SEC_1, WAIT_TIME_SEC_3)

class QtPyScriptCanvasNodePalette():
    """
       QtPy class for handling the behavior of the script canvas node palette such as filter/searching for a node type
        """
    def __init__(self, sc_editor):
        self.node_palette = sc_editor.sc_editor.findChild(QtWidgets.QDockWidget, NODE_PALETTE_QT)
        self.node_palette_tree = self.node_palette.findChild(QtWidgets.QTreeView, TREE_VIEW_QT)
        self.node_palette_search_frame = self.node_palette.findChild(QtWidgets.QFrame, SEARCH_FRAME_QT)
        self.node_palette_search_box = self.node_palette_search_frame.findChild(QtWidgets.QLineEdit, SEARCH_FILTER_QT)
        self.node_palette_clear_search_button = self.node_palette_search_frame.findChild(QtWidgets.QToolButton,
                                                                                         NODE_PALETTE_CLEAR_BUTTON_QT)

    def search_for_node(self, node_name: str):
        """
        Function to mimic searching the node palette. Providing a name filters the node palette and reduces the list of
        visible nodes to the user. We return an empty search instead of throwing an assert because we allow the user
        to filter down to nothing

        params node_name: the name of the node you are searching for

        returns: a list of nodes found using the provided node_name string
        """
        self.clear_search_filter()
        self.__set_search_filter(node_name)

        nodes_found = pyside_utils.find_child_by_pattern(self.node_palette_tree, {"text": f"{node_name}"})

        return nodes_found

    def __set_search_filter(self, node_name: str):
        """
        helper function for validating that the search field was set properly by search functions

        param node_name: The name of the node we are searching for

        returns: None

        """
        self.node_palette_search_box.setText(node_name)
        search_field_set = self.node_palette_search_box.text() == node_name
        # this wait_for_condition call is placeholder while we wait on a better way to tell if the search filtering has ended
        # see GHI #12277 for more information and update this if the issue has been resolved
        helper.wait_for_condition(lambda: True is False, WAIT_TIME_SEC_1)
        helper.wait_for_condition(lambda: search_field_set, WAIT_TIME_SEC_3)

        assert search_field_set, f"Failed to set the node palette search field to: {node_name}."

    def clear_search_filter(self) -> None:
        """
        Function for manually setting the node palette search filter to a blank string.

        returns: None
        """
        self.__set_search_filter("")

    def click_clear_search_button(self) -> None:
        """
        Function for clicking the node palette's clear search filter button. Asserts if the search field could not be
        cleared out

        returns: None
        """
        self.node_palette_clear_search_button.click()
        search_field_text = self.node_palette_search_box.text()
        search_field_cleared = search_field_text == ""

        assert search_field_cleared, f"Failed to clear the node palette search field. Remaining text was {search_field_text} "
