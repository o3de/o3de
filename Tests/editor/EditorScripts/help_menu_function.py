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
C1564080: Help Menu Function
https://testrail.agscollab.com/index.php?/cases/view/1564080
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import Tests.ly_shared.pyside_utils as pyside_utils
from PySide2 import QtCore, QtWidgets, QtGui
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestHelpMenuFunction(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="help_menu_function: ")

    def run_test(self):
        """
        Summary:
        Interact with Help Menu options that open Urls and verify if Urls are valid.

        Expected Behavior:
        The Help menu Urls are valid.

        Test Steps:
         1) Interact with Help Menu options that open Urls

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """
        url_help_menu_options = [
            ("Getting Started",),
            ("Tutorials",),
            ("Documentation", "Glossary",),
            ("Documentation", "Lumberyard Documentation",),
            ("Documentation", "GameLift Documentation",),
            ("Documentation", "Release Notes",),
            ("GameDev Resources", "GameDev Blog",),
            ("GameDev Resources", "GameDev Twitch Channel",),
            ("GameDev Resources", "Forums",),
            ("GameDev Resources", "AWS Support",),
            ("About Lumberyard",),
        ]

        def validate_url_action(action_name, urlHandler):
            if urlHandler.last_opened_url.isEmpty():
                print(f"{action_name} didn't open a url with an expected scheme (http/https)")
            else:
                url_display_string = urlHandler.last_opened_url.toDisplayString()
                if urlHandler.last_opened_url.isValid():
                    print(f"{action_name} triggered Url {url_display_string}")
                else:
                    print(f"{action_name} triggered invalid Url {url_display_string}")
            urlHandler.last_opened_url.clear()

        def on_focus_changed(old, new):
            active_widget = QtWidgets.QApplication.activeModalWidget()
            if active_widget:
                active_widget.close()

        # 1) Interact with the Help Menu options that open urls
        try:
            # Create a handler to open Urls
            class UrlHandler(QtCore.QObject):
                self.last_opened_url = QtCore.QUrl()
                @QtCore.Slot("QUrl")
                def open_url(self, url):
                    self.last_opened_url = url
            urlHandler = UrlHandler()
            QtGui.QDesktopServices.setUrlHandler("https", urlHandler, "open_url")
            QtGui.QDesktopServices.setUrlHandler("http", urlHandler, "open_url")
            editor_window = pyside_utils.get_editor_main_window()
            app = QtWidgets.QApplication.instance()
            app.focusChanged.connect(on_focus_changed)
            # Check help menu actions that trigger Urls
            for option in url_help_menu_options:
                action = pyside_utils.get_action_for_menu_path(editor_window, "Help", *option)
                if action.isVisible():
                    action.trigger()
                    action_name = action.iconText()
                    validate_url_action(f"{action_name} Action", urlHandler)
                else:
                    print(f"{option} is not visible in Help Menu")
            # Check the documentation search field under the help menu
            menu_bar = editor_window.menuBar()
            menu_bar_actions = [index.iconText() for index in menu_bar.actions()]
            main_menu_item = "Help"
            if main_menu_item not in menu_bar_actions:
                print(f"QAction not found for main menu item '{main_menu_item}'")
            else:
                help_action = menu_bar.actions()[menu_bar_actions.index(main_menu_item)]
                # Get the first action in the help menu which should be the search action
                search_action = help_action.menu().actions()[0]
                # Get the search line edit from the search action
                search_line_widget = search_action.defaultWidget().findChild(QtWidgets.QLineEdit)
                search_string = "component"
                search_line_widget.setText(search_string)
                search_line_widget.returnPressed.emit()
                validate_url_action(f"Documentation search with '{search_string}' text", urlHandler)
                search_string = ""
                search_line_widget.setText(search_string)
                search_line_widget.returnPressed.emit()
                validate_url_action(f"Documentation Search with '{search_string}' text", urlHandler)
        finally:
            app.focusChanged.disconnect(on_focus_changed)
            QtGui.QDesktopServices.unsetUrlHandler("https")
            QtGui.QDesktopServices.unsetUrlHandler("http")

test = TestHelpMenuFunction()
test.run()
