"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

propagationBegun = False
propagationEnded = False

def PrefabNotifications_PropagationNotificationsReceived():

    from pathlib import Path

    import azlmbr.prefab as prefab

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.prefab_utils import Prefab
    import Prefab.tests.PrefabTestUtils as prefab_test_utils

    CAR_PREFAB_FILE_NAME = Path(__file__).stem + 'car_prefab'

    prefab_test_utils.open_base_tests_level()

    # Creates a new entity at the root level
    car_entity = EditorEntity.create_editor_entity("Car")
    car_prefab_entities = [car_entity]

    # Creates a prefab from the new entity
    _, car = Prefab.create_prefab(
        car_prefab_entities, CAR_PREFAB_FILE_NAME)

    # Connects PrefabPublicNotificationBusHandler and add callbacks for 'OnPrefabInstancePropagationBegin'
    # and 'OnPrefabInstancePropagationEnd'
    def OnPrefabInstancePropagationBegin(parameters):
        global propagationBegun
        propagationBegun = True

    def OnPrefabInstancePropagationEnd(parameters):
        global propagationEnded
        propagationEnded = True

    handler = prefab.PrefabPublicNotificationBusHandler()
    handler.connect()
    handler.add_callback('OnPrefabInstancePropagationBegin', OnPrefabInstancePropagationBegin)
    handler.add_callback('OnPrefabInstancePropagationEnd', OnPrefabInstancePropagationEnd)

    # Duplicates the prefab instance to trigger callbacks
    Prefab.duplicate_prefabs([car])

    handler.disconnect()

    assert propagationBegun, "Notification 'PrefabPublicNotifications::OnPrefabInstancePropagationBegin' is not sent."
    assert propagationEnded, "Notification 'PrefabPublicNotifications::OnPrefabInstancePropagationEnd' is not sent."


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(PrefabNotifications_PropagationNotificationsReceived)
