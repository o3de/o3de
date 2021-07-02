"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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
