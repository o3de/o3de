#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
def check_result(result, msg):
    from editor_python_test_tools.utils import Report
    if not result:
        Report.result(msg, False)
        raise Exception(msg + " : FAILED")

def Editor_LayerEntity_Works():
    # Description: 
    # Tests AZ Layer creation, property modification and interaction with entity CRUD operations in the editor

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper    
    import azlmbr.layers as layers
    import azlmbr.editor as editor
    import azlmbr.bus as bus
    import azlmbr.entity as entity
    import azlmbr.math as math

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    def add_component(typename, entityId):
        typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', [typename], entity.EntityType().Game)
        componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)
        return componentOutcome.IsSuccess()

    def change_entity_status(desiredStatusProperty, entityId):
        editor.EditorEntityAPIBus(bus.Event, 'SetStartStatus', entityId, desiredStatusProperty)
        status = editor.EditorEntityInfoRequestBus(bus.Event, 'GetStartStatus', entityId)
        if (status == desiredStatusProperty):
            print('[PASS] successfully changed status: ' + str(desiredStatusProperty))
        check_result(False, '[FAIL] unable to set status: ' + str(desiredStatusProperty))

    # Test creating layer entity
    layerId = layers.EditorLayerComponent_CreateLayerEntityFromName('testLayer')
    if(layerId):
        print('[PASS] layer has been created')
    check_result(False, '[FAIL] layer was not created')

    # Test getting and setting the name property for the layer
    name = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', layerId);
    if(name):
        print('[PASS] Get name request succeeded')
    check_result(False, '[FAIL] Get name request failed')

    editor.EditorEntityAPIBus(bus.Event, 'SetName', layerId, "test_layer")
    name = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', layerId)
    if(name == 'test_layer'):
        print('[PASS] Layer name changed')
    else:
        check_result(False, '[FAIL] Did not complete layer name change')

    # Test Getting and Setting the color for the layer
    color = math.Color_ConstructFromValues(255,255,0,1)
    oldColor = layers.EditorLayerComponentRequestBus(bus.Event, 'GetColorPropertyValue', layerId)
    layers.EditorLayerComponentRequestBus(bus.Event, 'SetLayerColor', layerId, color)
    newColor = layers.EditorLayerComponentRequestBus(bus.Event, 'GetColorPropertyValue', layerId)
    if(oldColor and newColor):
        if (oldColor != newColor):
            print('[PASS] Layer color changed')
        else:
            check_result(False, '[FAIL] Was unable to change layer color property')
    else:
        check_result(False, '[FAIL] Was unable to get color property of layer')


    # Test creating child entity in layer entity
    layerChild = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', layerId)
    if (layerChild):
        print('[PASS] layer child entity has been created')
    else:
        check_result(False, '[FAIL] layer child entity was not created')

    queryParent = editor.EditorEntityInfoRequestBus(bus.Event, 'GetParent', layerChild)
    if(queryParent.ToString() == layerId.ToString()):
        print('[PASS] Query parent returned layer ID')
    else:
        check_result(False, '[FAIL] layer is not parented to entity')

    # Test getting child entity IDs
    childList = editor.EditorEntityInfoRequestBus(bus.Event, 'GetChildren', layerId)
    if(childList):
        print('[PASS] EditorEntityInfoRequestBus GetChildren return list of decendants')
    else:
        check_result(False, '[FAILL] EditorEntityInfoRequestBus GetChildren did not return list of decendants')

    # Test adding component to layer entity
    result = add_component('Comment', layerId)
    if(result == True):
        print('[PASS] comment component has been added')
    else:
        check_result(False, '[FAIL] comment component was not added to layer')

    # Test locking layer entity. Note: layers themselves are not locked or unlocked;
    # setting lock on a layer effectively sets the lock for all child entities
    queryLock = editor.EditorEntityInfoRequestBus(bus.Event, 'IsLocked', layerId)
    check_result(queryLock == False, '[INFO] layer is not locked')

    editor.EditorEntityAPIBus(bus.Event, 'SetLockState', layerId, True)
    queryLock = editor.EditorEntityInfoRequestBus(bus.Event, 'IsLocked', layerId)
    check_result(queryLock, '[PASS] layer has been locked')

    # Test layer visibility. Note: layers themselves are not visible or invisible;
    # setting visibility on a layer effectively sets visibility for all child entities
    queryVisibility = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', layerChild)
    check_result(queryVisibility, '[INFO] layer children are visible')

    layers.EditorLayerComponentRequestBus(azlmbr.bus.Event, 'SetVisibility', layerId, False)
    queryVisibility = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', layerChild)
    if(queryVisibility == False):
        print('[PASS] layer children are hidden')
    else:
        check_result(False, '[FAIL] unable to set layer visibility to hidden')

    # Test layer status property
    # start active
    change_entity_status(azlmbr.globals.property.EditorEntityStartStatus_StartActive, layerId)

    # start inactive
    change_entity_status(azlmbr.globals.property.EditorEntityStartStatus_StartInactive, layerId)

    # editor only
    change_entity_status(azlmbr.globals.property.EditorEntityStartStatus_EditorOnly, layerId)

    # Test deleting layer
    editor.ToolsApplicationRequestBus(bus.Broadcast, 'DeleteEntityAndAllDescendants', layerId)
    entityName = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', layerId)
    if (entityName):
        check_result(False, '[FAIL] Layer entity found after delete request')
    else:
        print('[PASS] Layer entity not found after delete request')

    # all tests worked
    Report.result("Editor_LayerEntity_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_LayerEntity_Works)
