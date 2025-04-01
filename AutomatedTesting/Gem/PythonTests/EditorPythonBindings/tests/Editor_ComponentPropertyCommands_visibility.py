"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_ComponentPropertyCommands_visibility(BaseClass):
    # Description: 
    # ComponentPropertyCommands test case visibility 
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.legacy.general
        import azlmbr.entity as entity
        check_result = BaseClass.check_result

        # Find entity
        searchFilter = entity.SearchFilter()
        searchFilter.names = ["Shader Ball"]
        entityId = entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)[0]
        check_result(entityId, "entityId was found")

        # Find Mesh component
        typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ['Mesh'], entity.EntityType().Game)
        getComponentOutcome =  editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', entityId, typeIdsList[0])
        check_result(getComponentOutcome.IsSuccess(), "Found component")
        componentId = getComponentOutcome.GetValue()

        # Get the PTE from the Mesh Component
        pteObj = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', componentId)
        check_result(pteObj.IsSuccess(), "Created a PropertyTreeEditor for the Mesh component")
        pte = pteObj.GetValue()

        paths = pte.build_paths_list_with_types()

        # test for visibility (default all nodes are exposed)
        check_result(pte.get_value('Controller|Configuration|Model Asset').IsSuccess(), "Found property hidden node in path")

        # enable visibility enforcement
        pte.set_visible_enforcement(True)
        paths = pte.build_paths_list_with_types()
        check_result(pte.get_value('Controller|Configuration|Model Asset').IsSuccess() is not True, "Property Controller|Configuration| is now a hidden path")

        # test for visibility (missing some property paths parts now)
        check_result(pte.get_value('Model Asset').IsSuccess(), "Property path enforcement of visibility")

if __name__ == "__main__":
    tester = Editor_ComponentPropertyCommands_visibility()
    tester.test_case(tester.test, level="TestDependenciesLevel")
