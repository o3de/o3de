"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.atom
import azlmbr.math as math
import azlmbr.asset as asset
import azlmbr.bus as bus
import azlmbr.editor
import azlmbr.math as math
import random

from azlmbr.entity import EntityId
from azlmbr.render import MaterialAssignmentId
from azlmbr.name import Name

materials = [   'materials/presets/macbeth/00_illuminant.azmaterial',
                'materials/presets/macbeth/03_blue_sky.azmaterial',
                'materials/presets/macbeth/02_light_skin_tex.azmaterial',
                'materials/presets/macbeth/04_foliage_tex.azmaterial',
                'materials/presets/macbeth/07_orange.azmaterial',
                'materials/presets/macbeth/09_moderate_red_tex.azmaterial']

textures = [    'materials/presets/macbeth/05_blue_flower_srgb.tif.streamingimage',
                'materials/presets/macbeth/06_bluish_green_srgb.tif.streamingimage',
                'materials/presets/macbeth/09_moderate_red_srgb.tif.streamingimage',
                'materials/presets/macbeth/11_yellow_green_srgb.tif.streamingimage',
                'materials/presets/macbeth/12_orange_yellow_srgb.tif.streamingimage',
                'materials/presets/macbeth/17_magenta_srgb.tif.streamingimage']


def getRandomMaterial():
    material = random.choice(materials)
    return azlmbr.asset.AssetCatalogRequestBus(azlmbr.bus.Broadcast, 'GetAssetIdByPath', material, math.Uuid(), False)

def assetIdToPath(assetId):
    return azlmbr.asset.AssetCatalogRequestBus(azlmbr.bus.Broadcast, 'GetAssetPathById', assetId)

# List all slot ids
def getIds(entityId):
    print("Get material ids")
    return azlmbr.render.MaterialComponentRequestBus(azlmbr.bus.Event, 'GetOriginalMaterialAssignments', entityId)

def printMaterials(entityId):
    materials = azlmbr.render.MaterialComponentRequestBus(azlmbr.bus.Event, 'GetMaterialOverrides', entityId)
    print(f"Materials: {[(id.ToString(), override.ToString()) for id, override in materials.items()]}", sep='\n')

def setMaterialOverrides(entityId, ids):
    print("Setting material overrides")
    for id in ids:
        azlmbr.render.MaterialComponentRequestBus(azlmbr.bus.Event, 'SetMaterialOverride', entityId, id, getRandomMaterial())

# Sets overrides of 3 types: float, color, and texture
def setPropertyOverrides(entityId, ids):
    print("Setting property overrides")
    for id in ids:
        # factor override (float)
        factor = random.random()
        propertyName = Name("baseColor.factor")
        azlmbr.render.MaterialComponentRequestBus(azlmbr.bus.Event, 'SetPropertyOverride', entityId, id, propertyName, factor)

        # color override
        color = math.Color(random.random(), random.random(), random.random(), 1.0)
        propertyName = Name("baseColor.color")
        azlmbr.render.MaterialComponentRequestBus(azlmbr.bus.Event, 'SetPropertyOverride', entityId, id, propertyName, color)

        # texture override
        texturePath = random.choice(textures)
        assetId = azlmbr.asset.AssetCatalogRequestBus(azlmbr.bus.Broadcast, 'GetAssetIdByPath', texturePath, math.Uuid(), False)
        propertyName = Name("baseColor.textureMap")
        azlmbr.render.MaterialComponentRequestBus(azlmbr.bus.Event, 'SetPropertyOverride', entityId, id, propertyName, assetId)

# Convert property value to string
def propertyValueToString(value):
    if isinstance(value, (int, float, bool, str)):
        return value
    if value.typename == 'Color':
        return 'Color(r={:.2f}, g={:.2f}, b={:.2f}, a={:.2f})'.format(value.r, value.g, value.b, value.a)
    if value.typename == 'ImageBinding':
        return assetIdToPath(value.GetAssetId())
    if value.typename == 'AssetId':
        return assetIdToPath(value)
    return value

# print all property overrides on a material component
def printPropertyOverrides(entityId, ids):
    print("Property overrides:")
    for id in ids:
        propertyOverrides = azlmbr.render.MaterialComponentRequestBus(azlmbr.bus.Event, 'GetPropertyOverrides', entityId, id)
        print(f"{id.ToString(), [(name.ToString(), propertyValueToString(override)) for name, override in propertyOverrides.items()]}", sep='\n')

# Clear property overrides for a single slot
def clearPropertyOverrides(entityId, id):
    print(f"Clearing property overrides for slot <{id.ToString()}>")
    azlmbr.render.MaterialComponentRequestBus(azlmbr.bus.Event, 'ClearPropertyOverrides', entityId, id)

# Clear all property overrides for the material
def clearAllPropertyOverrides(entityId):
    print("Clearing remaining property overrides")
    azlmbr.render.MaterialComponentRequestBus(azlmbr.bus.Event, 'ClearAllPropertyOverrides', entityId)

def run():
    searchFilter = azlmbr.entity.SearchFilter()
    searchFilter.names = ["Entity1"]
    matching_entities = azlmbr.entity.SearchBus(bus.Broadcast, 'SearchEntities', searchFilter)
    if matching_entities == 0:
        print('Could not find entity')
        return

    entityId = matching_entities[0]

    # Get ids
    ids = getIds(entityId)
    print(f"Slots: {[id.ToString() for id in ids]}", sep='\n')

    # Print current materials
    printMaterials(entityId)

    # Set material overrides
    setMaterialOverrides(entityId, ids)

    # Print current materials
    printMaterials(entityId)

    # Set property overrides
    setPropertyOverrides(entityId, ids)

    # Print current property overrides
    printPropertyOverrides(entityId, ids)

    # Clear property overrides for first slot
    for id in ids:
        clearPropertyOverrides(entityId, id)
        break

    # Print current property overrides
    printPropertyOverrides(entityId, ids)

    # Clear remaining property overrides
    clearAllPropertyOverrides(entityId)

    # Print current property overrides
    printPropertyOverrides(entityId, ids)

if __name__ == "__main__":
    run()
