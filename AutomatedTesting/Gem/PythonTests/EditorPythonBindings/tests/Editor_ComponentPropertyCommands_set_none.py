"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_ComponentPropertyCommands_set_none(BaseClass):
    # Description: 
    # Tests setting property values that are asset values to None
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.legacy.general
        import azlmbr.entity as entity

        # toggle the entity system to use visibility enforcement rules
        editor.EditorComponentAPIBus(bus.Broadcast, 'SetVisibleEnforcement', True)

        def get_entity_by_name(name):
            searchFilter = entity.SearchFilter()
            searchFilter.names = [name]
            searchResult = entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)
            BaseClass.check_result(len(searchResult) > 0, "get_entity_by_name - {}".format(name))
            return searchResult[0]

        def get_component_type_by_name(name):
            gameType = entity.EntityType().Game
            listOfTypeIds = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', [name], gameType)
            BaseClass.check_result(len(listOfTypeIds) > 0, "get_component_type_by_name - {}".format(name))
            return listOfTypeIds[0]

        def get_component_of_type(entity, componentTypeId):
            outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', entity, componentTypeId)
            BaseClass.check_result(outcome.IsSuccess(), "get_component_of_type - {} for type {}".format(entity, componentTypeId))
            return outcome.GetValue()

        def get_component_property(component, path):
            outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', component, path)
            BaseClass.check_result(outcome.IsSuccess(), "get_component_property - {}".format(path))
            return outcome.GetValue()

        def set_component_property(component, path, value):
            outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'SetComponentProperty', component, path, value)
            BaseClass.check_result(outcome.IsSuccess(), "set_component_property - {} with value {}".format(path, value))
            return True

        # get the shader ball and mesh component then assign it to None
        entityId = get_entity_by_name('Shader Ball')
        componentTypeId = get_component_type_by_name('Mesh')
        componentId = get_component_of_type(entityId, componentTypeId)

        # this series tests that a material can be fetched, mutated with a None, and reverts to an 'empty asset'
        propertPath = 'Mesh Asset'
        property = get_component_property(componentId, propertPath)
        BaseClass.check_result(property.is_valid(), "propertPath current is valid - {}".format(propertPath))

        updateResult = set_component_property(componentId, propertPath, None)
        BaseClass.check_result(updateResult, "componentId set to None")

        camera = get_component_property(componentId, propertPath)
        BaseClass.check_result(camera.is_valid() is False, "confirmed, componentId has been set to None")

if __name__ == "__main__":
    tester = Editor_ComponentPropertyCommands_set_none()
    tester.test_case(tester.test, level="TestDependenciesLevel")
