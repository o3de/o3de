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

import pytest
import os
import sys
import tempfile
import logging
from unittest.mock import MagicMock

logger = logging.getLogger()

# Code lives one folder above
projects_path = os.path.realpath(os.path.join(os.path.dirname(__file__), '..'))
sys.path.append(projects_path)

from pyside import add_pyside_environment, is_configuration_valid
from ly_test_tools import WINDOWS

project_marker_file = "project.json"

class ProjectHelper:
    def __init__(self):
        self._temp_directory = tempfile.TemporaryDirectory()
        self.temp_project_root = self._temp_directory.name
        self.temp_file_dir = os.path.join(self.temp_project_root, "o3de")

        self.application = None
        self.dialog = None

        if not os.path.exists(self.temp_file_dir):
            os.makedirs(self.temp_file_dir)

    def create_empty_projects(self):
        self.project_1_dir = os.path.join(self.temp_project_root, "Project1")
        if not os.path.exists(self.project_1_dir):
            os.makedirs(self.project_1_dir)
        with open(os.path.join(self.project_1_dir,project_marker_file), 'w') as marker_file:
            marker_file.write("{}")
        self.project_2_dir = os.path.join(self.temp_project_root, "Project2")
        if not os.path.exists(self.project_2_dir):
            os.makedirs(self.project_2_dir)
        with open(os.path.join(self.project_2_dir, project_marker_file), 'w') as marker_file:
            marker_file.write("{}")
        self.project_3_dir = os.path.join(self.temp_project_root, "Project3")
        if not os.path.exists(self.project_3_dir):
            os.makedirs(self.project_3_dir)
        with open(os.path.join(self.project_3_dir, project_marker_file), 'w') as marker_file:
            marker_file.write("{}")
        self.invalid_project_dir = os.path.join(self.temp_project_root, "InvalidProject")


    def setup_dialog_test(self, workspace):
        add_pyside_environment(workspace.paths.build_directory())

        if not is_configuration_valid(workspace):
            # This is essentially skipif debug.  Our debug tests use our profile version of python, but that means we'd
            # need to use the profile version of PySide which works with the profile QT libs which aren't in the debug
            # folder we've built.
            return None
        from PySide2.QtWidgets import QApplication, QMessageBox

        if QApplication.instance():
            self.application = QApplication.instance()
        else:
            self.application = QApplication(sys.argv)
        assert self.application

        from projects import ProjectDialog

        try:
            self.dialog = ProjectDialog(settings_folder=self.temp_file_dir)
            return self.dialog
        except Exception as e:
            logger.error(f'Failed to create ProjectDialog with error {e}')
        return None

    def create_project_from_template(self, project_name) -> bool:
        """
        Uses the dialog to create a temporary project based on the first template found
        :param project_name: Name of project to create.  Will be created under temp_project_root
        :return: True for Success, False for failure
        """
        from PySide2.QtWidgets import QWidget, QFileDialog
        from projects import ProjectDialog

        QWidget.exec = MagicMock()
        self.dialog.create_project_handler()
        QWidget.exec.assert_called_once()

        assert len(self.dialog.project_templates), 'Failed to find any project templates'
        ProjectDialog.get_selected_project_template = MagicMock(return_value=self.dialog.project_templates[0])

        QFileDialog.exec = MagicMock()
        create_project_dir = os.path.join(self.temp_project_root, project_name)
        QFileDialog.selectedFiles = MagicMock(return_value=[create_project_dir])
        self.dialog.create_project_accepted_handler()
        assert os.path.isdir(create_project_dir), f"Expected project folder not found at {create_project_dir}"
        assert QWidget.exec.call_count == 2, "Message box confirming project creation failed to show"


@pytest.fixture
def project_helper():
    return ProjectHelper()


@pytest.mark.skipif(not WINDOWS, reason="PySide2 only works on windows currently")
@pytest.mark.parametrize('project', ['']) # Workspace wants a project, but this test is not project dependent
def test_logger_handler(workspace, project_helper):
    my_dialog = project_helper.setup_dialog_test(workspace)
    if not my_dialog:
        return

    from PySide2.QtWidgets import QMessageBox
    QMessageBox.warning = MagicMock()
    logger.error(f'Testing logger')
    QMessageBox.warning.assert_called_once()


@pytest.mark.skipif(not WINDOWS, reason="PySide2 only works on windows currently")
@pytest.mark.parametrize('project', ['']) # Workspace wants a project, but this test is not project dependent
def test_mru_list(workspace, project_helper):
    my_dialog = project_helper.setup_dialog_test(workspace)
    if not my_dialog:
        return

    project_helper.create_empty_projects()

    from PySide2.QtWidgets import QMessageBox

    mru_list = my_dialog.get_mru_list()
    assert len(mru_list) == 0, f'MRU list unexpectedly had entries: {mru_list}'

    QMessageBox.warning = MagicMock()
    my_dialog.add_new_project('TestProjectInvalid')
    mru_list = my_dialog.get_mru_list()
    assert len(mru_list) == 0, f'MRU list unexpectedly added an invalid project : {mru_list}'
    QMessageBox.warning.assert_called_once()

    my_dialog.add_new_project(project_helper.project_1_dir)
    mru_list = my_dialog.get_mru_list()
    assert len(mru_list) == 1, f'MRU list failed to add project at {project_helper.project_1_dir}'

    my_dialog.add_new_project(project_helper.project_1_dir)
    mru_list = my_dialog.get_mru_list()
    assert len(mru_list) == 1, f'MRU list added project at {project_helper.project_1_dir} a second time : {mru_list}'

    my_dialog.update_mru_list(project_helper.project_1_dir)
    mru_list = my_dialog.get_mru_list()
    assert len(mru_list) == 1, f'MRU list added project at {project_helper.project_1_dir} a second time : {mru_list}'

    my_dialog.add_new_project(project_helper.project_2_dir)
    mru_list = my_dialog.get_mru_list()
    assert len(mru_list) == 2, f'MRU list failed to add project at {project_helper.project_2_dir}'

    assert mru_list[0] == project_helper.project_2_dir, f"{project_helper.project_2_dir} wasn't first item"
    assert mru_list[1] == project_helper.project_1_dir, f"{project_helper.project_1_dir} wasn't second item"

    my_dialog.update_mru_list(project_helper.project_1_dir)
    mru_list = my_dialog.get_mru_list()
    assert len(mru_list) == 2, f'MRU list added wrong items {mru_list}'
    assert mru_list[0] == project_helper.project_1_dir, f"{project_helper.project_1_dir} wasn't first item"
    assert mru_list[1] == project_helper.project_2_dir, f"{project_helper.project_2_dir} wasn't second item"

    my_dialog.add_new_project(project_helper.invalid_project_dir)
    mru_list = my_dialog.get_mru_list()
    assert len(mru_list) == 2, f'MRU list added invalid item {mru_list}'
    assert mru_list[0] == project_helper.project_1_dir, f"{project_helper.project_1_dir} wasn't first item"
    assert mru_list[1] == project_helper.project_2_dir, f"{project_helper.project_2_dir} wasn't second item"

    my_dialog.add_new_project(project_helper.project_3_dir)
    mru_list = my_dialog.get_mru_list()
    assert len(mru_list) == 3, f'MRU list failed to add {project_helper.project_3_dir} : {mru_list}'
    assert mru_list[0] == project_helper.project_3_dir, f"{project_helper.project_3_dir} wasn't first item"
    assert mru_list[1] == project_helper.project_1_dir, f"{project_helper.project_1_dir} wasn't second item"
    assert mru_list[2] == project_helper.project_2_dir, f"{project_helper.project_2_dir} wasn't third item"


@pytest.mark.skipif(not WINDOWS, reason="PySide2 only works on windows currently")
@pytest.mark.parametrize('project', ['']) # Workspace wants a project, but this test is not project dependent
def test_create_project(workspace, project_helper):
    my_dialog = project_helper.setup_dialog_test(workspace)
    if not my_dialog:
        return

    project_helper.create_project_from_template("TestCreateProject")


@pytest.mark.skipif(not WINDOWS, reason="PySide2 only works on windows currently")
@pytest.mark.parametrize('project', ['']) # Workspace wants a project, but this test is not project dependent
def test_add_remove_gems(workspace, project_helper):
    my_dialog = project_helper.setup_dialog_test(workspace)
    if not my_dialog:
        return

    my_project_name = "TestAddRemoveGems"

    project_helper.create_project_from_template(my_project_name)
    my_project_path = os.path.join(project_helper.temp_project_root, my_project_name)

    from PySide2.QtWidgets import QWidget, QFileDialog
    from projects import ProjectDialog

    assert my_dialog.path_for_selection() == my_project_path, "Gems project not selected"
    QWidget.exec = MagicMock()
    my_dialog.manage_gems_handler()
    assert my_dialog.manage_gems_dialog, "No gem management dialog created"
    QWidget.exec.assert_called_once()

    assert len(my_dialog.gems_list), 'Failed to find any gems'
    my_test_gem = my_dialog.gems_list[0]
    my_test_gem_name = my_test_gem.get("Name")
    my_test_gem_path = my_test_gem.get("Path")
    my_test_gem_selection = (my_test_gem_name, my_test_gem_path)
    ProjectDialog.get_selected_add_gems = MagicMock(return_value=[my_test_gem_selection])


    assert my_test_gem_name, "No Name set in test gem"
    assert my_test_gem_name not in my_dialog.project_gem_list, f'Gem {my_test_gem_name} already in project gem list'
    my_dialog.add_gems_handler()
    assert my_test_gem_name in my_dialog.project_gem_list, f'Gem {my_test_gem_name} failed to add to gem list'
    ProjectDialog.get_selected_project_gems = MagicMock(return_value=[my_test_gem_name])
    my_dialog.remove_gems_handler()
    assert my_test_gem_name not in my_dialog.project_gem_list, f'Gem {my_test_gem_name} still in project gem list'


