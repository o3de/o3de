"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

rootPrefabLoaded = False

def PrefabNotifications_RootPrefabLoadedNotificationsReceived():

    from pathlib import Path

    import azlmbr.prefab as prefab

    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    # Connects PrefabPublicNotificationBusHandler and add callbacks for 'OnRootPrefabInstanceLoaded'
    def OnRootPrefabInstanceLoaded(parameters):
        global rootPrefabLoaded
        rootPrefabLoaded = True

    handler = prefab.PrefabPublicNotificationBusHandler()
    handler.connect()
    handler.add_callback('OnRootPrefabInstanceLoaded', OnRootPrefabInstanceLoaded)

    prefab_test_utils.open_base_tests_level()

    handler.disconnect()

    assert rootPrefabLoaded, "Notification 'PrefabPublicNotifications::OnRootPrefabInstanceLoaded' is not sent."


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PrefabNotifications_RootPrefabLoadedNotificationsReceived)
