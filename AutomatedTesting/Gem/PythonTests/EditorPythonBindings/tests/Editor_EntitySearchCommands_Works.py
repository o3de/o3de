"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_EntitySearchCommands_Works(BaseClass):
    # Description: 
    # Tests the Entity Search Python API while the Editor is running
    
    @staticmethod
    def test():
        import azlmbr.editor as editor
        import azlmbr.entity as entity
        import azlmbr.bus as bus
        import azlmbr.math as math
        import azlmbr.components as components
        check_result = BaseClass.check_result

        # Save Root Entities and Total Entity number
        root = entity.SearchBus(bus.Broadcast, 'GetRootEditorEntities')
        rootNum = len(root)

        all = entity.SearchBus(bus.Broadcast, 'SearchEntities', entity.SearchFilter())
        allNum = len(all)

        # Create Hierarchy
        #
        #   City
        #   |_  Street              (2)
        #       |_  Car
        #       |   |_ Passenger    (1, 2)
        #       |   |_ Passenger
        #       |_  Car             (1)
        #       |   |_ Passenger
        #       |_  SportsCar
        #           |_ Passenger    (2)
        #           |_ Passenger
        #

        def CreateEntity(name, parentId):
            newEntityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', parentId)
            editor.EditorEntityAPIBus(bus.Event, 'SetName', newEntityId, name)
            testName = editor.EditorEntityInfoRequestBus(bus.Event, 'GetName', newEntityId)
            check_result(testName == name, f"testName {testName} does not equal set name {name}")
            return newEntityId

        # Entities
        cityId = CreateEntity('City', entity.EntityId())
        streetId = CreateEntity('Street', cityId)
        carId1 = CreateEntity('Car', streetId)
        passengerId1 = CreateEntity('Passenger', carId1)
        passengerId2 = CreateEntity('Passenger', carId1)
        carId2 = CreateEntity('Car', streetId)
        passengerId3 = CreateEntity('Passenger', carId2)
        sportsCarId = CreateEntity('SportsCar', streetId)
        passengerId4 = CreateEntity('Passenger', sportsCarId)
        passengerId5 = CreateEntity('Passenger', sportsCarId)

        # Components
        typeNameList = ["Comment", "Actor"]
        typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', typeNameList, entity.EntityType().Game)

        editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', passengerId1, [typeIdsList[0]])
        editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', carId2, [typeIdsList[0]])
        editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', streetId, [typeIdsList[1]])
        editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', passengerId1, [typeIdsList[1]])
        editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', passengerId4, [typeIdsList[1]])

        # Test the Entity Search API
        def CompareEntityIds(id1, id2):
            return (id1.ToString() == id2.ToString())

        def SearchResultCheck(searchFilter, testName, resultId):
            entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)
            check_result((len(entities) == 1) and (CompareEntityIds(entities[0], resultId)), "SearchResultCheck: " + testName)

        def SearchResultsCheck(searchFilter, testName, resultSize):
            entities = entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)
            check_result(len(entities) == resultSize, testName + " (size is " +  str(len(entities)) + ", should be " + str(resultSize) + ")")

        # Search by Name - Base

        # No filters - return all entities
        searchFilter1 = entity.SearchFilter()
        SearchResultsCheck(searchFilter1, "No filters - return all entities", (10 + allNum))

        # Filter by name - single entity
        searchFilter2 = entity.SearchFilter()
        searchFilter2.names = ["Street"]
        SearchResultCheck(searchFilter2, "Filter by name - single entity", streetId)

        # Filter by name - multiple entities
        searchFilter3 = entity.SearchFilter()
        searchFilter3.names = ["Passenger"]
        SearchResultsCheck(searchFilter3, "Filter by name - multiple entities", 5)

        # Filter by name - multiple names
        searchFilter4 = entity.SearchFilter()
        searchFilter4.names = ["Passenger", "Street"]
        SearchResultsCheck(searchFilter4, "Filter by name - multiple names", 6)

        # Search by Name - Wildcard

        # Filter by name - wildcard 01
        searchFilter5 = entity.SearchFilter()
        searchFilter5.names = ["Str*et"]
        SearchResultCheck(searchFilter5, "Filter by name - wildcard 01", streetId)

        # Filter by name - wildcard 02
        searchFilter6 = entity.SearchFilter()
        searchFilter6.names = ["St*et"]
        SearchResultCheck(searchFilter6, "Filter by name - wildcard 02", streetId)

        # Filter by name - wildcard 03
        searchFilter7 = entity.SearchFilter()
        searchFilter7.names = ["Str?et"]
        SearchResultCheck(searchFilter7, "Filter by name - wildcard 03", streetId)

        # Filter by name - wildcard 04
        searchFilter8 = entity.SearchFilter()
        searchFilter8.names = ["Str?t"]
        SearchResultsCheck(searchFilter8, "Filter by name - wildcard 04", 0)

        # Filter by name - wildcard 05
        searchFilter10 = entity.SearchFilter()
        searchFilter10.names = ["*"]
        SearchResultsCheck(searchFilter10, "Filter by name - wildcard 05", (10 + allNum))

        # Search by Name - Case Sensitive

        # Filter by name - case sensitive 01
        searchFilter11 = entity.SearchFilter()
        searchFilter11.names = ["Street"]
        searchFilter11.names_case_sensitive = False  # default
        SearchResultCheck(searchFilter11, "Filter by name - case sensitive 01", streetId)

        # Filter by name - case sensitive 02
        searchFilter12 = entity.SearchFilter()
        searchFilter12.names = ["street"]
        searchFilter12.names_case_sensitive = False
        SearchResultCheck(searchFilter12, "Filter by name - case sensitive 02", streetId)

        # Filter by name - case sensitive 03
        searchFilter13 = entity.SearchFilter()
        searchFilter13.names = ["Street"]
        searchFilter13.names_case_sensitive = True
        SearchResultCheck(searchFilter13, "Filter by name - case sensitive 03", streetId)

        # Filter by name - case sensitive 04
        searchFilter14 = entity.SearchFilter()
        searchFilter14.names = ["street"]
        searchFilter14.names_case_sensitive = True  # default
        SearchResultsCheck(searchFilter14, "Filter by name - case sensitive 04", 0)

        # Search by Path - Base

        # Filter by path - base 01
        searchFilter15 = entity.SearchFilter()
        searchFilter15.names = ["City|Street|SportsCar"]
        SearchResultCheck(searchFilter15, "Filter by path - base 01", sportsCarId)

        # Filter by path - base 02
        searchFilter16 = entity.SearchFilter()
        searchFilter16.names = ["City|Street|Car|Passenger"]
        SearchResultsCheck(searchFilter16, "Filter by path - base 02", 3)

        # Search by Path - Wildcard

        # Filter by path - wildcard 01
        searchFilter17 = entity.SearchFilter()
        searchFilter17.names = ["City|*|SportsCar"]
        SearchResultCheck(searchFilter17, "Filter by path - wildcard 01", sportsCarId)

        # Filter by path - wildcard 02
        searchFilter18 = entity.SearchFilter()
        searchFilter18.names = ["City|Street|*|Passenger"]
        SearchResultsCheck(searchFilter18, "Filter by path - wildcard 02", 5)

        # Filter by path - wildcard 03
        searchFilter19 = entity.SearchFilter()
        searchFilter19.names = ["City|Street|*Car|Passenger"]
        SearchResultsCheck(searchFilter19, "Filter by path - wildcard 03", 5)

        # Filter by path - wildcard 04
        searchFilter20 = entity.SearchFilter()
        searchFilter20.names = ["City|Street|Sport*|Passenger"]
        SearchResultsCheck(searchFilter20, "Filter by path - wildcard 04", 2)

        # Search by Path - Case Sensitive

        # Filter by path - case sensitive 01
        searchFilter21 = entity.SearchFilter()
        searchFilter21.names = ["City|Street"]
        searchFilter21.names_case_sensitive = False  # default
        SearchResultCheck(searchFilter21, "Filter by path - case sensitive 01", streetId)

        # Filter by path - case sensitive 02
        searchFilter22 = entity.SearchFilter()
        searchFilter22.names = ["city|street"]
        searchFilter22.names_case_sensitive = False  # default
        SearchResultCheck(searchFilter22, "Filter by path - case sensitive 02", streetId)

        # Filter by path - case sensitive 03
        searchFilter23 = entity.SearchFilter()
        searchFilter23.names = ["City|Street"]
        searchFilter23.names_case_sensitive = True
        SearchResultCheck(searchFilter23, "Filter by path - case sensitive 03", streetId)

        # Filter by path - case sensitive 04
        searchFilter24 = entity.SearchFilter()
        searchFilter24.names = ["city|street"]
        searchFilter24.names_case_sensitive = True
        SearchResultsCheck(searchFilter24, "Filter by path - case sensitive 04", 0)

        # Search by Component - Base

        # Filter by component - base 01
        searchFilter25 = entity.SearchFilter()
        searchFilter25.components = { typeIdsList[0]:{} }
        SearchResultsCheck(searchFilter25, "Filter by component - base 01", 2)

        # Filter by component - base 02
        searchFilter26 = entity.SearchFilter()
        searchFilter26.components = { typeIdsList[1]:{} }
        SearchResultsCheck(searchFilter26, "Filter by component - base 02", 3)

        # Search by Component - Multiple

        # Filter by component - multiple 01
        searchFilter27 = entity.SearchFilter()
        searchFilter27.components = { typeIdsList[0]: {} , typeIdsList[1]: {} }
        searchFilter27.components_match_all = False  # default
        SearchResultsCheck(searchFilter27, "Filter by component - multiple 01", 4)

        # Filter by component - multiple 02
        searchFilter28 = entity.SearchFilter()
        searchFilter28.components = { typeIdsList[0]: {} , typeIdsList[1]: {} }
        searchFilter28.components_match_all = True
        SearchResultCheck(searchFilter28, "Filter by component - multiple 02", passengerId1)

        # Search with Roots - Base

        # Filter with roots - base 01
        searchFilter29 = entity.SearchFilter()
        searchFilter29.names = ["Passenger"]
        searchFilter29.roots = [carId1]
        SearchResultsCheck(searchFilter29, "Filter with roots - base 01", 2)

        # Filter with roots - base 02
        searchFilter30 = entity.SearchFilter()
        searchFilter30.names = ["Passenger"]
        searchFilter30.roots = [carId2]
        SearchResultCheck(searchFilter30, "Filter with roots - base 02", passengerId3)

        # Filter with roots - base 03
        searchFilter31 = entity.SearchFilter()
        searchFilter31.names = ["SportsCar"]
        searchFilter31.roots = [carId1]
        SearchResultsCheck(searchFilter31, "Filter with roots - base 03", 0)

        # Filter with roots - base 04
        searchFilter32 = entity.SearchFilter()
        searchFilter32.names = ["City|Street|SportsCar|Passenger"]
        searchFilter32.roots = [carId1]
        SearchResultsCheck(searchFilter32, "Filter with roots - base 04", 0)

        # Filter with roots - base 05
        searchFilter33 = entity.SearchFilter()
        searchFilter33.names = ["Car|Passenger"]
        searchFilter33.roots = [carId1]
        SearchResultsCheck(searchFilter33, "Filter with roots - base 05", 2)

        # Search with Roots - NameIsRootBased

        # Filter with roots - NameIsRootBased 01
        searchFilter34 = entity.SearchFilter()
        searchFilter34.names = ["Car|Passenger"]
        searchFilter34.names_are_root_based = False  # default
        SearchResultsCheck(searchFilter34, "Filter with roots - NameIsRootBased 01", 3)

        # Filter with roots - NameIsRootBased 02
        searchFilter35 = entity.SearchFilter()
        searchFilter35.names = ["Car|Passenger"]
        searchFilter35.names_are_root_based = True
        SearchResultsCheck(searchFilter35, "Filter with roots - NameIsRootBased 02", 0)

        # Filter with roots - NameIsRootBased 03
        searchFilter36 = entity.SearchFilter()
        searchFilter36.names = ["Car|Passenger"]
        searchFilter36.roots = [streetId]
        searchFilter36.names_are_root_based = False  # default
        SearchResultsCheck(searchFilter36, "Filter with roots - NameIsRootBased 03", 3)

        # Filter with roots - NameIsRootBased 04
        searchFilter37 = entity.SearchFilter()
        searchFilter37.names = ["Car|Passenger"]
        searchFilter37.roots = [streetId]
        searchFilter37.names_are_root_based = True
        SearchResultsCheck(searchFilter37, "Filter with roots - NameIsRootBased 04", 3)

        # Filter with roots - NameIsRootBased 05
        searchFilter38 = entity.SearchFilter()
        searchFilter38.names = ["Car|Passenger"]
        searchFilter38.roots = [carId2]
        searchFilter38.names_are_root_based = True
        SearchResultsCheck(searchFilter38, "Filter with roots - NameIsRootBased 05", 0)

        # Search with Multiple Filters
        searchFilter39 = entity.SearchFilter()
        searchFilter39.names = ["Pass*"]
        searchFilter39.roots = [sportsCarId]
        searchFilter39.components = { typeIdsList[1]:{} }
        searchFilter39.names_are_root_based = True
        searchFilter39.names_case_sensitive = True
        SearchResultCheck(searchFilter39, "Search with Multiple Filters", passengerId4)

        # Search with Aabb - Base

        aabb = math.Aabb()
        cityPosition = components.TransformBus(bus.Event, "GetWorldTranslation", cityId)
        print("City Position: ( " + str(cityPosition.x) + ", " + str(cityPosition.y) + ", " + str(cityPosition.z) + " )")

        # Filter by AABB - base 01
        searchFilter40 = entity.SearchFilter()
        aabbMin = math.Vector3(cityPosition.x - 1000.0, cityPosition.y - 1000.0, cityPosition.z - 1000.0)
        aabbMax = math.Vector3(cityPosition.x + 1000.0, cityPosition.y + 1000.0, cityPosition.z + 1000.0)
        aabb.Set(aabbMin, aabbMax)
        searchFilter40.aabb = aabb
        searchFilter40.names = ["City", "Street", "SportsCar", "Car", "Passenger"]
        searchFilter40.names_case_sensitive = True
        SearchResultsCheck(searchFilter40, "Filter by AABB - base 01", 10)

        # Filter by AABB - base 02
        searchFilter41 = entity.SearchFilter()
        aabbMin = math.Vector3(cityPosition.x - 1000.0, cityPosition.y - 1000.0, cityPosition.z - 1000.0)
        aabbMax = math.Vector3(cityPosition.x - 100.0, cityPosition.y - 100.0, cityPosition.z - 100.0)
        aabb.Set(aabbMin, aabbMax)
        searchFilter41.aabb = aabb
        searchFilter41.names = ["City", "Street", "SportsCar", "Car", "Passenger"]
        searchFilter41.names_case_sensitive = True
        SearchResultsCheck(searchFilter41, "Filter by AABB - base 02", 0)

        # Search with Component Properties - Base

        # Filter by Component Properties - base 01
        searchFilter42 = entity.SearchFilter()
        searchFilter42.components = { typeIdsList[1]:{'Render options|Draw character': True} }
        searchFilter42.names = ["City", "Street", "SportsCar", "Car", "Passenger"]
        SearchResultsCheck(searchFilter42, "Filter by Component Properties - base 01", 3)

        # Filter by Component Properties - base 02
        searchFilter43 = entity.SearchFilter()
        searchFilter43.components = { typeIdsList[0]:{}, typeIdsList[1]:{'Render options|Draw character': True} }
        searchFilter43.components_match_all = False
        searchFilter43.names = ["City", "Street", "SportsCar", "Car", "Passenger"]
        SearchResultsCheck(searchFilter43, "Filter by Component Properties - base 02", 4)

        # Filter by Component Properties - base 03
        searchFilter44 = entity.SearchFilter()
        searchFilter44.components = { typeIdsList[0]:{}, typeIdsList[1]:{'Render options|Draw character': True} }
        searchFilter44.components_match_all = True
        searchFilter44.names = ["City", "Street", "SportsCar", "Car", "Passenger"]
        SearchResultsCheck(searchFilter44, "Filter by Component Properties - base 03", 1)

if __name__ == "__main__":
    tester = Editor_EntitySearchCommands_Works()
    tester.test_case(tester.test)
