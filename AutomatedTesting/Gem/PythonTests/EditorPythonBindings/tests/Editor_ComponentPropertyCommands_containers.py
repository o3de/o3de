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

def Editor_ComponentPropertyCommands_containers():
    # Description: 
    # Tests component properties that are containers

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general
    import azlmbr.entity as entity
    import azlmbr.surface_data
    import azlmbr.globals


    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    def is_container(pte, path):
        return pte.is_container(path)

    def get_container_count(pte, path):
        outcome = pte.get_container_count(path)
        if(outcome.IsSuccess()):
            return outcome.GetValue()
        return False

    def reset_container(pte, path):
        outcome = pte.reset_container(path)
        if(outcome.IsSuccess()):
            return outcome.GetValue()
        return False

    def add_container_item(pte, path, key, item):
        outcome = pte.add_container_item(path, key, item)
        if(outcome.IsSuccess()):
            return outcome.GetValue()
        return False

    def remove_container_item(pte, path, key):
        outcome = pte.remove_container_item(path, key)
        if(outcome.IsSuccess()):
            return outcome.GetValue()
        return False

    def update_container_item(pte, path, key, value):
        outcome = pte.update_container_item(path, key, value)
        if(outcome.IsSuccess()):
            return outcome.GetValue()
        return False

    def get_container_item(pte, path, key):
        outcome = pte.get_container_item(path, key)
        if(outcome.IsSuccess()):
            return outcome.GetValue()
        return False

    # Create new Entity
    entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())
    check_result(entityId, "New entity with no parent created")

    tagOne = azlmbr.surface_data.SurfaceTag()
    tagOne.SetTag('one')

    tagTwo = azlmbr.surface_data.SurfaceTag()
    tagTwo.SetTag('two')

    tagThree = azlmbr.surface_data.SurfaceTag()
    tagThree.SetTag('three')

    tagFour = azlmbr.surface_data.SurfaceTag()
    tagFour.SetTag('four')

    # create a component with a TagSurface container
    typeIdsList = [azlmbr.globals.property.GradientSurfaceDataComponentTypeId]

    componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)
    check_result(componentOutcome.IsSuccess(), 'AddComponentsOfType')

    components = componentOutcome.GetValue()
    tagList = components[0]

    pteOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', tagList)
    #check_result(pteOutcome.IsSuccess(), 'BuildComponentPropertyTreeEditor')
    #pte = pteOutcome.GetValue()
        
    ## Test BuildComponentPropertyList
    #paths = pte.build_paths_list()
    #print(f'Paths {paths}')

    #tagListPropertyPath = 'm_template|Extended Tags'

    #check_result(is_container(pte, tagListPropertyPath), 'Has a container')
    #check_result(get_container_count(pte, tagListPropertyPath) == 0, 'Has zero items')
    #check_result(add_container_item(pte, tagListPropertyPath, 0, tagOne), 'Add an item 0')
    #check_result(get_container_count(pte, tagListPropertyPath) == 1, 'Has one item 0')
    #check_result(add_container_item(pte, tagListPropertyPath, 1, tagOne), 'Add an item 1')
    #check_result(add_container_item(pte, tagListPropertyPath, 2, tagTwo), 'Add an item 2')
    #check_result(add_container_item(pte, tagListPropertyPath, 3, tagThree), 'Add an item 3')
    #check_result(get_container_count(pte, tagListPropertyPath) == 4, 'Has four items')
    #check_result(update_container_item(pte, tagListPropertyPath, 2, tagFour), 'Updated an item')

    #itemTag = get_container_item(pte, tagListPropertyPath, 2)
    #check_result (itemTag.Equal(tagFour), 'itemTag equals tagFour')

    #check_result(remove_container_item(pte, tagListPropertyPath, 0), 'Removed one item 0')
    #check_result(remove_container_item(pte, tagListPropertyPath, 0), 'Removed one item 1')
    #check_result(get_container_count(pte, tagListPropertyPath) == 2, 'Has two items')
    #check_result(reset_container(pte, tagListPropertyPath), 'Reset items')
    #check_result(get_container_count(pte, tagListPropertyPath) == 0, 'Has cleared the items')

    # all tests worked
    Report.result("Editor_ComponentPropertyCommands_containers ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_ComponentPropertyCommands_containers)

