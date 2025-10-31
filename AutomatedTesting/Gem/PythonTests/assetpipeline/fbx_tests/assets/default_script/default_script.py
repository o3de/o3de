#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os

sceneJobHandler = None

def clear_sceneJobHandler():
    global sceneJobHandler
    # do not delete or set sceneJobHandler to None, just disconnect from it.
    # this call is occurring while the scene Job Handler itself is in the call stack,
    # so deleting it here would cause a crash.
    sceneJobHandler.disconnect()

def on_prepare_for_export(args):
    import azlmbr.scene
    import azlmbr.object
    import azlmbr.paths
    import azlmbr.math

    scene = args[0] # azlmbr.scene.Scene
    outputDirectory = args[1] # string

    jsonFilename = os.path.basename(scene.sourceFilename)
    jsonFilename = os.path.join(outputDirectory, jsonFilename + '.default_script')

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
    exportProduct.subId = 1

    exportProductList = azlmbr.scene.ExportProductList()
    exportProductList.AddProduct(exportProduct)
    return exportProductList

import azlmbr.scene
sceneJobHandler = azlmbr.scene.ScriptBuildingNotificationBusHandler()
sceneJobHandler.connect()
sceneJobHandler.add_callback('OnPrepareForExport', on_prepare_for_export)
