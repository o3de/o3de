# coding:utf-8
#!/usr/bin/python
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------
import os
import json

# Const
USER_NAME = os.getenv('username')
DEFAULT_SDK_MANIFEST_PATH = os.path.join('C:\\', 'Users', '{}', '.o3de', 'o3de_manifest.json').format(USER_NAME)

def LookatEngineManifest():
    """
    This function will look at your O3DE engine manifest JSON
    """
    engineIsInstalled = None
    try:
        with open(DEFAULT_SDK_MANIFEST_PATH, "r") as data_file:
            data = json.load(data_file)
            # Look at manifest defualt project
            o3deDefaultProjectsFolder = data['default_projects_folder']
            # Look at manifest projects
            o3deProjects = data['projects']
            # Send if o3de is not installed
            engineIsInstalled = True
            return o3deDefaultProjectsFolder, o3deProjects, engineIsInstalled
    except:
        o3deDefaultProjectsFolder = ''
        o3deProjects = ['']
        engineIsInstalled = False
        return o3deDefaultProjectsFolder, o3deProjects, engineIsInstalled

def BuildProjectsList():
    """
    This function will build the o3de project list
    """
    o3deDefaultProjectsFolder, o3deProjects, engineIsInstalled = LookatEngineManifest()

    listOfo3deProjects = []
    
    if engineIsInstalled:
        # Make a list of projects to select from
        for projectFullPath in o3deProjects:
            listOfo3deProjects.append((projectFullPath, os.path.basename(projectFullPath), projectFullPath))
    return listOfo3deProjects