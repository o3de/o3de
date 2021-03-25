"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# Tests the Python API from MainWindow.cpp while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.legacy.general as general

# Get all pane names
panes = general.get_pane_class_names()

if(len(panes) > 0):
    print('get_pane_class_names worked')

# Get any element from the panes list
test_pane = panes[4]

general.open_pane(test_pane)

if (general.is_pane_visible(test_pane)) :
    print('open_pane worked')

general.close_pane(test_pane)

if not (general.is_pane_visible(test_pane)) :
    print('close_pane worked')

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
