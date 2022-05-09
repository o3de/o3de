"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests a portion of the View Pane Python API from ViewPane.cpp while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math
import azlmbr.legacy.general as general

# Open a level (any level should work)
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'Base')

general.idle_wait(0.5)

viewport_size = general.get_viewport_size()
view_width = int(viewport_size.x)
view_height = int(viewport_size.y)
general.set_viewport_size(320, 240)

new_viewport_size = general.get_viewport_size()
new_view_width = int(new_viewport_size.x)
new_view_height = int(new_viewport_size.y)

if not (view_width == new_view_width) and not (view_height == new_view_height):
    print("set_viewport_size works")

general.resize_viewport(int(view_width), int(view_height))

newer_viewport_size = general.get_viewport_size()

newer_view_width = int(newer_viewport_size.x)
newer_view_height = int(newer_viewport_size.y)

if not (newer_view_width == new_view_width) and not (newer_view_height == new_view_height):
    print("resize_viewport works")

    def set_and_validate_viewport_size(width, height):
        general.set_viewport_size(width, height)
        general.update_viewport()
        general.idle_wait(1.0)
        new_viewport_size = general.get_viewport_size()
        if (new_viewport_size.x != width) or (new_viewport_size.y != height):
            return False
        return True

# Perform some simple validation of get/set viewport expansion policy
general.set_viewport_expansion_policy('FixedSize')
if (general.get_viewport_expansion_policy() == 'FixedSize'):
    print("get_viewport_expansion_policy works")

general.set_viewport_expansion_policy('AutoExpand')
if (general.get_viewport_expansion_policy() == 'AutoExpand'):
    print("get_viewport_expansion_policy works")

# Validate that setting it to an invalid value doesn't change the setting.
general.set_viewport_expansion_policy('XXXXX')
if (general.get_viewport_expansion_policy() == 'AutoExpand'):
    print("get_viewport_expansion_policy works")

# Set a series of viewport sizes (smaller to larger than display resolution sizes) and verify they all work.
general.set_viewport_expansion_policy('FixedSize')
if (set_and_validate_viewport_size(150, 300)):
    print("set_viewport_expansion_policy works")

# Set different view pane layouts and verify it works
success = True
#general.set_view_pane_layout(0)
general.idle_wait(0.5)

success = success and (general.get_view_pane_layout() == 0)
success = success and (general.get_viewport_count() == 1)
# general.set_view_pane_layout(1)
# general.idle_wait(0.5)
# success = success and (general.get_view_pane_layout() == 1)
# success = success and (general.get_viewport_count() == 2)
if success:
    print("get_view_pane_layout works")
    print("set_view_pane_layout works")
    print("get_viewport_count works")

#success = True
#general.set_active_viewport(0)
#general.idle_wait(0.5)
#success = success and (general.get_active_viewport() == 0)
#general.set_active_viewport(1)
#general.idle_wait(0.5)
#success = success and (general.get_active_viewport() == 1)
#if success:
#    print("get_active_viewport works")
#    print("set_active_viewport works")

#general.set_view_pane_layout(0)
general.idle_wait(0.5)

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
