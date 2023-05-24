#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pytest
from unittest.mock import patch
from inspect import signature
import pathlib

from o3de import manifest, engine_properties, register, engine_template, enable_gem, disable_gem, project_properties, repo, download

# If any tests are failing in this, this means the interface Project Manager depends on has changed.
# This likely means that some Project Manager functionality has been broken.
# Please ensure that functionality found in o3de/Code/Tools/ProjectManager/Source/PythonBindings.cpp
# still works with the affected interface before updating the test to the latest interface.


# manifest interface
def test_get_engine_json_data():
    sig = signature(manifest.get_engine_json_data)
    assert len(sig.parameters) >= 2

    engine_path = list(sig.parameters.values())[1]
    assert engine_path.name == 'engine_path'
    assert engine_path.annotation == str or engine_path.annotation == pathlib.Path

    assert sig.return_annotation == dict

def test_load_o3de_manifest():
    sig = signature(manifest.load_o3de_manifest)

    assert sig.return_annotation == dict

def test_get_o3de_gems_folder():
    sig = signature(manifest.get_o3de_gems_folder)

    assert sig.return_annotation == pathlib.Path

def test_get_o3de_projects_folder():
    sig = signature(manifest.get_o3de_projects_folder)

    assert sig.return_annotation == pathlib.Path

def test_get_o3de_restricted_folder():
    sig = signature(manifest.get_o3de_restricted_folder)

    assert sig.return_annotation == pathlib.Path

def test_get_o3de_templates_folder():
    sig = signature(manifest.get_o3de_templates_folder)

    assert sig.return_annotation == pathlib.Path

def test_get_o3de_third_party_folder():
    sig = signature(manifest.get_o3de_third_party_folder)

    assert sig.return_annotation == pathlib.Path

def test_get_manifest_engines():
    sig = signature(manifest.get_manifest_engines)

    assert sig.return_annotation == list

def test_get_this_engine_path():
    sig = signature(manifest.get_this_engine_path)

    assert sig.return_annotation == pathlib.Path

def test_get_registered():
    sig = signature(manifest.get_registered)
    assert len(sig.parameters) >= 1

    engine_name = list(sig.parameters.values())[0]
    assert engine_name.name == 'engine_name'
    assert engine_name.annotation == str

    assert sig.return_annotation == pathlib.Path

def test_get_engine_gems():
    sig = signature(manifest.get_engine_gems)

    assert sig.return_annotation == list

def test_get_all_gems():
    sig = signature(manifest.get_all_gems)
    assert len(sig.parameters) >= 1

    project_path = list(sig.parameters.values())[0]
    assert project_path.name == 'project_path'
    assert project_path.annotation == pathlib.Path

    assert sig.return_annotation == list

def test_get_gem_json_data():
    sig = signature(manifest.get_gem_json_data)
    assert len(sig.parameters) >= 3

    parameters = list(sig.parameters.values())
    gem_path = parameters[1]
    assert gem_path.name == 'gem_path'
    assert gem_path.annotation == str or gem_path.annotation == pathlib.Path
    project_path = parameters[2]
    assert project_path.name == 'project_path'
    assert project_path.annotation == str or project_path.annotation == pathlib.Path

    assert sig.return_annotation == dict

def test_get_project_json_data():
    sig = signature(manifest.get_project_json_data)
    assert len(sig.parameters) >= 2

    project_path = list(sig.parameters.values())[1]
    assert project_path.name == 'project_path'
    assert project_path.annotation == str or project_path.annotation == pathlib.Path

    assert sig.return_annotation == dict

def test_get_manifest_projects():
    sig = signature(manifest.get_manifest_projects)

    assert sig.return_annotation == list

def test_get_engine_projects():
    sig = signature(manifest.get_engine_projects)

    assert sig.return_annotation == list

def test_get_template_json_data():
    sig = signature(manifest.get_template_json_data)
    assert len(sig.parameters) >= 3

    parameters = list(sig.parameters.values())
    template_path = parameters[1]
    assert template_path.name == 'template_path'
    assert template_path.annotation == str or template_path.annotation == pathlib.Path
    project_path = parameters[2]
    assert project_path.name == 'project_path'
    assert project_path.annotation == str or project_path.annotation == pathlib.Path

    assert sig.return_annotation == dict

def test_get_templates_for_project_creation():
    sig = signature(manifest.get_templates_for_project_creation)

    assert sig.return_annotation == list

def test_get_repo_json_data():
    sig = signature(manifest.get_repo_json_data)
    assert len(sig.parameters) >= 1

    repo_uri = list(sig.parameters.values())[0]
    assert repo_uri.name == 'repo_uri'
    assert repo_uri.annotation == str

    assert sig.return_annotation == dict

def test_get_repo_path():
    sig = signature(manifest.get_repo_path)
    assert len(sig.parameters) >= 1

    repo_uri = list(sig.parameters.values())[0]
    assert repo_uri.name == 'repo_uri'
    assert repo_uri.annotation == str

    assert sig.return_annotation == pathlib.Path

def test_get_manifest_repos():
    sig = signature(manifest.get_manifest_repos)

    assert sig.return_annotation == list

# engine_properties interface
def test_edit_engine_props():
    sig = signature(engine_properties.edit_engine_props)
    assert len(sig.parameters) >= 4

    parameters = list(sig.parameters.values())
    engine_path = parameters[0]
    assert engine_path.name == 'engine_path'
    assert engine_path.annotation == pathlib.Path
    new_name = parameters[2]
    assert new_name.name == 'new_name'
    assert new_name.annotation == str
    engine_version = parameters[3]
    assert engine_version.name == 'new_version'
    assert engine_version.annotation == str

    assert sig.return_annotation == int

# register interface
def test_register():
    sig = signature(register.register)
    assert len(sig.parameters) >= 18

    parameters = list(sig.parameters.values())
    engine_path = parameters[0]
    assert engine_path.name == 'engine_path'
    assert engine_path.annotation == pathlib.Path
    project_path = parameters[1]
    assert project_path.name == 'project_path'
    assert project_path.annotation == pathlib.Path
    gem_path = parameters[2]
    assert gem_path.name == 'gem_path'
    assert gem_path.annotation == pathlib.Path
    template_path = parameters[4]
    assert template_path.name == 'template_path'
    assert template_path.annotation == pathlib.Path
    repo_uri = parameters[6]
    assert repo_uri.name == 'repo_uri'
    assert repo_uri.annotation == str
    default_projects_folder = parameters[8]
    assert default_projects_folder.name == 'default_projects_folder'
    assert default_projects_folder.annotation == pathlib.Path
    default_gems_folder = parameters[9]
    assert default_gems_folder.name == 'default_gems_folder'
    assert default_gems_folder.annotation == pathlib.Path
    default_templates_folder = parameters[10]
    assert default_templates_folder.name == 'default_templates_folder'
    assert default_templates_folder.annotation == pathlib.Path
    default_third_party_folder = parameters[12]
    assert default_third_party_folder.name == 'default_third_party_folder'
    assert default_third_party_folder.annotation == pathlib.Path
    external_subdir_engine_path = parameters[13]
    assert external_subdir_engine_path.name == 'external_subdir_engine_path'
    assert external_subdir_engine_path.annotation == pathlib.Path
    external_subdir_project_path = parameters[14]
    assert external_subdir_project_path.name == 'external_subdir_project_path'
    assert external_subdir_project_path.annotation == pathlib.Path
    external_subdir_gem_path = parameters[15]
    assert external_subdir_gem_path.name == 'external_subdir_gem_path'
    assert external_subdir_gem_path.annotation == pathlib.Path
    remove = parameters[16]
    assert remove.name == 'remove'
    assert remove.annotation == bool
    force = parameters[17]
    assert force.name == 'force'
    assert force.annotation == bool

    assert sig.return_annotation == int

def test_remove_invalid_o3de_projects():
    sig = signature(register.remove_invalid_o3de_projects)

    assert sig.return_annotation == int

# cmake interface
def test_get_enabled_gem_cmake_file():
    sig = signature(manifest.get_enabled_gem_cmake_file)
    assert len(sig.parameters) >= 2

    project_path = list(sig.parameters.values())[1]
    assert project_path.name == 'project_path'
    assert project_path.annotation == str or project_path.annotation == pathlib.Path

    assert sig.return_annotation == pathlib.Path

def test_get_enabled_gems():
    sig = signature(manifest.get_enabled_gems)
    assert len(sig.parameters) >= 1

    cmake_file = list(sig.parameters.values())[0]
    assert cmake_file.name == 'cmake_file'
    assert cmake_file.annotation == pathlib.Path

    assert sig.return_annotation == set

# engine_template interface
def test_create_project():
    sig = signature(engine_template.create_project)
    assert len(sig.parameters) >= 3

    parameters = list(sig.parameters.values())
    project_path = parameters[0]
    assert project_path.name == 'project_path'
    assert project_path.annotation == pathlib.Path
    project_name = parameters[1]
    assert project_name.name == 'project_name'
    assert project_name.annotation == str
    template_path = parameters[2]
    assert template_path.name == 'template_path'
    assert template_path.annotation == pathlib.Path

    assert sig.return_annotation == int

# enable_gem interface
def test_enable_gem():
    sig = signature(enable_gem.enable_gem_in_project)
    assert len(sig.parameters) >= 4

    parameters = list(sig.parameters.values())
    gem_path = parameters[1]
    assert gem_path.name == 'gem_path'
    assert gem_path.annotation == pathlib.Path
    project_path = parameters[3]
    assert project_path.name == 'project_path'
    assert project_path.annotation == pathlib.Path

    assert sig.return_annotation == int

# disable_gem interface
def test_disable_gem():
    sig = signature(disable_gem.disable_gem_in_project)
    assert len(sig.parameters) >= 4

    parameters = list(sig.parameters.values())
    gem_path = parameters[1]
    assert gem_path.name == 'gem_path'
    assert gem_path.annotation == pathlib.Path
    project_path = parameters[3]
    assert project_path.name == 'project_path'
    assert project_path.annotation == pathlib.Path

    assert sig.return_annotation == int

# project_properties interface
def test_edit_project_properties():
    sig = signature(project_properties.edit_project_props)
    assert len(sig.parameters) >= 11

    parameters = list(sig.parameters.values())
    proj_path = parameters[0]
    assert proj_path.name == 'proj_path'
    assert proj_path.annotation == pathlib.Path
    new_name = parameters[2]
    assert new_name.name == 'new_name'
    assert new_name.annotation == str
    new_id = parameters[3]
    assert new_id.name == 'new_id'
    assert new_id.annotation == str
    new_origin = parameters[4]
    assert new_origin.name == 'new_origin'
    assert new_origin.annotation == str
    new_display = parameters[5]
    assert new_display.name == 'new_display'
    assert new_display.annotation == str
    new_summary = parameters[6]
    assert new_summary.name == 'new_summary'
    assert new_summary.annotation == str
    new_icon = parameters[7]
    assert new_icon.name == 'new_icon'
    assert new_icon.annotation == str
    replace_tags = parameters[10]
    assert replace_tags.name == 'replace_tags'
    assert replace_tags.annotation == str or replace_tags.annotation == list

    assert sig.return_annotation == int

# manifest interface
def test_refresh_repo():
    sig = signature(repo.refresh_repo)
    assert len(sig.parameters) >= 3

    parameters = list(sig.parameters.values())

    assert parameters[0].name == 'repo_uri'
    assert parameters[0].annotation == str

    assert parameters[1].name == 'repo_set'
    assert parameters[1].annotation == set

    assert parameters[2].name == 'download_missing_files_only'
    assert parameters[2].annotation == bool

    assert sig.return_annotation == int

def test_refresh_repos():
    sig = signature(repo.refresh_repos)
    assert len(sig.parameters) >= 1
    parameters = list(sig.parameters.values())

    assert parameters[0].name == 'download_missing_files_only'
    assert parameters[0].annotation == bool

    assert sig.return_annotation == int

def test_get_gem_json_data_from_cached_repo():
    sig = signature(repo.get_gem_json_data_from_cached_repo)
    assert len(sig.parameters) >= 1

    repo_uri = list(sig.parameters.values())[0]
    assert repo_uri.name == 'repo_uri'
    assert repo_uri.annotation == str

    assert sig.return_annotation == list 

def test_get_gem_json_paths_from_all_cached_repos():
    sig = signature(repo.get_gem_json_data_from_all_cached_repos)

    assert sig.return_annotation == list

# download interface
def test_download_gem():
    sig = signature(download.download_gem)
    assert len(sig.parameters) >= 5

    parameters = list(sig.parameters.values())
    gem_name = parameters[0]
    assert gem_name.name == 'gem_name'
    assert gem_name.annotation == str
    skip_auto_register = parameters[2]
    assert skip_auto_register.name == 'skip_auto_register'
    assert skip_auto_register.annotation == bool
    force_overwrite = parameters[3]
    assert force_overwrite.name == 'force_overwrite'
    assert force_overwrite.annotation == bool
    download_progress_callback = parameters[4]
    assert download_progress_callback.name == 'download_progress_callback'
    # Cannot check annotation type so just check name

    assert sig.return_annotation == int

def test_is_o3de_gem_update_available():
    sig = signature(download.is_o3de_gem_update_available)
    assert len(sig.parameters) >= 2

    parameters = list(sig.parameters.values())
    gem_name = parameters[0]
    assert gem_name.name == 'gem_name'
    assert gem_name.annotation == str
    last_updated = parameters[1]
    assert last_updated.name == 'local_last_updated'
    assert last_updated.annotation == str

    assert sig.return_annotation == bool
