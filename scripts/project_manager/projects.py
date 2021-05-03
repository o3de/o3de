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
import pathlib
import sys
import argparse
import json
import logging
import subprocess
from logging.handlers import RotatingFileHandler
from typing import List
from pyside import add_pyside_environment, is_pyside_ready, uninstall_env

engine_path = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..'))
sys.path.append(engine_path)
executable_path = ''

logger = logging.getLogger()
logger.setLevel(logging.INFO)

from cmake.Tools import engine_template
from cmake.Tools import registration

o3de_folder = registration.get_o3de_folder()
o3de_logs_folder = registration.get_o3de_logs_folder()
project_manager_log_file_path = o3de_log_folder / "project_manager.log"
log_file_handler = RotatingFileHandler(filename=project_manager_log_file_path, maxBytes=1024 * 1024, backupCount=1)
formatter = logging.Formatter('%(asctime)s | %(levelname)s : %(message)s')
log_file_handler.setFormatter(formatter)
logger.addHandler(log_file_handler)

logger.info("Starting Project Manager")


def initialize_pyside_from_parser():
    # Parse arguments up top.  We need to know the path to our binaries and QT libs in particular to load up
    # PySide
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable-path', required=True, help='Path to Executable to launch with project')
    parser.add_argument('--binaries-path', default=None, help='Path to QT Binaries necessary for PySide.  If not'
                                                              'provided executable_path folder is assumed')
    parser.add_argument('--parent-pid', default=0, help='Process ID of launching process')

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


class ProjectManagerDialog(QObject):
    """
    Main project manager dialog is responsible for displaying the project selection list and output pane
    """

    def __init__(self, parent=None):
        super(ProjectManagerDialog, self).__init__(parent)

        self.ui_path = (pathlib.Path(__file__).parent / 'ui').resolve()
        self.home_folder = registration.get_home_folder()

        self.log_display = None
        self.dialog_logger = DialogLogger(self)
        logger.addHandler(self.dialog_logger)
        logger.setLevel(logging.INFO)

        self.dialog_logger.signaller.send_to_dialog.connect(self.handle_log_message)
        self.mru_file_path = o3de_folder / 'mru.json'

        self.create_from_template_ui_file_path = self.ui_path / 'create_from_template.ui'
        self.create_gem_ui_file_path = self.ui_path / 'create_gem.ui'
        self.create_project_ui_file_path = self.ui_path / 'create_project.ui'
        self.manage_project_gem_targets_ui_file_path = self.ui_path / 'manage_gem_targets.ui'
        self.project_manager_icon_file_path = self.ui_path / 'project_manager.ico'
        self.project_manager_ui_file_path = self.ui_path / 'project_manager.ui'

        self.project_manager_ui_file = QFile(self.project_manager_ui_file_path.as_posix())
        self.project_manager_ui_file.open(QFile.ReadOnly)

        loader = QUiLoader()
        self.dialog = loader.load(self.project_manager_ui_file)
        self.dialog.setWindowIcon(QIcon(self.project_manager_icon_file_path.as_posix()))
        self.dialog.setFixedSize(self.dialog.size())

        self.project_list_box = self.dialog.findChild(QComboBox, 'projectListBox')
        self.refresh_project_list()
        mru = self.get_mru_list()
        if len(mru):
            last_mru = pathlib.Path(mru[0]).resolve()
            for this_slot in range(self.project_list_box.count()):
                item_text = self.project_list_box.itemText(this_slot)
                if last_mru.as_posix() in item_text:
                    self.project_list_box.setCurrentIndex(this_slot)
                    break

        self.create_project_button = self.dialog.findChild(QPushButton, 'createProjectButton')
        self.create_project_button.clicked.connect(self.create_project_handler)
        self.create_gem_button = self.dialog.findChild(QPushButton, 'createGemButton')
        self.create_gem_button.clicked.connect(self.create_gem_handler)
        self.create_template_button = self.dialog.findChild(QPushButton, 'createTemplateButton')
        self.create_template_button.clicked.connect(self.create_template_handler)
        self.create_from_template_button = self.dialog.findChild(QPushButton, 'createFromTemplateButton')
        self.create_from_template_button.clicked.connect(self.create_from_template_handler)

        self.add_project_button = self.dialog.findChild(QPushButton, 'addProjectButton')
        self.add_project_button.clicked.connect(self.add_project_handler)
        self.add_gem_button = self.dialog.findChild(QPushButton, 'addGemButton')
        self.add_gem_button.clicked.connect(self.add_gem_handler)
        self.add_template_button = self.dialog.findChild(QPushButton, 'addTemplateButton')
        self.add_template_button.clicked.connect(self.add_template_handler)
        self.add_restricted_button = self.dialog.findChild(QPushButton, 'addRestrictedButton')
        self.add_restricted_button.clicked.connect(self.add_restricted_handler)

        self.remove_project_button = self.dialog.findChild(QPushButton, 'removeProjectButton')
        self.remove_project_button.clicked.connect(self.remove_project_handler)
        self.remove_gem_button = self.dialog.findChild(QPushButton, 'removeGemButton')
        self.remove_gem_button.clicked.connect(self.remove_gem_handler)
        self.remove_template_button = self.dialog.findChild(QPushButton, 'removeTemplateButton')
        self.remove_template_button.clicked.connect(self.remove_template_handler)
        self.remove_restricted_button = self.dialog.findChild(QPushButton, 'removeRestrictedButton')
        self.remove_restricted_button.clicked.connect(self.remove_restricted_handler)

        self.manage_runtime_project_gem_targets_button = self.dialog.findChild(QPushButton, 'manageRuntimeGemTargetsButton')
        self.manage_runtime_project_gem_targets_button.clicked.connect(self.manage_runtime_project_gem_targets_handler)
        self.manage_tool_project_gem_targets_button = self.dialog.findChild(QPushButton, 'manageToolGemTargetsButton')
        self.manage_tool_project_gem_targets_button.clicked.connect(self.manage_tool_project_gem_targets_handler)
        self.manage_server_project_gem_targets_button = self.dialog.findChild(QPushButton, 'manageServerGemTargetsButton')
        self.manage_server_project_gem_targets_button.clicked.connect(self.manage_server_project_gem_targets_handler)

        self.log_display = self.dialog.findChild(QLabel, 'logDisplay')

        self.ok_cancel_button = self.dialog.findChild(QDialogButtonBox, 'okCancel')
        self.ok_cancel_button.accepted.connect(self.accepted_handler)

        self.dialog.show()

    def refresh_project_list(self) -> None:
        projects = registration.get_all_projects()
        self.project_list_box.clear()
        for this_slot in range(len(projects)):
            display_name = f'{os.path.basename(os.path.normpath(projects[this_slot]))}  ({projects[this_slot]})'
            self.project_list_box.addItem(display_name)
            self.project_list_box.setItemData(self.project_list_box.count() - 1, projects[this_slot],
                                              Qt.ToolTipRole)

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
        self.launch_with_project_path(self.get_selected_project_path())

    def get_launch_project(self) -> str:
        return os.path.normpath(self.get_selected_project_path())

    def get_executable_launch_params(self) -> list:
        """
        Retrieve the necessary launch parameters to make the subprocess launch call with - this is the path
        to the executable such as the Editor and the path to the selected project
        :return: list of params
        """
        launch_params = [executable_path,
                         f'-regset="/Amazon/AzCore/Bootstrap/project_path={self.get_launch_project()}"']
        return launch_params

    def launch_with_project_path(self, project_path: str) -> None:
        """
        Launch the desired application given the selected project
        :param project_path: Path to currently selected project
        :return: None
        """
        logger.info(f'Attempting to open {project_path}')
        self.update_mru_list(project_path)
        launch_params = self.get_executable_launch_params()
        logger.info(f'Launching with params {launch_params}')
        subprocess.run(launch_params, env=uninstall_env())

    def get_selected_project_path(self) -> str:
        if self.project_list_box.currentIndex() == -1:
            logger.warning("No project selected")
            return ""
        return self.project_list_box.itemData(self.project_list_box.currentIndex(), Qt.ToolTipRole)

    def get_selected_project_name(self) -> str:
        project_data = registration.get_project_data(project_path=self.get_selected_project_path())
        return project_data['project_name']

    def create_project_handler(self):
        """
        Opens the Create Project pane.  Retrieves a list of available templates for display
        :return: None
        """
        loader = QUiLoader()
        self.create_project_file = QFile(self.create_project_ui_file_path.as_posix())

        if not self.create_project_file:
            logger.error(f'Failed to create project UI file at {self.create_project_file}')
            return

        self.create_project_dialog = loader.load(self.create_project_file)

        if not self.create_project_dialog:
            logger.error(f'Failed to load create project dialog file at {self.create_project_file}')
            return

        self.create_project_ok_button = self.create_project_dialog.findChild(QDialogButtonBox, 'okCancel')
        self.create_project_ok_button.accepted.connect(self.create_project_accepted_handler)

        self.create_project_template_list = self.create_project_dialog.findChild(QListView, 'projectTemplates')
        self.refresh_create_project_template_list()

        self.create_project_dialog.exec()

    def create_project_accepted_handler(self) -> None:
        """
        Searches the available gems list for selected gems and attempts to add each one to the current project.
        Updates UI after completion.
        :return: None
        """

        selected_item = self.create_project_template_list.selectionModel().currentIndex()
        project_template_path = self.create_project_template_list.model().data(selected_item)
        if not project_template_path:
            return

        folder_dialog = QFileDialog(self.dialog, "Select a Folder and Enter a New Project Name",
                                    registration.get_o3de_projects_folder().as_posix())
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
            if engine_template.create_project(project_path=project_folder[0],
                                              template_path=project_template_path) == 0:
                # Success
                registration.register(project_path=project_folder[0])
                self.refresh_project_list()
                msg_box = QMessageBox(parent=self.dialog)
                msg_box.setWindowTitle("O3DE")
                msg_box.setText(f"Project {project_folder[0]} created.")
                msg_box.exec()
                return

    def create_gem_handler(self):
        """
        Opens the Create Gem pane.  Retrieves a list of available templates for display
        :return: None
        """
        loader = QUiLoader()
        self.create_gem_file = QFile(self.create_gem_ui_file_path.as_posix())

        if not self.create_gem_file:
            logger.error(f'Failed to create gem UI file at {self.create_gem_file}')
            return

        self.create_gem_dialog = loader.load(self.create_gem_file)

        if not self.create_gem_dialog:
            logger.error(f'Failed to load create gem dialog file at {self.create_gem_file}')
            return

        self.create_gem_ok_button = self.create_gem_dialog.findChild(QDialogButtonBox, 'okCancel')
        self.create_gem_ok_button.accepted.connect(self.create_gem_accepted_handler)

        self.create_gem_template_list = self.create_gem_dialog.findChild(QListView, 'gemTemplates')
        self.refresh_create_gem_template_list()

        self.create_gem_dialog.exec()

    def create_gem_accepted_handler(self) -> None:
        """
        Searches the available gems list for selected gems and attempts to add each one to the current gem.
        Updates UI after completion.
        :return: None
        """
        selected_item = self.create_gem_template_list.selectionModel().currentIndex()
        gem_template_path = self.create_gem_template_list.model().data(selected_item)
        if not gem_template_path:
            return

        folder_dialog = QFileDialog(self.dialog, "Select a Folder and Enter a New Gem Name",
                                    registration.get_o3de_gems_folder().as_posix())
        folder_dialog.setFileMode(QFileDialog.AnyFile)
        folder_dialog.setOptions(QFileDialog.ShowDirsOnly)
        gem_count = 0
        gem_name = "MyNewGem"
        while os.path.exists(os.path.join(engine_path, gem_name)):
            gem_name = f"MyNewGem{gem_count}"
            gem_count += 1
        folder_dialog.selectFile(gem_name)
        gem_folder = None
        if folder_dialog.exec():
            gem_folder = folder_dialog.selectedFiles()
        if gem_folder:
            if engine_template.create_gem(gem_path=gem_folder[0],
                                          template_path=gem_template_path) == 0:
                # Success
                registration.register(gem_path=gem_folder[0])
                msg_box = QMessageBox(parent=self.dialog)
                msg_box.setWindowTitle("O3DE")
                msg_box.setText(f"Gem {gem_folder[0]} created.")
                msg_box.exec()
                return

    def create_template_handler(self):
        """
        Opens a foldr select dialog and lets the user select the source folder they want to make a template
        out of, then opens a second folder select dialog to get where they want to put the template and it name
        :return: None
        """

        source_folder = QFileDialog.getExistingDirectory(self.dialog,
                                                         "Select a Folder to make a template out of.",
                                                         registration.get_o3de_folder().as_posix())
        if not source_folder:
            return

        destination_template_folder_dialog = QFileDialog(self.dialog,
                                                         "Select where the template is to be created and named.",
                                                         registration.get_o3de_templates_folder().as_posix())
        destination_template_folder_dialog.setFileMode(QFileDialog.AnyFile)
        destination_template_folder_dialog.setOptions(QFileDialog.ShowDirsOnly)
        destination_folder = None
        if destination_template_folder_dialog.exec():
            destination_folder = destination_template_folder_dialog.selectedFiles()
        if not destination_folder:
            return

        if engine_template.create_template(source_path=source_folder,
                                           template_path=destination_folder[0]) == 0:
            # Success
            registration.register(template_path=destination_folder[0])
            msg_box = QMessageBox(parent=self.dialog)
            msg_box.setWindowTitle("O3DE")
            msg_box.setText(f"Template {destination_folder[0]} created.")
            msg_box.exec()
            return

    def create_from_template_handler(self):
        """
        Opens the Create from_template pane.  Retrieves a list of available from_templates for display
        :return: None
        """
        loader = QUiLoader()
        self.create_from_template_file = QFile(self.create_from_template_ui_file_path.as_posix())

        if not self.create_from_template_file:
            logger.error(f'Failed to create from_template UI file at {self.create_from_template_file}')
            return

        self.create_from_template_dialog = loader.load(self.create_from_template_file)

        if not self.create_from_template_dialog:
            logger.error(f'Failed to load create from_template dialog file at {self.create_from_template_file}')
            return

        self.create_from_template_ok_button = self.create_from_template_dialog.findChild(QDialogButtonBox, 'okCancel')
        self.create_from_template_ok_button.accepted.connect(self.create_from_template_accepted_handler)

        self.create_from_template_list = self.create_from_template_dialog.findChild(QListView, 'genericTemplates')
        self.refresh_create_from_template_list()

        self.create_from_template_dialog.exec()

    def create_from_template_accepted_handler(self) -> None:
        """
        Searches the available gems list for selected gems and attempts to add each one to the current gem.
        Updates UI after completion.
        :return: None
        """
        create_gem_item = self.get_selected_gem_template()
        if not create_gem_item:
            return

        folder_dialog = QFileDialog(self.dialog, "Select a Folder and Enter a New Gem Name",
                                    registration.get_o3de_gems_folder().as_posix())
        folder_dialog.setFileMode(QFileDialog.AnyFile)
        folder_dialog.setOptions(QFileDialog.ShowDirsOnly)
        gem_count = 0
        gem_name = "MyNewGem"
        while os.path.exists(os.path.join(engine_path, gem_name)):
            gem_name = f"MyNewGem{gem_count}"
            gem_count += 1
        folder_dialog.selectFile(gem_name)
        gem_folder = None
        if folder_dialog.exec():
            gem_folder = folder_dialog.selectedFiles()
        if gem_folder:
            if engine_template.create_gem(gem_folder[0], create_gem_item[1]) == 0:
                # Success
                msg_box = QMessageBox(parent=self.dialog)
                msg_box.setWindowTitle("O3DE")
                msg_box.setText(f"gem {os.path.basename(os.path.normpath(gem_folder[0]))} created."
                                "  Build your\nnew gem before hitting OK to launch.")
                msg_box.exec()
                return

    def add_project_handler(self):
        """
        Open a file search dialog looking for a folder which contains a valid project.  If valid
        will update the mru list with the new entry, if invalid will warn the user.
        :return: None
        """
        project_folder = QFileDialog.getExistingDirectory(self.dialog, "Select Project Folder",
                                                          registration.get_o3de_projects_folder().as_posix())
        if project_folder:
            if registration.register(project_path=project_folder) == 0:
                # Success
                self.refresh_project_list()

                msg_box = QMessageBox(parent=self.dialog)
                msg_box.setWindowTitle("O3DE")
                msg_box.setText(f"Added Project {project_folder}.")
                msg_box.exec()
        return

    def add_gem_handler(self):
        """
        Open a file search dialog looking for a folder which contains a gem.  If valid
        will update the mru list with the new entry, if invalid will warn the user.
        :return: None
        """
        gem_folder = QFileDialog.getExistingDirectory(self.dialog, "Select Gem Folder",
                                                      registration.get_o3de_gems_folder().as_posix())
        if gem_folder:
            if registration.register(gem_path=gem_folder) == 0:
                # Success
                msg_box = QMessageBox(parent=self.dialog)
                msg_box.setWindowTitle("O3DE")
                msg_box.setText(f"Added Gem {gem_folder}.")
                msg_box.exec()
        return

    def add_template_handler(self):
        """
        Open a file search dialog looking for a folder which contains a valid template.  If valid
        will update the mru list with the new entry, if invalid will warn the user.
        :return: None
        """
        template_folder = QFileDialog.getExistingDirectory(self.dialog, "Select Template Folder",
                                                           registration.get_o3de_templates_folder().as_posix())
        if template_folder:
            if registration.register(template_path=template_folder) == 0:
                # Success
                msg_box = QMessageBox(parent=self.dialog)
                msg_box.setWindowTitle("O3DE")
                msg_box.setText(f"Added Template {template_folder}.")
                msg_box.exec()
        return

    def add_restricted_handler(self):
        """
        Open a file search dialog looking for a folder which contains a valid template.  If valid
        will update the mru list with the new entry, if invalid will warn the user.
        :return: None
        """
        restricted_folder = QFileDialog.getExistingDirectory(self.dialog, "Select Restricted Folder",
                                                             registration.get_o3de_restricted_folder().as_posix())
        if restricted_folder:
            if registration.register(restricted_path=restricted_folder) == 0:
                # Success
                msg_box = QMessageBox(parent=self.dialog)
                msg_box.setWindowTitle("O3DE")
                msg_box.setText(f"Added Restricted {restricted_folder}.")
                msg_box.exec()
        return

    def remove_project_handler(self):
        """
        Open a file search dialog looking for a folder which contains a valid project.  If valid
        will update the mru list with the new entry, if invalid will warn the user.
        :return: None
        """
        project_folder = QFileDialog.getExistingDirectory(self.dialog, "Select Project Folder",
                                                          registration.get_o3de_projects_folder().as_posix())
        if project_folder:
            if registration.register(project_path=project_folder, remove=True) == 0:
                # Success
                self.refresh_project_list()

                msg_box = QMessageBox(parent=self.dialog)
                msg_box.setWindowTitle("O3DE")
                msg_box.setText(f"Removed Project {project_folder}.")
                msg_box.exec()
        return

    def remove_gem_handler(self):
        """
        Open a file search dialog looking for a folder which contains a gem.  If valid
        will update the mru list with the new entry, if invalid will warn the user.
        :return: None
        """
        gem_folder = QFileDialog.getExistingDirectory(self.dialog, "Select Gem Folder",
                                                      registration.get_o3de_gems_folder().as_posix())
        if gem_folder:
            if registration.register(gem_path=gem_folder, remove=True) == 0:
                # Success
                msg_box = QMessageBox(parent=self.dialog)
                msg_box.setWindowTitle("O3DE")
                msg_box.setText(f"Removed Gem {gem_folder}.")
                msg_box.exec()
        return

    def remove_template_handler(self):
        """
        Open a file search dialog looking for a folder which contains a valid template.  If valid
        will update the mru list with the new entry, if invalid will warn the user.
        :return: None
        """
        template_folder = QFileDialog.getExistingDirectory(self.dialog, "Select Template Folder",
                                                           registration.get_o3de_templates_folder().as_posix())
        if template_folder:
            if registration.register(template_path=template_folder, remove=True) == 0:
                # Success
                msg_box = QMessageBox(parent=self.dialog)
                msg_box.setWindowTitle("O3DE")
                msg_box.setText(f"Removed Template {template_folder}.")
                msg_box.exec()
        return

    def remove_restricted_handler(self):
        """
        Open a file search dialog looking for a folder which contains a valid template.  If valid
        will update the mru list with the new entry, if invalid will warn the user.
        :return: None
        """
        restricted_folder = QFileDialog.getExistingDirectory(self.dialog, "Select Restricted Folder",
                                                             registration.get_o3de_restricted_folder().as_posix())
        if restricted_folder:
            if registration.register(restricted_path=restricted_folder, remove=True) == 0:
                # Success
                msg_box = QMessageBox(parent=self.dialog)
                msg_box.setWindowTitle("O3DE")
                msg_box.setText(f"Removed Restricted {restricted_folder}.")
                msg_box.exec()
        return

    def manage_runtime_project_gem_targets_handler(self):
        """
        Opens the Gem management pane.  Waits for the load thread to complete if still running and displays all
        active gems for the current project as well as all available gems which aren't currently active.
        :return: None
        """

        if not self.get_selected_project_path():
            msg_box = QMessageBox(parent=self.dialog)
            msg_box.setWindowTitle("O3DE")
            msg_box.setText("Please select a project")
            msg_box.exec()
            return

        loader = QUiLoader()
        self.manage_project_gem_targets_file = QFile(self.manage_project_gem_targets_ui_file_path.as_posix())

        if not self.manage_project_gem_targets_file:
            logger.error(f'Failed to load manage gem targets UI file at {self.manage_project_gem_targets_ui_file_path}')
            return

        self.manage_project_gem_targets_dialog = loader.load(self.manage_project_gem_targets_file)

        if not self.manage_project_gem_targets_dialog:
            logger.error(f'Failed to load gems dialog file at {self.manage_project_gem_targets_ui_file_path.as_posix()}')
            return

        self.manage_project_gem_targets_dialog.setWindowTitle(f"Manage Runtime Gem Targets for Project:"
                                                              f" {self.get_selected_project_name()}")

        self.add_gem_button = self.manage_project_gem_targets_dialog.findChild(QPushButton, 'addGemTargetsButton')
        self.add_gem_button.clicked.connect(self.add_runtime_project_gem_targets_handler)

        self.available_gem_targets_list = self.manage_project_gem_targets_dialog.findChild(QListView,
                                                                                           'availableGemTargetsList')
        self.refresh_runtime_project_gem_targets_available_list()

        self.remove_project_gem_targets_button = self.manage_project_gem_targets_dialog.findChild(QPushButton,
                                                                                            'removeGemTargetsButton')
        self.remove_project_gem_targets_button.clicked.connect(self.remove_runtime_project_gem_targets_handler)

        self.enabled_gem_targets_list = self.manage_project_gem_targets_dialog.findChild(QListView,
                                                                                         'enabledGemTargetsList')
        self.refresh_runtime_project_gem_targets_enabled_list()

        self.manage_project_gem_targets_dialog.exec()

    def manage_tool_project_gem_targets_handler(self):
        """
        Opens the Gem management pane.  Waits for the load thread to complete if still running and displays all
        active gems for the current project as well as all available gems which aren't currently active.
        :return: None
        """

        if not self.get_selected_project_path():
            msg_box = QMessageBox(parent=self.dialog)
            msg_box.setWindowTitle("O3DE")
            msg_box.setText("Please select a project")
            msg_box.exec()
            return

        loader = QUiLoader()
        self.manage_project_gem_targets_file = QFile(self.manage_project_gem_targets_ui_file_path.as_posix())

        if not self.manage_project_gem_targets_file:
            logger.error(f'Failed to load manage gem targets UI file at {self.manage_project_gem_targets_ui_file_path}')
            return

        self.manage_project_gem_targets_dialog = loader.load(self.manage_project_gem_targets_file)

        if not self.manage_project_gem_targets_dialog:
            logger.error(
                f'Failed to load gems dialog file at {self.manage_project_gem_targets_ui_file_path.as_posix()}')
            return

        self.manage_project_gem_targets_dialog.setWindowTitle(f"Manage Tool Gem Targets for Project:"
                                                              f" {self.get_selected_project_name()}")

        self.add_gem_button = self.manage_project_gem_targets_dialog.findChild(QPushButton, 'addGemTargetsButton')
        self.add_gem_button.clicked.connect(self.add_tool_project_gem_targets_handler)

        self.available_gem_targets_list = self.manage_project_gem_targets_dialog.findChild(QListView,
                                                                                           'availableGemTargetsList')
        self.refresh_tool_project_gem_targets_available_list()

        self.remove_project_gem_targets_button = self.manage_project_gem_targets_dialog.findChild(QPushButton,
                                                                                            'removeGemTargetsButton')
        self.remove_project_gem_targets_button.clicked.connect(self.remove_tool_project_gem_targets_handler)

        self.enabled_gem_targets_list = self.manage_project_gem_targets_dialog.findChild(QListView,
                                                                                         'enabledGemTargetsList')
        self.refresh_tool_project_gem_targets_enabled_list()

        self.manage_project_gem_targets_dialog.exec()

    def manage_server_project_gem_targets_handler(self):
        """
        Opens the Gem management pane.  Waits for the load thread to complete if still running and displays all
        active gems for the current project as well as all available gems which aren't currently active.
        :return: None
        """

        if not self.get_selected_project_path():
            msg_box = QMessageBox(parent=self.dialog)
            msg_box.setWindowTitle("O3DE")
            msg_box.setText("Please select a project")
            msg_box.exec()
            return

        loader = QUiLoader()
        self.manage_project_gem_targets_file = QFile(self.manage_project_gem_targets_ui_file_path.as_posix())

        if not self.manage_project_gem_targets_file:
            logger.error(f'Failed to load manage gem targets UI file at {self.manage_project_gem_targets_ui_file_path}')
            return

        self.manage_project_gem_targets_dialog = loader.load(self.manage_project_gem_targets_file)

        if not self.manage_project_gem_targets_dialog:
            logger.error(
                f'Failed to load gems dialog file at {self.manage_project_gem_targets_ui_file_path.as_posix()}')
            return

        self.manage_project_gem_targets_dialog.setWindowTitle(f"Manage Server Gem Targets for Project:"
                                                              f" {self.get_selected_project_name()}")

        self.add_gem_button = self.manage_project_gem_targets_dialog.findChild(QPushButton, 'addGemTargetsButton')
        self.add_gem_button.clicked.connect(self.add_server_project_gem_targets_handler)

        self.available_gem_targets_list = self.manage_project_gem_targets_dialog.findChild(QListView,
                                                                                           'availableGemTargetsList')
        self.refresh_server_project_gem_targets_available_list()

        self.remove_project_gem_targets_button = self.manage_project_gem_targets_dialog.findChild(QPushButton,
                                                                                            'removeGemTargetsButton')
        self.remove_project_gem_targets_button.clicked.connect(self.remove_server_project_gem_targets_handler)

        self.enabled_gem_targets_list = self.manage_project_gem_targets_dialog.findChild(QListView,
                                                                                         'enabledGemTargetsList')
        self.refresh_server_project_gem_targets_enabled_list()

        self.manage_project_gem_targets_dialog.exec()

    def manage_project_gem_targets_get_selected_available_gems(self) -> list:
        selected_items = self.available_gem_targets_list.selectionModel().selectedRows()
        return [(self.available_gem_targets_list.model().data(item)) for item in selected_items]

    def manage_project_gem_targets_get_selected_enabled_gems(self) -> list:
        selected_items = self.enabled_gem_targets_list.selectionModel().selectedRows()
        return [(self.enabled_gem_targets_list.model().data(item)) for item in selected_items]

    def add_runtime_project_gem_targets_handler(self) -> None:
        gem_paths = registration.get_all_gems()
        for gem_target in self.manage_project_gem_targets_get_selected_available_gems():
            for gem_path in gem_paths:
                this_gems_targets = registration.get_gem_targets(gem_path=gem_path)
                for this_gem_target in this_gems_targets:
                    if gem_target == this_gem_target:
                        registration.add_gem_to_project(gem_path=gem_path,
                                                        gem_target=gem_target,
                                                        project_path=self.get_selected_project_path(),
                                                        runtime_dependency=True)
                        self.refresh_runtime_project_gem_targets_available_list()
                        self.refresh_runtime_project_gem_targets_enabled_list()
                        return
        self.refresh_runtime_project_gem_targets_available_list()
        self.refresh_runtime_project_gem_targets_enabled_list()

    def remove_runtime_project_gem_targets_handler(self):
        gem_paths = registration.get_all_gems()
        for gem_target in self.manage_project_gem_targets_get_selected_enabled_gems():
            for gem_path in gem_paths:
                this_gems_targets = registration.get_gem_targets(gem_path=gem_path)
                for this_gem_target in this_gems_targets:
                    if gem_target == this_gem_target:
                        registration.remove_gem_from_project(gem_path=gem_path,
                                                             gem_target=gem_target,
                                                             project_path=self.get_selected_project_path(),
                                                             runtime_dependency=True)
                        self.refresh_runtime_project_gem_targets_available_list()
                        self.refresh_runtime_project_gem_targets_enabled_list()
                        return
        self.refresh_runtime_project_gem_targets_available_list()
        self.refresh_runtime_project_gem_targets_enabled_list()

    def add_tool_project_gem_targets_handler(self) -> None:
        gem_paths = registration.get_all_gems()
        for gem_target in self.manage_project_gem_targets_get_selected_available_gems():
            for gem_path in gem_paths:
                this_gems_targets = registration.get_gem_targets(gem_path=gem_path)
                for this_gem_target in this_gems_targets:
                    if gem_target == this_gem_target:
                        registration.add_gem_to_project(gem_path=gem_path,
                                                        gem_target=gem_target,
                                                        project_path=self.get_selected_project_path(),
                                                        tool_dependency=True)
                        self.refresh_tool_project_gem_targets_available_list()
                        self.refresh_tool_project_gem_targets_enabled_list()
                        return
        self.refresh_tool_project_gem_targets_available_list()
        self.refresh_tool_project_gem_targets_enabled_list()

    def remove_tool_project_gem_targets_handler(self):
        gem_paths = registration.get_all_gems()
        for gem_target in self.manage_project_gem_targets_get_selected_enabled_gems():
            for gem_path in gem_paths:
                this_gems_targets = registration.get_gem_targets(gem_path=gem_path)
                for this_gem_target in this_gems_targets:
                    if gem_target == this_gem_target:
                        registration.remove_gem_from_project(gem_path=gem_path,
                                                             gem_target=gem_target,
                                                             project_path=self.get_selected_project_path(),
                                                             tool_dependency=True)
                        self.refresh_tool_project_gem_targets_available_list()
                        self.refresh_tool_project_gem_targets_enabled_list()
                        return
        self.refresh_tool_project_gem_targets_available_list()
        self.refresh_tool_project_gem_targets_enabled_list()
        
    def add_server_project_gem_targets_handler(self) -> None:
        gem_paths = registration.get_all_gems()
        for gem_target in self.manage_project_gem_targets_get_selected_available_gems():
            for gem_path in gem_paths:
                this_gems_targets = registration.get_gem_targets(gem_path=gem_path)
                for this_gem_target in this_gems_targets:
                    if gem_target == this_gem_target:
                        registration.add_gem_to_project(gem_path=gem_path,
                                                        gem_target=gem_target,
                                                        project_path=self.get_selected_project_path(),
                                                        server_dependency=True)
                        self.refresh_server_project_gem_targets_available_list()
                        self.refresh_server_project_gem_targets_enabled_list()
                        return
        self.refresh_server_project_gem_targets_available_list()
        self.refresh_server_project_gem_targets_enabled_list()

    def remove_server_project_gem_targets_handler(self):
        gem_paths = registration.get_all_gems()
        for gem_target in self.manage_project_gem_targets_get_selected_enabled_gems():
            for gem_path in gem_paths:
                this_gems_targets = registration.get_gem_targets(gem_path=gem_path)
                for this_gem_target in this_gems_targets:
                    if gem_target == this_gem_target:
                        registration.remove_gem_from_project(gem_path=gem_path,
                                                             gem_target=gem_target,
                                                             project_path=self.get_selected_project_path(),
                                                             server_dependency=True)
                        self.refresh_server_project_gem_targets_available_list()
                        self.refresh_server_project_gem_targets_enabled_list()
                        return
        self.refresh_server_project_gem_targets_available_list()
        self.refresh_server_project_gem_targets_enabled_list()

    def refresh_runtime_project_gem_targets_enabled_list(self) -> None:
        enabled_project_gem_targets_model = QStandardItemModel()
        enabled_project_gem_targets = registration.get_project_runtime_gem_targets(
            project_path=self.get_selected_project_path())
        for gem_target in sorted(enabled_project_gem_targets):
            model_item = QStandardItem(gem_target)
            enabled_project_gem_targets_model.appendRow(model_item)
        self.enabled_gem_targets_list.setModel(enabled_project_gem_targets_model)

    def refresh_runtime_project_gem_targets_available_list(self) -> None:
        available_project_gem_targets_model = QStandardItemModel()
        enabled_project_gem_targets = registration.get_project_runtime_gem_targets(
            project_path=self.get_selected_project_path())
        all_gem_targets = registration.get_all_gem_targets()
        for gem_target in sorted(all_gem_targets):
            if gem_target not in enabled_project_gem_targets:
                model_item = QStandardItem(gem_target)
                available_project_gem_targets_model.appendRow(model_item)
        self.available_gem_targets_list.setModel(available_project_gem_targets_model)
        
    def refresh_tool_project_gem_targets_enabled_list(self) -> None:
        enabled_project_gem_targets_model = QStandardItemModel()
        enabled_project_gem_targets = registration.get_project_tool_gem_targets(
            project_path=self.get_selected_project_path())
        for gem_target in sorted(enabled_project_gem_targets):
            model_item = QStandardItem(gem_target)
            enabled_project_gem_targets_model.appendRow(model_item)
        self.enabled_gem_targets_list.setModel(enabled_project_gem_targets_model)

    def refresh_tool_project_gem_targets_available_list(self) -> None:
        available_project_gem_targets_model = QStandardItemModel()
        enabled_project_gem_targets = registration.get_project_tool_gem_targets(
            project_path=self.get_selected_project_path())
        all_gem_targets = registration.get_all_gem_targets()
        for gem_target in sorted(all_gem_targets):
            if gem_target not in enabled_project_gem_targets:
                model_item = QStandardItem(gem_target)
                available_project_gem_targets_model.appendRow(model_item)
        self.available_gem_targets_list.setModel(available_project_gem_targets_model)
        
    def refresh_server_project_gem_targets_enabled_list(self) -> None:
        enabled_project_gem_targets_model = QStandardItemModel()
        enabled_project_gem_targets = registration.get_project_server_gem_targets(
            project_path=self.get_selected_project_path())
        for gem_target in sorted(enabled_project_gem_targets):
            model_item = QStandardItem(gem_target)
            enabled_project_gem_targets_model.appendRow(model_item)
        self.enabled_gem_targets_list.setModel(enabled_project_gem_targets_model)

    def refresh_server_project_gem_targets_available_list(self) -> None:
        available_project_gem_targets_model = QStandardItemModel()
        enabled_project_gem_targets = registration.get_project_server_gem_targets(
            project_path=self.get_selected_project_path())
        all_gem_targets = registration.get_all_gem_targets()
        for gem_target in sorted(all_gem_targets):
            if gem_target not in enabled_project_gem_targets:
                model_item = QStandardItem(gem_target)
                available_project_gem_targets_model.appendRow(model_item)
        self.available_gem_targets_list.setModel(available_project_gem_targets_model)

    def refresh_create_project_template_list(self) -> None:
        self.create_project_template_model = QStandardItemModel()
        for project_template_path in registration.get_project_templates():
            model_item = QStandardItem(project_template_path)
            self.create_project_template_model.appendRow(model_item)
        self.create_project_template_list.setModel(self.create_project_template_model)

    def refresh_create_gem_template_list(self) -> None:
        self.create_gem_template_model = QStandardItemModel()
        for gem_template_path in registration.get_gem_templates():
            model_item = QStandardItem(gem_template_path)
            self.create_gem_template_model.appendRow(model_item)
        self.create_gem_template_list.setModel(self.create_gem_template_model)

    def refresh_create_from_template_list(self) -> None:
        self.create_from_template_model = QStandardItemModel()
        for generic_template_path in registration.get_generic_templates():
            model_item = QStandardItem(generic_template_path)
            self.create_from_template_model.appendRow(model_item)
        self.create_from_template_list.setModel(self.create_from_template_model)

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
        Retrieve the current MRU list.  Does not perform validation that the projects still appear valid
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
    my_dialog = ProjectManagerDialog()
    dialog_app.exec_()
    sys.exit(0)
