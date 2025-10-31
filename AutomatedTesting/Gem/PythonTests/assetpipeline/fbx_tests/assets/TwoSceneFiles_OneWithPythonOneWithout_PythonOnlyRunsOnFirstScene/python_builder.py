"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import datetime, uuid, os
import azlmbr.scene as sceneApi
import azlmbr.scene.graph

def output_test_data(scene):
    source_filename = os.path.basename(scene.sourceFilename)
    source_filename = source_filename.replace('.','_')
        
    log_output_file_name = f"{source_filename}.log"

    log_output_folder = os.path.dirname(scene.sourceFilename)
    log_output_location = os.path.join(log_output_folder, log_output_file_name)
    
    # Saving a file to the temp folder is the easiest way to have this test communicate
    # with the outer python test.
    with open(log_output_location, "w") as f:
        # Just write something to the file, but the filename is the main information
        # used for the test.
        f.write(f"scene.sourceFilename: {scene.sourceFilename}\n")
    return ''

mySceneJobHandler = None

def on_update_manifest(args):
    scene = args[0]
    result = output_test_data(scene)
    global mySceneJobHandler
    # do not delete or set sceneJobHandler to None, just disconnect from it.
    # this call is occuring while the scene Job Handler itself is in the callstack, so deleting it here
    # would cause a crash.
    mySceneJobHandler.disconnect()
    return result

def main():
    global mySceneJobHandler
    mySceneJobHandler = sceneApi.ScriptBuildingNotificationBusHandler()
    mySceneJobHandler.connect()
    mySceneJobHandler.add_callback('OnUpdateManifest', on_update_manifest)

if __name__ == "__main__":
    main()