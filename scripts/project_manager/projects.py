#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

# PySide project and gem selector GUI

import os
import sys
import argparse
import json
import logging
import subprocess
import threading
import platform
from logging.handlers import RotatingFileHandler

from typing import List

from pathlib import Path
from pyside import add_pyside_environment, is_pyside_ready, uninstall_env

logger = logging.getLogger()
logger.setLevel(logging.INFO)

log_path = os.path.join(os.path.dirname(__file__), "logs")
if not os.path.isdir(log_path):
    os.makedirs(log_path)
log_path = os.path.join(log_path, "project_manager.log")
log_file_handler = RotatingFileHandler(filename=log_path, maxBytes=1024 * 1024, backupCount=1)
formatter = logging.Formatter('%(asctime)s | %(levelname)s : %(message)s')
log_file_handler.setFormatter(formatter)
logger.addHandler(log_file_handler)

logger.info("Starting Project Manager")

engine_path = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..'))
sys.path.append(engine_path)
executable_path = ''


def initialize_pyside_from_parser():
    # Parse arguments up top.  We need to know the path to our binaries and QT libs in particular to load up
    # PySide
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable_path', required=True, help='Path to Executable to launch with project')
    parser.add_argument('--binaries_path', default=None, help='Path to QT Binaries necessary for PySide.  If not'
                        'provided executable_path folder is assumed')
    parser.add_argument('--parent_pid', default=0, help='Process ID of launching process')

    args = parser.parse_args()

    logger.info(f"parent_pid is {args.parent_pid}")
    global executable_path
    executable_path = args.executable_path
    binaries_path = args.binaries_path or os.path.dirname(executable_path)

    # Initialize PySide before imports below. This adds both PySide python modules to the python system interpreter
    # path and adds the necessary paths to binaries for the DLLs to be found and load their dependencies
    add_pyside_environment(binaries_path)


if not is_pyside_ready():
    initialize_pyside_from_parser()

try:
    from PySide2.QtWidgets import QApplication, QDialogButtonBox, QPushButton, QComboBox, QMessageBox, QFileDialog
    from PySide2.QtWidgets import QListView, QLabel
    from PySide2.QtUiTools import QUiLoader
    from PySide2.QtCore import QFile, QObject, Qt, Signal, Slot
    from PySide2.QtGui import QIcon, QStandardItemModel, QStandardItem
except ImportError as e:
    logger.error(f"Failed to import PySide2 with error {e}")
    exit(-1)

logger.error(f"PySide2 imports successful")

from cmake.Tools import engine_template
from cmake.Tools import add_remove_gem

ui_path = os.path.join(os.path.join(os.path.dirname(__file__), 'ui'))
ui_file = 'projects.ui'
ui_icon_file = 'projects.ico'
manage_gems_file = 'manage_gems.ui'
create_project_file = 'create_project.ui'

mru_file_name = 'o3de_projects.json'
default_settings_folder = os.path.join(Path.home(), '.o3de')

# Used to indicate a folder which appears to be a valid o3de project
project_marker_file = 'project.json'


class DialogLoggerSignaller(QObject):
    send_to_dialog = Signal(str)

    def __init__(self, dialog_logger):
        super(DialogLoggerSignaller, self).__init__()

        self.dialog_logger = dialog_logger


# Independent class to handle log forwarding.  Logger and qt signals both use emit method.
# This class's job is to receive the logger record and then emit the formatted message through
# DialogLoggerSignaller which is what the ProjectDialog handler listens for
class DialogLogger(logging.Handler):

    def __init__(self, log_dialog, log_level=logging.INFO, forward_log_level=logging.WARNING,
                 message_box_log_level=logging.ERROR):
        super(DialogLogger, self).__init__()

        self.log_dialog = log_dialog
        self.log_level = log_level
        self.forward_log_level = forward_log_level
        self.message_box_log_level = message_box_log_level
        self.log_records = []
        self.formatter = logging.Formatter('%(levelname)s : %(message)s')
        self.setFormatter(self.formatter)
        self.signaller = DialogLoggerSignaller(self)

    def emit(self, record):
        self.log_records.append(record)
        if record.levelno >= self.message_box_log_level:
            QMessageBox.warning(None, record.levelname, record.message)
        elif record.levelno >= self.forward_log_level:
            self.signaller.send_to_dialog.emit(self.format(record))



class ProjectDialog(QObject):
    """
    Main project dialog class responsible for displaying the project selection list
    """

    def __init__(self, parent=None, my_ui_path=ui_path, settings_folder=default_settings_folder):
        super(ProjectDialog, self).__init__(parent)

        self.log_display = None
        self.dialog_logger = DialogLogger(self)
        logger.addHandler(self.dialog_logger)
        logger.setLevel(logging.INFO)

        self.dialog_logger.signaller.send_to_dialog.connect(self.handle_log_message)
        self.displayed_projects = []

        self.mru_file_path = os.path.join(settings_folder, mru_file_name)

        self.my_ui_file_path = os.path.join(my_ui_path, ui_file)
        self.my_ui_icon_path = os.path.join(my_ui_path, ui_icon_file)
        self.my_ui_manage_gems_path = os.path.join(my_ui_path, manage_gems_file)
        self.my_ui_create_project_path = os.path.join(my_ui_path, create_project_file)
        self.ui_dir = my_ui_path
        self.ui_file = QFile(self.my_ui_file_path)
        self.ui_file.open(QFile.ReadOnly)

        loader = QUiLoader()
        self.dialog = loader.load(self.ui_file)
        self.dialog.setWindowIcon(QIcon(self.my_ui_icon_path))
        self.dialog.setFixedSize(self.dialog.size())
        self.browse_projects_button = self.dialog.findChild(QPushButton, 'browseProjectsButton')
        self.browse_projects_button.clicked.connect(self.browse_projects_handler)

        self.log_display = self.dialog.findChild(QLabel, 'logDisplay')

        self.create_project_button = self.dialog.findChild(QPushButton, 'createProjectButton')
        self.create_project_button.clicked.connect(self.create_project_handler)

        self.ok_cancel_button = self.dialog.findChild(QDialogButtonBox, 'okCancel')
        self.ok_cancel_button.accepted.connect(self.accepted_handler)

        self.manage_gems_button = self.dialog.findChild(QPushButton, 'manageGemsButton')
        self.manage_gems_button.clicked.connect(self.manage_gems_handler)

        self.project_list_box = self.dialog.findChild(QComboBox, 'projectListBox')
        self.add_projects(self.get_mru_list())
        self.project_gem_list = []

        self.dialog.show()

        self.load_thread = None
        self.gems_list = []
        self.load_gems()

    def update_gems(self) -> None:
        """
        Perform a full refresh of active project and available gems.  Loads both from
        data on disk and refreshes UI
        :return: None
        """
        project_path = self.path_for_selection()
        if not project_path:
            self.project_gem_list = set()
        else:
            self.project_gem_list = add_remove_gem.get_project_gem_list(self.path_for_selection())
        project_gem_model = QStandardItemModel()
        for item in sorted(self.project_gem_list):
            project_gem_model.appendRow(QStandardItem(item))
        self.project_gems.setModel(project_gem_model)

        add_gem_model = QStandardItemModel()
        for item in self.gems_list:
            if item.get('Name') in self.project_gem_list:
                continue
            model_item = QStandardItem(item.get('Name'))
            model_item.setData(item.get('Path'), Qt.UserRole)
            add_gem_model.appendRow(model_item)
        self.add_gems_list.setModel(add_gem_model)

    def get_selected_project_name(self) -> str:
        return os.path.basename(self.path_for_selection())

    def get_launch_project(self) -> str:
        # We can't currently pass an absolute path, so we need to assume the folder lives under engine and just pass
        # the base name of the selected project
        return os.path.basename(os.path.normpath(self.path_for_selection()))

    def get_executable_launch_params(self) -> list:
        """
        Retrieve the necessary launch parameters to make the subprocess launch call with - this is the path
        to the executable such as the Editor and the path to the selected project
        :return: list of params
        """
        launch_params = [executable_path,
                         f'-regset="/Amazon/AzCore/Bootstrap/project_path={self.get_launch_project()}"']
        return launch_params

    def load_gems(self) -> None:
        """
        Starts another thread to discover available gems.  This requires file discovery/parsing and would likely
        cause a noticeable pause if it were not on another thread
        :return: None
        """
        self.load_thread = threading.Thread(target=self.load_gems_thread_func)
        self.load_thread.start()

    def load_gems_thread_func(self) -> None:
        """
        Actual load function for load_gems thread.  Blocking call to lower level find method to fill out gems_list
        :return: None
        """
        self.gems_list = add_remove_gem.find_all_gems([os.path.join(engine_path, 'Gems')])

    def load_template_list(self) -> None:
        """
        Search for available templates to fill out template list
        :return: None
        """
        self.project_templates = engine_template.find_all_project_templates([os.path.join(engine_path, 'Templates')])

    def get_gem_info(self, gem_name: str) -> str:
        """
        Provided a known gem name provides gem info
        :param gem_name: Name of known gem.  Names are based on Gem.<Gemname> module names in CMakeLists.txt rather
        than folders a Gem lives in.
        :return: Dictionary with data about gem if known
        """
        for this_gem in self.gems_list:
            if this_gem.get('Name') == gem_name:
                this_gem
        return None

    def get_gem_info_by_path(self, gem_path: str) -> str:
        """
        Provided a gem path returns the gem info if known
        :param gem_path: Path to gem
        :return: Dictionary with data about gem if known
        """
        for this_gem in self.gems_list:
            if this_gem.get('Path') == gem_path:
                return this_gem
        return None

    def path_for_gem(self, gem_name: str) -> str:
        """
        Provided a known gem name provides the full path on disk
        :param gem_name: Name of known gem.  Names are based on Gem.<Gemname> module names in CMakeLists.txt rather
        than folders a Gem lives in.
        :return: Path to CMakeLists.txt containing the Gem modules
        """
        for this_gem in self.gems_list:
            if this_gem.get('Name') == gem_name:
                return this_gem.get('Path')
        return ''

    def open_project(self, project_path: str) -> None:
        """
        Launch the desired application given the selected project
        :param project_path: Path to currently selected project
        :return: None
        """
        logger.info(f'Attempting to open {project_path}')
        self.update_mru_list(project_path)
        launch_params = self.get_executable_launch_params()
        try:
            logger.info(f'Launching with params {launch_params}')
            subprocess.Popen(launch_params, env=uninstall_env())
            quit(0)
        except subprocess.CalledProcessError as e:
            logger.error(f'Failed to start executable with launch params {launch_params} error {e}')

    def path_for_selection(self) -> str:
        """
        Retrive the full path to the project the user currently has selected in the drop down
        :return: str path to project
        """
        if self.project_list_box.currentIndex() == -1:
            logger.warning("No project selected")
            return ""
        return self.project_list_box.itemData(self.project_list_box.currentIndex(), Qt.ToolTipRole)

    def accepted_handler(self) -> None:
        """
        Override for handling "Ok" on main project dialog to first check whether the user has selected a project and
        prompt them to if not.  If a project is selected will attempt to open it.
        :return: None
        """
        if not self.project_list_box.currentText():
            msg_box = QMessageBox(parent=self.dialog)
            msg_box.setWindowTitle("O3DE")
            msg_box.setText("Please select a project")
            msg_box.exec()
            return
        self.open_project(self.path_for_selection())

    def is_project_folder(self, project_path: str) -> bool:
        """
        Checks whether the supplied path appears to be a canonical project folder.
        Root of a valid project should contain the canonical file
        :param project_path:
        :return:
        """
        return os.path.isfile(os.path.join(project_path, project_marker_file))

    def add_new_project(self, project_folder, update_mru=True, reset_selected=True, validate=True):
        """
        Handle request to add a new project to our display and mru lists.  Validation checks whether the folder
        appears to be a project.  Duplicates should never be added with or without validation.
        :param project_folder: Absolute path to project folder
        :param update_mru: Should the project also be promoted to "most recent" in our mru list
        :param reset_selected: Update our drop down to show this as our current selection
        :param validate: Verify the folder appears to be a valid project folder
        :return:
        """
        if validate:
            if not self.is_project_folder(project_folder):
                QMessageBox.warning(self.dialog, "Invalid Project Folder",
                                    f"{project_folder} does not contain a {project_marker_file}"
                                    f" and does not appear to be a valid project")
                return
        self.add_projects([project_folder])
        if reset_selected:
            self.project_list_box.setCurrentIndex(self.project_list_box.count() - 1)
        if update_mru:
            self.update_mru_list(project_folder)

    def browse_projects_handler(self):
        """
        Open a file search dialog looking for a folder which contains a valid project marker.  If valid
        will update the mru list with the new entry, if invalid will warn the user.
        :return: None
        """
        project_folder = QFileDialog.getExistingDirectory(self.dialog, "Select Project Folder", engine_path)
        if project_folder:
            self.add_new_project(project_folder)

        return

    def get_selected_project_gems(self) -> list:
        """
        :return: List of (GemName, GemPath) of currently selected gems in the project gems list
        """
        selected_items = self.project_gems.selectionModel().selectedRows()
        return [self.project_gems.model().data(item) for item in selected_items]

    def remove_gems_handler(self):
        """
        Finds the currently selected gems in the active gems list and attempts to remove each and updates the UI.
        :return: None
        """
        remove_gems = self.get_selected_project_gems()

        for this_gem in remove_gems:
            gem_path = self.path_for_gem(this_gem)
            add_remove_gem.add_remove_gem(add=False,
                                          dev_root=engine_path,
                                          gem_path=gem_path or os.path.join(engine_path, 'Gems', this_gem),
                                          gem_target=this_gem,
                                          project_path=self.path_for_selection(),
                                          dependencies_file=None,
                                          runtime_dependency=True,
                                          tool_dependency=True,
                                          server_dependency=True)
        self.update_gems()

    def manage_gems_handler(self):
        """
        Opens the Gem management pane.  Waits for the load thread to complete if still running and displays all
        active gems for the current project as well as all available gems which aren't currently active.
        :return: None
        """

        if not self.path_for_selection():
            msg_box = QMessageBox(parent=self.dialog)
            msg_box.setWindowTitle("O3DE")
            msg_box.setText("Please select a project")
            msg_box.exec()
            return

        self.load_thread.join()
        loader = QUiLoader()
        self.manage_gems_file = QFile(self.my_ui_manage_gems_path)

        if not self.manage_gems_file:
            logger.error(f'Failed to load gems UI file at {self.manage_gems_file}')
            return

        self.manage_gems_dialog = loader.load(self.manage_gems_file)

        if not self.manage_gems_dialog:
            logger.error(f'Failed to load gems dialog file at {self.manage_gems_file}')
            return

        self.manage_gems_dialog.setWindowTitle(f"Manage Gems for {self.get_selected_project_name()}")

        self.add_gems_button = self.manage_gems_dialog.findChild(QPushButton, 'addGemsButton')
        self.add_gems_button.clicked.connect(self.add_gems_handler)

        self.add_gems_list = self.manage_gems_dialog.findChild(QListView, 'addGemsList')

        self.remove_gems_button = self.manage_gems_dialog.findChild(QPushButton, 'removeGemsButton')
        self.remove_gems_button.clicked.connect(self.remove_gems_handler)

        self.project_gems = self.manage_gems_dialog.findChild(QListView, 'projectGems')
        self.update_gems()

        self.manage_gems_dialog.exec()

    def get_selected_add_gems(self) -> list:
        """
        Find returns a list of currently selected gems in the add listas (GemName, GemPath)
        :return:
        """
        selected_items = self.add_gems_list.selectionModel().selectedRows()
        return [(self.add_gems_list.model().data(item), self.add_gems_list.model().data(item, Qt.UserRole)) for
                         item in selected_items]

    def add_gems_handler(self) -> None:
        """
        Searches the available gems list for selected gems and attempts to add each one to the current project.
        Updates UI after completion.
        :return: None
        """
        add_gems_list = self.get_selected_add_gems()
        for this_gem in add_gems_list:
            gem_info = self.get_gem_info_by_path(this_gem[1])
            if not gem_info:
                logger.error(f'Unknown gem {this_gem}!')
                continue
            add_remove_gem.add_remove_gem(add=True,
                                          dev_root=engine_path,
                                          gem_path=this_gem[1],
                                          gem_target=gem_info.get('Name'),
                                          project_path=self.path_for_selection(),
                                          dependencies_file=None,
                                          runtime_dependency=gem_info.get('Runtime', False),
                                          tool_dependency=gem_info.get('Tools', False),
                                          server_dependency=gem_info.get('Tools', False))
        self.update_gems()

    def create_project_handler(self):
        """
        Opens the Create Project pane.  Retrieves a list of available templates for display
        :return: None
        """
        loader = QUiLoader()
        self.create_project_file = QFile(self.my_ui_create_project_path)

        if not self.create_project_file:
            logger.error(f'Failed to create project UI file at {self.create_project_file}')
            return

        self.create_project_dialog = loader.load(self.create_project_file)

        if not self.create_project_dialog:
            logger.error(f'Failed to load create project dialog file at {self.create_project_file}')
            return

        self.create_project_ok_button = self.create_project_dialog.findChild(QDialogButtonBox, 'okCancel')
        self.create_project_ok_button.accepted.connect(self.create_project_accepted_handler)

        self.project_template_list = self.create_project_dialog.findChild(QListView, 'projectTemplates')

        self.load_template_list()

        self.load_template_model = QStandardItemModel()
        for item in self.project_templates:
            model_item = QStandardItem(item[0])
            model_item.setData(item[1], Qt.UserRole)
            self.load_template_model.appendRow(model_item)

        self.project_template_list.setModel(self.load_template_model)

        self.create_project_dialog.exec()

    def get_selected_project_template(self) -> tuple:
        """
        Get the current pair templatename, path to template selecte dby the user
        :return: pair
        """

        selected_item = self.project_template_list.selectionModel().currentIndex()
        if not selected_item.isValid():
            logger.warning("Select a template to create from")
            return None

        create_project_item = (self.project_template_list.model().data(selected_item),
                               self.project_template_list.model().data(selected_item, Qt.UserRole))
        return create_project_item

    def create_project_accepted_handler(self) -> None:
        """
        Searches the available gems list for selected gems and attempts to add each one to the current project.
        Updates UI after completion.
        :return: None
        """
        create_project_item = self.get_selected_project_template()
        if not create_project_item:
            return

        folder_dialog = QFileDialog(self.dialog, "Select a Folder and Enter a New Project Name", engine_path)
        folder_dialog.setFileMode(QFileDialog.AnyFile)
        folder_dialog.setOptions(QFileDialog.ShowDirsOnly)
        project_count = 0
        project_name = "MyNewProject"
        while os.path.exists(os.path.join(engine_path, project_name)):
            project_name = f"MyNewProject{project_count}"
            project_count += 1
        folder_dialog.selectFile(project_name)
        project_folder = None
        if folder_dialog.exec():
            project_folder = folder_dialog.selectedFiles()
        if project_folder:
            if engine_template.create_project(engine_path, project_folder[0], create_project_item[1]) == 0:
                # Success
                self.add_new_project(project_folder[0], validate=False)
                msg_box = QMessageBox(parent=self.dialog)
                msg_box.setWindowTitle("O3DE")
                msg_box.setText(f"Project {os.path.basename(os.path.normpath(project_folder[0]))} created." 
                                "  Build your\nnew project before hitting OK to launch.")
                msg_box.exec()
                return

    def get_display_name(self, project_path: str) -> str:
        """
        Returns the project path in the format to be displayed in the projects MRU list
        :param project_path: Path to the project folder
        :return: Formatted path
        """
        return f'{os.path.basename(os.path.normpath(project_path))}  ({project_path})'

    def add_projects(self, new_list: List[str]) -> None:
        """
        Attempt to add a list of known projects.  Performs validation first that the supplied folder appears valid and
        silently drops invalid folders - these can simply be folders which were previously valid and are in the MRU list
        but have been moved or deleted.  Used both when loading the MRU list or when a user browses or creates a new
        project
        :param new_list: List of full paths to projects to add
        :return: None
        """
        new_display_items = []
        new_display_paths = []
        for this_item in new_list:
            if self.is_project_folder(this_item) and this_item not in self.displayed_projects:
                self.displayed_projects.append(this_item)
                new_display_items.append(self.get_display_name(this_item))
                new_display_paths.append(this_item)
                # Storing the full path in the tooltip, if this is altered we need to store this elsewhere
                # as it's used when selecting a project to open

        for this_slot in range(len(new_display_items)):
            self.project_list_box.addItem(new_display_items[this_slot])
            self.project_list_box.setItemData(self.project_list_box.count() - 1, new_display_paths[this_slot],
                                              Qt.ToolTipRole)

    def update_mru_list(self, used_project: str) -> None:
        """
        Promote a supplied project name to the "most recent" in a given MRU list.
        :param used_project: path to project to promote
        :param file_path: path to mru list file
        :return: None
        """
        used_project = os.path.normpath(used_project)
        if not os.path.exists(os.path.dirname(self.mru_file_path)):
            os.makedirs(os.path.dirname(self.mru_file_path), exist_ok=True)
        mru_data = {}
        try:
            with open(self.mru_file_path, 'r') as mru_file:
                mru_data = json.loads(mru_file.read())
        except FileNotFoundError:
            pass
        except json.JSONDecodeError:
            pass

        recent_list = mru_data.get('Projects', [])
        recent_list = [item for item in recent_list if item.get('Path') != used_project and
                       self.is_project_folder(item.get('Path'))]

        new_list = [{'Path': used_project}]
        new_list.extend(recent_list)

        mru_data['Projects'] = new_list
        try:
            with open(self.mru_file_path, 'w') as mru_file:
                mru_file.write(json.dumps(mru_data, indent=1))
        except PermissionError as e:
            logger.warning(f"Failed to write {self.mru_file_path} with error {e}")

    def get_mru_list(self) -> List[str]:
        """
        Retrive the current MRU list.  Does not perform validation that the projects still appear valid
        :return: list of full path strings to project folders
        """
        if not os.path.exists(os.path.dirname(self.mru_file_path)):
            return []
        try:
            with open(self.mru_file_path, 'r') as mru_file:
                mru_data = json.loads(mru_file.read())
        except FileNotFoundError:
            return []
        except json.JSONDecodeError:
            logger.error(f'MRU list at {self.mru_file_path} is not valid JSON')
            return []

        recent_list = mru_data.get('Projects', [])
        return [item.get('Path') for item in recent_list if item.get('Path') is not None]

    @Slot(str)
    def handle_log_message(self, message: str) -> None:
        """
        Signal handler for messages from the logger.  Displays the most recent warning/error
        :param message: formatted log message from DialogLoggerSignaller
        :return:
        """
        if not self.log_display:
            return
        self.log_display.setText(message)
        self.log_display.setToolTip(message)


if __name__ == "__main__":
    dialog_app = QApplication(sys.argv)
    my_dialog = ProjectDialog()
    dialog_app.exec_()
    sys.exit(0)
