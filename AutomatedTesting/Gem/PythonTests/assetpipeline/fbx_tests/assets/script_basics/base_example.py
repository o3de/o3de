#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

sceneJobHandler = None

def clear_sceneJobHandler():
    global sceneJobHandler
    # do not delete or set sceneJobHandler to None, just disconnect from it.
    # this call is occuring while the scene Job Handler itself is in the callstack, so deleting it here
    # would cause a crash.
    sceneJobHandler.disconnect()

def on_prepare_for_export(args): 
    print (f'on_prepare_for_export')

    import azlmbr.scene
    import azlmbr.object
    import azlmbr.paths
    import json, os
    import azlmbr.math

    scene = args[0] # azlmbr.scene.Scene
    outputDirectory = args[1] # string

    # write out a fake export product asset so that it can be found and tested
    jsonFilename = os.path.basename(scene.sourceFilename)
    jsonFilename = os.path.join(outputDirectory, jsonFilename + '.fake_asset')

    # prepare output folder
    basePath, _ = os.path.split(jsonFilename)
    outputPath = os.path.join(outputDirectory, basePath)
    if not os.path.exists(outputPath):
        os.makedirs(outputPath, False)

    # write out a JSON file
    with open(jsonFilename, "w") as jsonFile:
        jsonFile.write("{}")

    clear_sceneJobHandler()

    exportProduct = azlmbr.scene.ExportProduct()
    exportProduct.filename = jsonFilename
    exportProduct.sourceId = scene.sourceGuid
    exportProduct.subId = 2

    exportProductList = azlmbr.scene.ExportProductList()
    exportProductList.AddProduct(exportProduct)
    return exportProductList

def on_update_manifest(args):
    print (f'on_update_manifest')
    data = """{
        "values": [
            {
                "$type": "{07B356B7-3635-40B5-878A-FAC4EFD5AD86} MeshGroup",
                "name": "fake",
                "nodeSelectionList": { "selectedNodes": ["fake_node"] }
            }
        ]
    }"""
    clear_sceneJobHandler()
    return data

# try to create SceneAPI handler for processing
try:
    import azlmbr.scene as sceneApi
    
    sceneJobHandler = sceneApi.ScriptBuildingNotificationBusHandler()
    sceneJobHandler.connect()
    sceneJobHandler.add_callback('OnUpdateManifest', on_update_manifest)
    sceneJobHandler.add_callback('OnPrepareForExport', on_prepare_for_export)

except:
    sceneJobHandler = None
