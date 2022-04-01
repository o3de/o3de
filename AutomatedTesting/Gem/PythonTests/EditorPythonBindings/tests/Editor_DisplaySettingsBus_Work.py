"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def check_result(result, msg):
    from editor_python_test_tools.utils import Report
    if not result:
        Report.result(msg, False)
        raise Exception(msg + " : FAILED")

def Editor_DisplaySettingsBus_Work():
    # Description: 
    # Tests the Python API from DisplaySettingsPythonFuncs.cpp while the Editor is running

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general
    import azlmbr.entity as entity
    import azlmbr.display_settings as display_settings

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="TestDependenciesLevel")
    azlmbr.legacy.general.idle_wait_frames(1)

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
    if alteredState.no_collision == newState.no_collision and \
            alteredState.no_labels == newState.no_labels and \
            alteredState.simulate == newState.simulate and \
            alteredState.hide_tracks == newState.hide_tracks and \
            alteredState.hide_links == newState.hide_links and \
            alteredState.hide_helpers == newState.hide_helpers and \
            alteredState.show_dimension_figures == newState.show_dimension_figures:
        print("display settings were changed correctly")

    # Restore previous settings
    display_settings.DisplaySettingsBus(bus.Broadcast, 'SetSettingsState', existingState)

    # all tests worked
    Report.result("Editor_DisplaySettingsBus_Work ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_DisplaySettingsBus_Work)

