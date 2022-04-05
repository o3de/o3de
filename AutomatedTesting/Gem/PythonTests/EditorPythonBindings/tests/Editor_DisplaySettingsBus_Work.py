"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_DisplaySettingsBus_Work(BaseClass):
    # Description: 
    # Tests the Python API from DisplaySettingsPythonFuncs.cpp while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.display_settings as display_settings
        check_result = BaseClass.check_result

        # Retrieve current settings
        existingState = display_settings.DisplaySettingsBus(bus.Broadcast, 'GetSettingsState')

        # Alter settings
        alteredState = azlmbr.object.create('DisplaySettingsState')
        alteredState.no_collision = False
        alteredState.no_labels = False
        alteredState.simulate = True
        alteredState.hide_tracks = False
        alteredState.hide_links = False
        alteredState.hide_helpers = False
        alteredState.show_dimension_figures = True

        # Set altered settings
        display_settings.DisplaySettingsBus(bus.Broadcast, 'SetSettingsState', alteredState)

        # Get settings again
        newState = display_settings.DisplaySettingsBus(bus.Broadcast, 'GetSettingsState')

        # Check if the setter worked
        check_result(alteredState.no_collision == newState.no_collision, 'alteredState.no_collision')
        check_result(alteredState.no_labels == newState.no_labels, 'alteredState.no_labels')
        check_result(alteredState.simulate == newState.simulate, 'alteredState.simulate')
        check_result(alteredState.hide_tracks == newState.hide_tracks, 'alteredState.hide_tracks')
        check_result(alteredState.hide_links == newState.hide_links, 'alteredState.hide_links')
        check_result(alteredState.hide_helpers == newState.hide_helpers, 'alteredState.hide_helpers')
        check_result(alteredState.show_dimension_figures == newState.show_dimension_figures, 'alteredState.show_dimension_figures')

        # Restore previous settings
        display_settings.DisplaySettingsBus(bus.Broadcast, 'SetSettingsState', existingState)

if __name__ == "__main__":
    tester = Editor_DisplaySettingsBus_Work()
    tester.test_case(tester.test, level="TestDependenciesLevel")

