import os
import argparse
import math
import time

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as azentity
import azlmbr.object as azobj
import azlmbr.math as azmath
import azlmbr.asset as azasset
import azlmbr.terrain as terrain
import azlmbr.legacy.general as general
import azlmbr.shape as azshape

from editor_python_test_tools.editor_entity_utils import EditorEntity
from editor_python_test_tools.editor_entity_utils import EditorComponent

#Sphere Shape properties
#<13:54:50> (python_test) - Info: ['Sphere Shape', 'Sphere Shape|Sphere Configuration', 'Visible', 'Shape Color', 'Game View', 'Sphere Shape|Sphere Configuration|Radius', 'Filled']
#<13:54:50> (python_test) -
#<13:54:50> (python_test) - Sphere Shape: ('SphereShape', 'ShowChildrenOnly')
#<13:54:50> (python_test) -
#<13:54:50> (python_test) - Sphere Shape|Sphere Configuration: ('SphereShapeConfig', 'ShowChildrenOnly')
#<13:54:50> (python_test) -
#<13:54:50> (python_test) - Visible: ('bool', 'Visible')
#<13:54:50> (python_test) -
#<13:54:50> (python_test) - Shape Color: ('Color', 'Visible')
#<13:54:50> (python_test) -
#<13:54:50> (python_test) - Game View: ('bool', 'Visible')
#<13:54:50> (python_test) -
#<13:54:50> (python_test) - Sphere Shape|Sphere Configuration|Radius: ('float', 'Visible')
#<13:54:50> (python_test) -
#<13:54:50> (python_test) - Filled: ('bool', 'Visible')

# PhysX Shape Collider
# ython_test: Info: ['Collider configuration|Trigger', 'Collider configuration|Rotation', 'Collider configuration||Physics Materials', 'Collider configuration|Simulated', 'Collider configuration|In Scene Queries', 'Collider configuration||Physics Materials|[0]|', 'Debug draw settings|Draw collider', 'Collider configuration|', 'Collider configuration|Rest offset', 'Subdivision count', 'Collider configuration', 'Single sided', 'Collider configuration|Offset', 'Collider configuration|Tag', 'Debug draw settings', 'Collider configuration|Collision Layer', 'Collider configuration|Collides With', 'Collider configuration||Physics Materials|[0]', 'Collider configuration|Contact offset', 'Radius']<14:02:54> (python_test) - Info: ['Collider configuration|Trigger', 'Collider configuration|Rotation', 'Collider configuration||Physics Materials', 'Collider configuration|Simulated', 'Collider configuration|In Scene Queries', 'Collider configuration||Physics Materials|[0]|', 'Debug draw settings|Draw collider', 'Collider configuration|', 'Collider configuration|Rest offset', 'Subdivision count', 'Collider configuration', 'Single sided', 'Collider configuration|Offset', 'Collider configuration|Tag', 'Debug draw settings', 'Collider configuration|Collision Layer', 'Collider configuration|Collides With', 'Collider configuration||Physics Materials|[0]', 'Collider configuration|Contact offset', 'Radius']


#Mesh component
#4:14:10> (python_test) - Info: ['Controller|Configuration|Mesh Asset', 'Controller|Configuration|Lod Configuration|Minimum Screen Coverage', 'Controller|Configuration|Lod Configuration|Quality Decay Rate', 'Model Stats', 'Controller', 'Controller|Configuration', 'Controller|Configuration|Sort Key', 'Controller|Configuration|Model Asset', 'Controller|Configuration|Use ray tracing', 'Controller|Configuration|Lod Type', 'Controller|Configuration|Lod Configuration|Lod Override', 'Controller|Configuration|Exclude from reflection cubemaps', 'Controller|Configuration|Use Forward Pass IBL Specular', 'Model Stats|Mesh Stats']
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Info: Model Stats|Mesh Stats (AZStd::vector<EditorMeshStatsForLod, allocator>,Visible)
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Controller|Configuration|Mesh Asset: ('Asset<ModelAsset>', 'Visible')
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Controller|Configuration|Lod Configuration|Minimum Screen Coverage: ('float', 'NotVisible')
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Controller|Configuration|Lod Configuration|Quality Decay Rate: ('float', 'NotVisible')
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Model Stats: ('EditorMeshStats', 'Visible')
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Controller: ('AZ::Render::MeshComponentController', 'ShowChildrenOnly')
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Controller|Configuration: ('AZ::Render::MeshComponentConfig', 'ShowChildrenOnly')
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Controller|Configuration|Sort Key: ('AZ::s64', 'Visible')
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Controller|Configuration|Model Asset: ('Asset<ModelAsset>', 'Visible')
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Controller|Configuration|Use ray tracing: ('bool', 'Visible')
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Controller|Configuration|Lod Type: ('unsigned char', 'Visible')
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Controller|Configuration|Lod Configuration|Lod Override: ('unsigned char', 'NotVisible')
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Controller|Configuration|Exclude from reflection cubemaps: ('bool', 'Visible')
#<14:14:10> (python_test) -
#<14:14:10> (python_test) - Controller|Configuration|Use Forward Pass IBL Specular: ('bool', 'Visible')
#<14:14:10> (python_test) -


#Material Component
#<14:25:47> (python_test) - Info: ['Controller|Configuration|Mesh Asset', 'Controller|Configuration|Lod Configuration|Minimum Screen Coverage', 'Controller|Configuration|Lod Configuration|Quality Decay Rate', 'Model Stats', 'Model Stats|Mesh Stats|LOD 0|Mesh Count', 'Controller', 'Controller|Configuration', 'Model Stats|Mesh Stats|LOD 0', 'Controller|Configuration|Sort Key', 'Controller|Configuration|Model Asset', 'Model Stats|Mesh Stats|LOD 0|Vert Count', 'Controller|Configuration|Use ray tracing', 'Controller|Configuration|Lod Type', 'Controller|Configuration|Lod Configuration|Lod Override', 'Model Stats|Mesh Stats|LOD 0|Tri Count', 'Controller|Configuration|Exclude from reflection cubemaps', 'Controller|Configuration|Use Forward Pass IBL Specular', 'Model Stats|Mesh Stats']
#<14:25:47> (python_test) -
#<14:25:48> (python_test) - Info: ['Default Material|Material Slot Stable Id', 'LOD Materials', 'Controller', 'Controller|Materials', 'Model Materials|[0]|Material Asset', 'Enable LOD Materials', 'Model Materials|[0]|LOD Index', 'LOD Materials|LOD 0|[0]|Material Asset', 'Model Materials', 'Default Material|LOD Index', 'Model Materials|[0]|Material Slot Stable Id', 'LOD Materials|LOD 0', 'LOD Materials|LOD 0|[0]', 'LOD Materials|LOD 0|[0]|Material Slot Stable Id', 'Default Material', 'Default Material|Material Asset', 'Model Materials|[0]', 'LOD Materials|LOD 0|[0]|LOD Index']
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - Info: LOD Materials (AZStd::vector<AZStd::vector<EditorMaterialComponentSlot, allocator>, allocator>,NotVisible)
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - Info: Controller|Materials (AZStd::unordered_map<AZ::Render::MaterialAssignmentId, AZ::Render::MaterialAssignment, AZStd::hash<AZ::Render::MaterialAssignmentId>, AZStd::equal_to<AZ::Render::MaterialAssignmentId>, allocator>,Visible)
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - Info: Model Materials (AZStd::vector<EditorMaterialComponentSlot, allocator>,Visible)
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - Info: LOD Materials|LOD 0 (AZStd::vector<EditorMaterialComponentSlot, allocator>,Visible)
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - Default Material|Material Slot Stable Id: ('unsigned int', 'Visible')
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - Controller: ('MaterialComponentController', 'ShowChildrenOnly')
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - ]|Material Asset: ('Asset<MaterialAsset>', 'Visible')
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - Enable LOD Materials: ('bool', 'Visible')
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - ]|LOD Index: ('AZ::u64', 'Visible')
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - Default Material|LOD Index: ('AZ::u64', 'Visible')
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - ]|Material Slot Stable Id: ('unsigned int', 'Visible')
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - ]: ('EditorMaterialComponentSlot', 'ShowChildrenOnly')
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - Default Material: ('EditorMaterialComponentSlot', 'ShowChildrenOnly')
#<14:25:48> (python_test) -
#<14:25:48> (python_test) - Default Material|Material Asset: ('Asset<MaterialAsset>', 'Visible')

def DumpProperties(editorComponent: EditorComponent):
    """
        A helper function to print all properties of a component.
    """
    props = editorComponent.get_property_type_visibility()
    for key, value in props.items():
        print(f"{key}: {value}")


class TerrainPopulator():
    def __init__(self, maxEntities: int):
        terrainAabb = terrain.TerrainDataRequestBus(bus.Broadcast, 'GetTerrainAabb')
        print("aabb: ", terrainAabb)
        terrainSizeX = terrainAabb.GetXExtent()
        terrainSizeY = terrainAabb.GetYExtent()
        numColumns = math.sqrt(maxEntities)
        xOverY = terrainSizeX / terrainSizeY
        numColumnsX = numColumns * xOverY
        numRowsY = numColumns / xOverY
        spaceX = terrainSizeX / numColumnsX
        spaceY = terrainSizeY / numRowsY
        startingPosition = terrainAabb.min
        startingPosition.x = startingPosition.x - spaceX
        print(f"terrainSizeX={terrainSizeX}, terrainSizeY={terrainSizeY}, numColumns={numColumns}, xOverY={xOverY}, numObjectsX={numColumnsX}, numObjectsY={numRowsY}, spaceX={spaceX}, spaceY={spaceY}")
        #terrainSizeX=2048.0, terrainSizeY=1024.0, numColumns=94.86832980505137, xOverY=2.0, numObjectsX=189.73665961010275, numObjectsY=47.43416490252569, spaceX=10.793907746708069, spaceY=21.587815493416137
        self.currentSpawnPos = startingPosition
        self.terrainAabb = terrainAabb
        self.spaceX = spaceX
        self.spaceY = spaceY
        self.numColumnsX = numColumnsX
        self.numRowsY = numRowsY


    def AddStaticEntity(self, name: str, position: azmath.Vector3):
        physics_entity = EditorEntity.create_editor_entity(name)
        #physics_entity.set_world_translation(position)

        #Add sphere shape component.
        shapeComponent = physics_entity.add_component("Sphere Shape")
        shapeComponent.set_component_property_value("Sphere Shape|Sphere Configuration|Radius", 0.5)
        #DumpProperties(shapeComponent)

        #Add PhysX Shape Collider.
        shapeColliderComponent = physics_entity.add_component("PhysX Shape Collider")
        #DumpProperties(shapeColliderComponent)

        #Add child entity with mesh + material at local (0,0,-0.5)
        mesh_entity = EditorEntity.create_editor_entity("Mesh", physics_entity.id )
        mesh_entity.set_local_translation(azmath.Vector3_ConstructFromValues(0.0, 0.0, -0.5))

        #Add mesh component with asset.
        meshComponent = mesh_entity.add_component("Mesh")
        meshAssetPath = os.path.join("models", "sphere.azmodel")
        meshAsset = azasset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", meshAssetPath, azmath.Uuid(), False)
        meshComponent.set_component_property_value("Controller|Configuration|Model Asset", meshAsset)
        #DumpProperties(meshComponent)
        #D:\GIT\o3de\Gems\Atom\Feature\Common\Assets\Models\sphere.fbx

        #Add material component.
        materialComponent = mesh_entity.add_component("Material")
        materialAssetPath = os.path.join("materials", "presets", "pbr", "metal_aluminum.azmaterial")
        materialAsset = azasset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", materialAssetPath, azmath.Uuid(), False)
        materialComponent.set_component_property_value("Default Material|Material Asset", materialAsset)
        #DumpProperties(materialComponent)
        #D:\GIT\o3de\Gems\Atom\Feature\Common\Assets\Materials\Presets\PBR\metal_aluminum.material

        #Apply offset zup:
        aabb = azshape.ShapeComponentRequestsBus(bus.Event, 'GetEncompassingAabb', physics_entity.id)
        position.z = position.z + aabb.GetZExtent() * 0.5
        physics_entity.set_world_translation(position)

        #DumpProperties(shapeComponent)
        #pte = shapeComponent.get_property_tree()
        #print(pte)


    def GetNextSpawnPos(self):
        spawnPosX = self.currentSpawnPos.x
        spawnPosY = self.currentSpawnPos.y
        deltaX = self.spaceX
        deltaY = self.spaceY
        maxX = self.terrainAabb.max.x
        minX = self.terrainAabb.min.x

        spawnPosX = spawnPosX + deltaX
        if spawnPosX > maxX:
            spawnPosX = minX
            spawnPosY = spawnPosY + deltaY
        
        # Get the height at the current x,y
        spawnPos = azmath.Vector3_ConstructFromValues(spawnPosX, spawnPosY, 0.0)
        spawnPosZ, isHole = terrain.TerrainDataRequestBus(bus.Broadcast, 'GetHeight', spawnPos, 0)
        #local spawnPosZ = tuple_f_b:Get0()
        #local isHole = tuple_f_b:Get1()
        spawnPos.z = spawnPosZ
        #print(f"GetNextSpawnPos {spawnPos}")
        self.currentSpawnPos = azmath.Vector3_ConstructFromValues(spawnPos.x, spawnPos.y, spawnPos.z)
        return spawnPos


    def RecalculateStartingPosition(self, startingIdx: int, numEntities: int):
        spawnPosX = self.currentSpawnPos.x
        spawnPosY = self.currentSpawnPos.y
        deltaX = self.spaceX
        deltaY = self.spaceY
        maxX = self.terrainAabb.max.x
        minX = self.terrainAabb.min.x
        for idx in range(startingIdx):
            spawnPosX = spawnPosX + deltaX
            if spawnPosX > maxX:
                spawnPosX = minX
                spawnPosY = spawnPosY + deltaY
        self.currentSpawnPos = azmath.Vector3_ConstructFromValues(spawnPosX, spawnPosY, 0.0)
        print(f"{self.currentSpawnPos}")

        #offsetIndexY = int(startingIdx / self.numColumnsX)
        #print(f"{offsetIndexY}, {startingIdx}, {self.numColumnsX}, {self.spaceY}") #51, 4899, 94.86832980505137, 21.587815493416137
        #startingPosition.y = startingPosition.y + offsetIndexY * self.spaceY
        #offsetIndexX = startingIdx - offsetIndexY * self.numObjectsX
        #startingPosition.x = startingPosition.x + offsetIndexX * self.spaceX
        #startingPosition.x = startingPosition.x - self.spaceX
        #self.currentSpawnPos = azmath.Vector3_ConstructFromValues(startingPosition.x, startingPosition.y, startingPosition.z)


    def PopulateLevel(self, startingIdx: int, numEntities: int, saveCount: int):
        self.RecalculateStartingPosition(startingIdx, numEntities)
        start_time = time.time()
        batch_count = 0
        for idx in range(numEntities):
            spawnPos = self.GetNextSpawnPos()
            index = idx + startingIdx
            self.AddStaticEntity(f"Static{index}", spawnPos)
            general.idle_wait_frames(1)
            elapsed_time = time.time() - start_time
            print(f"Added Entity {idx} of {numEntities}. pos: {spawnPos} Elapsed {elapsed_time} seconds")
            batch_count = batch_count + 1
            if (saveCount > 0) and (batch_count >= saveCount):
                general.save_level()
                print("Level saved")
                batch_count = 0

# Usage examples:
# Example 1: To add 100 entities in a single call:
# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 100 -o 0 -c 100 

# Example 2:  To add 100 entities in two batches:
# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 100 -o 0 -c 50
# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 100 -o 50 -c 50

# Example 3: To add 9000 entities in one call (Can take arund 5 hours :-))
# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 9000 -o 0 -c 9000 -s 100

# Example 4: To add the first 5000 out of 9000 entities in five batches (Can take arund 40 minutes per batch :-))
# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 9000 -o 0 -c 1000
# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 9000 -o 1000 -c 1000
# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 9000 -o 2000 -c 1000
# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 9000 -o 3000 -c 1000
# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 9000 -o 4000 -c 1000

# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 9000 -o 4899 -c 4101 -s 100
# from 4899 to 8998 the debugger ran out of memory.
# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 9000 -o 8998 -c 4

# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 10000 -o 0 -c 10000 -s 100
# pyRunFile D:\GIT\o3de\AutomatedTesting\Levels\goal_9ks_1kd\Scripts\populate_static_objects.py -m 10000 -o 5099 -c 4901 -s 100
if __name__ == "__main__":
    optparser = argparse.ArgumentParser(description='Populates a level, evenly spaced out, with N entities above the terrain')
    optparser.add_argument("-m", "--max_entities", type=int, required=True,
        help="The maximum number of static entities to place.")
    optparser.add_argument("-o", "--offset_count", type=int, required=True,
        help="The entity index to start from.")
    optparser.add_argument("-c", "--count", type=int, required=True,
        help="The count of entities to add in this batch.")
    optparser.add_argument("-s", "--save_frequency", type=int, default=0,
        help="Batch count of created entities to save the level.")
    
    args = optparser.parse_args()
    terrainPopulator = TerrainPopulator(args.max_entities)
    terrainPopulator.PopulateLevel(args.offset_count, args.count, args.save_frequency)
