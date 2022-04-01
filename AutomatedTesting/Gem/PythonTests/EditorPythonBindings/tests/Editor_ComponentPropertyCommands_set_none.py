"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def ReportError(msg):
    from editor_python_test_tools.utils import Report
    Report.result(msg, False)
    raise Exception(msg + " : FAILED")

def Editor_ComponentPropertyCommands_set_none():
    # Description: 
    # Tests setting property values that are asset values to None

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general
    import azlmbr.entity as entity

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="TestDependenciesLevel")
    azlmbr.legacy.general.idle_wait_frames(1)

    # toggle the entity system to use visibility enforcement rules
    editor.EditorComponentAPIBus(bus.Broadcast, 'SetVisibleEnforcement', True)

    def print_result(message, result):
        if result:
            print(message + ": SUCCESS")
        else:
            ReportError(message + ": FAILURE")

    def get_entity_by_name(name):
        searchFilter = entity.SearchFilter()
        searchFilter.names = [name]
        searchResult = entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)
        if(len(searchResult) > 0):
            return searchResult[0]
        print_result("get_entity_by_name - {}".format(name), False)

    def get_component_type_by_name(name):
        gameType = entity.EntityType().Game
        listOfTypeIds = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', [name], gameType)
        if(len(listOfTypeIds) > 0):
            return listOfTypeIds[0]
        print_result("get_component_type_by_name - {}".format(name), False)

    def get_component_of_type(entity, componentTypeId):
        outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', entity, componentTypeId)
        if(outcome.IsSuccess()):
            return outcome.GetValue()
        print_result("get_component_of_type - {} for type {}".format(entity, componentTypeId), False)

    def get_component_property(component, path):
        outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)
        if(outcome.IsSuccess()):
            return outcome.GetValue()
        print_result("get_component_property - {}".format(path), False)

    def set_component_property(component, path, value):
        outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'SetComponentProperty', component, path, value)
        if(outcome.IsSuccess()):
            return True
        print_result("set_component_property - {} with value {}".format(path, value), False)

    # get the shader ball and mesh component then assign it to None
    entityId = get_entity_by_name('Shader Ball')
    componentTypeId = get_component_type_by_name('Mesh')
    componentId = get_component_of_type(entityId, componentTypeId)

    # this series tests that a material can be fetched, mutated with a None, and reverts to an 'empty asset'
    propertPath = 'Mesh Asset'
    property = get_component_property(componentId, propertPath)
    print_result("propertPath current is valid - {}".format(propertPath), property.is_valid())

    updateResult = set_component_property(componentId, propertPath, None)
    print_result("componentId set to None", updateResult)

    camera = get_component_property(componentId, propertPath)
    print_result("confirmed, componentId has been set to None", camera.is_valid() is False)

    # all tests worked
    Report.result("Editor_ComponentPropertyCommands_set_none ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_ComponentPropertyCommands_set_none)
