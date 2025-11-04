# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

import sys
import shutil
from pathlib import Path
import json

def isSpawnableAsset(variableDatum):
    return (variableDatum.get('$type') == 'SpawnableAsset' or
        variableDatum.get('scriptCanvasType', {}).get('m_azType') == '{A96A5037-AD0D-43B6-9948-ED63438C4A52}')

fullPath = sys.argv[1]

path = Path(fullPath)
fileName = path.stem
pathToFile = path.parent.resolve()
extension = path.suffix

fileNameBackup = fileName + "_bckup"
fullBackupPath = (path.parent).joinpath(fileNameBackup).with_suffix(extension)

shutil.copy(fullPath, fullBackupPath)
print("Backup saved to \'%s\'" % fullBackupPath)

with open(fullPath, 'r') as file :
    filedata = file.read()

prefab_dom = json.loads(filedata)
# inspect DOM, do fixup - it'll be stored as a python dict

components = prefab_dom['ClassData']['m_scriptCanvas']['Components']

# replace 'Asset' with 'asset' in SpawnableScriptAssetRef variables
for componentKey, componentValue in components.items():
    if 'm_variableData' in componentValue:
        variableData = componentValue['m_variableData']['m_nameVariableMap']
        for variable in variableData:
            variableDatum = variable.get('Value', {}).get('Datum', {})
            if isSpawnableAsset(variableDatum):
                if 'value' in variableDatum:
                    asset = variableDatum['value'].pop('Asset')
                    variableDatum['value']['asset'] = asset

filedata = json.dumps(prefab_dom, indent=4)

# finally replace all matching type Uuids and type names
# SpawnTicketInstance -> EntitySpawnTicket
filedata = filedata.replace('"$type": "SpawnTicketInstance"', '"$type": "AzFramework::EntitySpawnTicket"')
filedata = filedata.replace('{2B5EB938-8962-4A43-A97B-112F398C604B}', '{BA62FF9A-A01E-4FEB-84C6-200881DF2B2B}')
# TicketArray
filedata = filedata.replace('{0F155764-DFEF-50FB-9B6E-E0EE9223A683}', '{7541F631-BA89-54F3-A6B8-33FF75B60192}')
# StringToTicketMap
filedata = filedata.replace('{C63B2684-DC8D-5C8F-B635-ABC248EEEF05}', '{1D4D0809-B9FD-5766-8F56-B6BD6F5CC16C}')
# NumberToTicketMap
filedata = filedata.replace('{C11E8C62-060A-5EC0-9370-412E43A66FED}', '{E7D8B6D5-ABF2-5067-86BA-B25CE38421C3}')

# SpawnableAsset -> SpawnableScriptAssetRef (no need to replace Uuids, since this is only class rename)
filedata = filedata.replace('"$type": "SpawnableAsset"', '"$type": "AzFramework::Scripts::SpawnableScriptAssetRef"')

with open(fullPath, 'w') as file:
    file.write(filedata)

print("Asset successfully updated")