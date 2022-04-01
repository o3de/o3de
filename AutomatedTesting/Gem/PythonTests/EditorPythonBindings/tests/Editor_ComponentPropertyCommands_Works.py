"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def ReportError(msg):
    from editor_python_test_tools.utils import Report
    Report.result(msg, False)
    raise Exception(msg + " : FAILED")

def GetSetCompareTest(component, path, value):
    import azlmbr.bus as bus
    import azlmbr.editor as editor

    oldObj = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)

    if(oldObj.IsSuccess()):
        oldValue = oldObj.GetValue()
    else:
        ReportError(f'oldObj can not be found at {path}')

    oldValueCompared = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, oldValue)

    editor.EditorComponentAPIBus(bus.Broadcast, 'SetComponentProperty', component, path, value)

    newObj = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)

    if(newObj.IsSuccess()):
        newValue = newObj.GetValue()

    newValueCompared = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, newValue)

    isOldNewValueSame = editor.EditorComponentAPIBus(bus.Broadcast, 'CompareComponentProperty', component, path, oldValue)

    if not(newValue == oldValue and oldValueCompared and newValueCompared and not isOldNewValueSame):
        print("GetSetCompareTest " + path + ": SUCCESS")
    else:
        ReportError("GetSetCompareTest " + path)

def PteTest(pte, path, value):
    oldObj = pte.get_value(path)

    if(oldObj.IsSuccess()):
        oldValue = oldObj.GetValue()
    oldValueCompared = pte.compare_value(path, oldValue)

    pte.set_value(path, value)

    newObj = pte.get_value(path)

    if(newObj.IsSuccess()):
        newValue = newObj.GetValue()

    newValueCompared = pte.compare_value(path, newValue)

    isOldNewValueSame = pte.compare_value(path, oldValue)

    if not(newValue == oldValue and oldValueCompared and newValueCompared and not isOldNewValueSame):
        print("PteTest " + path + ": SUCCESS")
    else:
        ReportError("PteTest " + path)

def Editor_ComponentPropertyCommands_Works():
    # Description:
    # Tests a portion of the Component Property Get/Set Python API while the Editor is running

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.legacy.general
    import azlmbr.prefab
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.entity as entity
    import azlmbr.math as math

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    # Create new Entity
    entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())

    if (not entityId):
        ReportError("New entity with no parent created")

    # Get Component Type for Quad Shape
    typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Quad Shape"], entity.EntityType().Game)

    componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)

    if (not componentOutcome.IsSuccess()):
        ReportError(f"Quad Shape component {typeIdsList} added to entity")

    components = componentOutcome.GetValue()
    component = components[0]

    hasComponent = editor.EditorComponentAPIBus(bus.Broadcast, 'HasComponentOfType', entityId, typeIdsList[0])

    if(not hasComponent):
        ReportError("Entity has an Quad Shape component")

    # Test BuildComponentPropertyList
    paths = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyList', component)
    
    if(len(paths) < 1):
        ReportError("BuildComponentPropertyList")

    # Tests for GetComponentProperty/SetComponentProperty
    path_color = 'Shape Color'
    path_visible = 'Visible'
    path_quad_width = 'Quad Shape|Quad Configuration|Width'

    GetSetCompareTest(component, path_visible, False)
    GetSetCompareTest(component, path_quad_width, 42.0)

    color = math.Color()
    color.r = 0.4
    color.g = 0.5
    color.b = 0.6

    GetSetCompareTest(component, path_color, color)

    # Tests for BuildComponentPropertyTreeEditor
    pteObj = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', component)

    if(not pteObj.IsSuccess()):
        ReportError("BuildComponentPropertyTreeEditor")
     
    pte = pteObj.GetValue()

    PteTest(pte, path_visible, True)
    PteTest(pte, path_quad_width, 48.0)

    color = math.Color()
    color.r = 0.9
    color.g = 0.1
    color.b = 0.3

    PteTest(pte, path_color, color)

    # all tests worked
    Report.result("Editor_ComponentPropertyCommands_Works succeeded.", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_ComponentPropertyCommands_Works)


