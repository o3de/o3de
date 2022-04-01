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

def Editor_EntityCRUDCommands_Works():
    # Description: 
    # Tests a portion of the Entity CRUD Python API while the Editor is running

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general
    import azlmbr.entity as entity
    import azlmbr.legacy.settings as settings
    from azlmbr.entity import EntityId
    import azlmbr.globals

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    parentEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
    childEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', EntityId())
    levelEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'GetCurrentLevelEntityId') 

    # Test SetParent/GetParent

    queryParent = editor.EditorEntityInfoRequestBus(bus.Event, 'GetParent', childEntityId)
    check_result(queryParent.ToString() == levelEntityId.ToString(), "childEntity is parented to the Level entity")

    editor.EditorEntityAPIBus(bus.Event, 'SetParent', childEntityId, parentEntityId)

    queryParent = editor.EditorEntityInfoRequestBus(bus.Event, 'GetParent', childEntityId)
    check_result(queryParent.ToString() == parentEntityId.ToString(), "childEntity is now parented to the parentEntity")

    # Test SetLockState and IsLocked

    queryLock = editor.EditorEntityInfoRequestBus(bus.Event, 'IsLocked', parentEntityId)
    check_result(queryLock == False, "parentEntityId isn't locked")

    editor.EditorEntityAPIBus(bus.Event, 'SetLockState', parentEntityId, True)

    queryLock = editor.EditorEntityInfoRequestBus(bus.Event, 'IsLocked', parentEntityId)
    check_result(queryLock == True, "parentEntityId is now locked")

    queryLock = editor.EditorEntityInfoRequestBus(bus.Event, 'IsLocked', childEntityId)
    check_result(queryLock, "childEntityId is now locked too")

    # Test SetVisibilityState, IsVisible and IsHidden

    queryVisibility = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', parentEntityId)
    check_result(queryVisibility, "parentEntityId is visible")

    queryHidden = editor.EditorEntityInfoRequestBus(bus.Event, 'IsHidden', parentEntityId)
    check_result(queryHidden == False, "parentEntityId isn't hidden")

    editor.EditorEntityAPIBus(bus.Event, 'SetVisibilityState', parentEntityId, False)

    queryVisibility = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', parentEntityId)
    check_result(queryVisibility == False, "parentEntityId is now hidden")

    queryVisibility = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', childEntityId)
    check_result(queryVisibility == False, "childEntityId is now hidden too")

    # Test EditorOnly

    queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', parentEntityId)
    print("EditorOnly before queryStatus: " + str(queryStatus))

    check_result(not(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_EditorOnly), "parentEntityId isn't set to Editor Only")

    editor.EditorEntityAPIBus(bus.Event, 'SetStartStatus', parentEntityId, azlmbr.globals.property.EditorEntityStartStatus_EditorOnly)

    queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', parentEntityId)
    print("EditorOnly after queryStatus: " + str(queryStatus))

    check_result(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_EditorOnly, "parentEntityId is now set to Editor Only")

    queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', childEntityId)
    check_result(not(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_EditorOnly), "childEntityId does not inherit Editor Only")

    # Test StartInactive

    queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', parentEntityId)
    print("queryStatus: " + str(queryStatus))

    check_result(not(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_StartInactive), "parentEntityId isn't set to Start Inactive")

    editor.EditorEntityAPIBus(bus.Event, 'SetStartStatus', parentEntityId, azlmbr.globals.property.EditorEntityStartStatus_StartInactive)

    queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', parentEntityId)
    check_result(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_StartInactive, "parentEntityId is now set to Start Inactive")

    queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', childEntityId)
    check_result(not(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_StartInactive), "childEntityId does not inherit Start Inactive")

    # Test StartActive

    queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', parentEntityId)
    print("queryStatus: " + str(queryStatus))

    check_result(not(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_StartActive), "parentEntityId isn't set to Start Active")

    editor.EditorEntityAPIBus(bus.Event, 'SetStartStatus', parentEntityId, azlmbr.globals.property.EditorEntityStartStatus_StartActive)

    queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', parentEntityId)
    check_result(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_StartActive, "parentEntityId is now set to Start Active")

    queryStatus = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', childEntityId)
    check_result(queryStatus == azlmbr.globals.property.EditorEntityStartStatus_StartActive, "childEntityId should still be set to Start Active by default")

    # all tests worked
    Report.result("Editor_EntityCRUDCommands_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_EntityCRUDCommands_Works)


