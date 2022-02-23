# coding:utf-8
#!/usr/bin/python
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------
import json
from pathlib import Path
from . import constants

def LookatEngineManifest():
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
            return o3de_projects, engine_is_installed
    except:
        o3de_projects = ['']
        engine_is_installed = False
        return o3de_projects, engine_is_installed

def build_projects_list():
    """!
    This function will build the o3de project list
    """
    o3de_projects, engine_is_installed = LookatEngineManifest()

    list_o3de_projects = []
    
    if engine_is_installed:
        # Make a list of projects to select from
        for project_full_path in o3de_projects:
            list_o3de_projects.append((project_full_path, Path(project_full_path).name, project_full_path))
    return list_o3de_projects