# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
import bpy
import json
from . import constants
from pathlib import Path
import addon_utils

def look_at_engine_manifest():
    """!
    This function will look at your O3DE engine manifest JSON
    """
    engine_is_installed = None
    try:
        with open(constants.DEFAULT_SDK_MANIFEST_PATH, "r") as data_file:
            data = json.load(data_file)
            # Look at manifest projects
            o3de_projects = data['projects']
            # Send if o3de is not installed
            engine_is_installed = True
            o3de_projects = extend_project_list(o3de_projects)
            return o3de_projects, engine_is_installed
    except:
        o3de_projects = ['']
        engine_is_installed = False
        return o3de_projects, engine_is_installed

def load_saved_projects():
    """!
    This function will load the users project.json list
    """
    # Find the installed modules path on users system
    for addon_path in addon_utils.modules():
        if addon_path.bl_info['name'] == 'O3DE_DCCSI_BLENDER_SCENE_EXPORTER':
            addon_full_path = addon_path.__file__
            # Lets just get the directory path
            bpy.types.Scene.plugin_directory = Path(addon_full_path).parent
    project_json_path = Path(bpy.types.Scene.plugin_directory, "projects.json")
    if project_json_path.exists():
        with open(Path(bpy.types.Scene.plugin_directory, "projects.json"), "r") as data_file:
            data = json.load(data_file)
            return data

def extend_project_list(o3de_projects):
    """!
    This function will load the users project.json list and extend it to the engine manifest projects
    """
    saved_o3de_projects = load_saved_projects()
    # Check to see if these projects are already on list
    if saved_o3de_projects not in o3de_projects:
        o3de_projects.extend(saved_o3de_projects)
    return o3de_projects

def build_projects_list():
    """!
    This function will check to see if O3DE is installed, looks at the o3de engine manifest projects
    and the user saved project.json, builds the o3de project list
    """
    o3de_projects, engine_is_installed = look_at_engine_manifest()
    # Project list
    list_o3de_projects = []
    # Check to see if O3DE is installed.
    if engine_is_installed:
        # Make a list of projects to select from
        for project_full_path in o3de_projects:
            # Check to see if the project name might be 1 level up in this path if ending with project.
            # For example we could have a path like this C:/Users/USERNAME/O3DE/Projects/loft-arch-vis-sample/Project
            if Path(project_full_path).name == 'Project':
                append_project_path = Path(project_full_path)
                list_o3de_projects.append((project_full_path, append_project_path.parts[-2], project_full_path))
            else:
                list_o3de_projects.append((project_full_path, Path(project_full_path).name, project_full_path))
    return list_o3de_projects

def save_project_list_json(append_path):
    """!
    This function will save the users projects Projects.json
    """
    # Load the current list to append to:
    saved_o3de_projects = load_saved_projects()
    # Check to see if the path not already in the list:
    if append_path not in saved_o3de_projects:
        saved_o3de_projects.append(append_path)
        # Save Updated
        with open(Path(bpy.types.Scene.plugin_directory, "projects.json"), 'w') as f:
            json.dump(saved_o3de_projects, f)