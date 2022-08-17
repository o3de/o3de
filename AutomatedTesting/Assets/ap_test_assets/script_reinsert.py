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
    sceneJobHandler.disconnect()
    sceneJobHandler = None

def on_prepare_for_export(args):
    print (f'on_prepare_for_export')
    try:
        scene = args[0] # azlmbr.scene.Scene
        outputDirectory = args[1] # string
        platformIdentifier = args[2] # string
        productList = args[3] # azlmbr.scene.ExportProductList
        # return export_chunk_asset(scene, outputDirectory, platformIdentifier, productList)
        clear_sceneJobHandler()
        return {}
    except:
        #log_exception_traceback()
        clear_sceneJobHandler()
        return {}

def on_update_manifest(args):
    print (f'on_update_manifest')
    data = '{}'
    try:
        scene = args[0]
        #data = update_manifest(scene)
    except RuntimeError as err:
        print(f'ERROR - {err}')
        #log_exception_traceback()
    except:
        print(f'ERROR - {err}')
        #log_exception_traceback()

    clear_sceneJobHandler()
    return data

# try to create SceneAPI handler for processing
try:
    import azlmbr.scene as sceneApi
    
    print (f'sceneJobHandler')

    if sceneJobHandler is None:
        sceneJobHandler = sceneApi.ScriptBuildingNotificationBusHandler()
        sceneJobHandler.connect()
        sceneJobHandler.add_callback('OnUpdateManifest', on_update_manifest)
        sceneJobHandler.add_callback('OnPrepareForExport', on_prepare_for_export)

except:
    sceneJobHandler = None
