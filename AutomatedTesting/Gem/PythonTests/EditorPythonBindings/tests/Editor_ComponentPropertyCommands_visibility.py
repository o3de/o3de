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

def Editor_ComponentPropertyCommands_visibility():
    # Description: 
    # ComponentPropertyCommands test case visibility

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

    # Find entity
    searchFilter = entity.SearchFilter()
    searchFilter.names = ["Shader Ball"]
    entityId = entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)[0]
    check_result(entityId, "entityId was found")

    # Find Infinite Ocean component
    typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ['Mesh'], entity.EntityType().Game)
    getComponentOutcome =  editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', entityId, typeIdsList[0])
    check_result(getComponentOutcome.IsSuccess(), "Found component")
    componentId = getComponentOutcome.GetValue()

    # Get the PTE from the Mesh Component
    pteObj = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', componentId)
    check_result(pteObj.IsSuccess(), "Created a PropertyTreeEditor for the infiniteOceanId")
    pte = pteObj.GetValue()

    paths = pte.build_paths_list_with_types()
    print(f'Paths = {paths}')

    # test for visibility (default all nodes are exposed)
    check_result(pte.get_value('Controller|Configuration|Mesh Asset').IsSuccess(), "Found property hidden node in path")

    # enable visibility enforcement
    pte.set_visible_enforcement(True)
    paths = pte.build_paths_list_with_types()
    print(f'Paths after = {paths}')
    check_result(pte.get_value('Controller|Configuration|Mesh Asset').IsSuccess() is not True, "Property Controller|Configuration| is now a hidden path")

    # test for visibility (missing some property paths parts now)
    check_result(pte.get_value('Mesh Asset').IsSuccess(), "Property path enforcement of visibility")

    # all tests worked
    Report.result("Editor_ComponentPropertyCommands_visibility ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_ComponentPropertyCommands_visibility)



