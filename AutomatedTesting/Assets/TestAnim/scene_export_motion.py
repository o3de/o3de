#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import traceback, sys, uuid, os, json

import scene_export_utils
import scene_api.motion_group

#
# Example for exporting MotionGroup scene rules
#

def update_manifest(scene):
    import azlmbr.scene.graph
    import scene_api.scene_data

    # create a SceneManifest
    sceneManifest = scene_api.scene_data.SceneManifest()

    # create a MotionGroup
    motionGroup = scene_api.motion_group.MotionGroup()
    motionGroup.name = os.path.basename(scene.sourceFilename.replace('.', '_'))

    motionAdditiveRule = scene_api.motion_group.MotionAdditiveRule()
    motionAdditiveRule.sampleFrame = 2
    motionGroup.add_rule(motionAdditiveRule)

    motionScaleRule = motionGroup.create_rule(scene_api.motion_group.MotionScaleRule())
    motionScaleRule.scaleFactor = 1.1
    motionGroup.add_rule(motionScaleRule)

    # add motion group to scene manifest
    sceneManifest.add_motion_group(motionGroup)

    # Convert the manifest to a JSON string and return it
    return sceneManifest.export()

sceneJobHandler = None

def on_update_manifest(args):
    try:
        scene = args[0]
        return update_manifest(scene)
    except RuntimeError as err:
        print (f'ERROR - {err}')
        scene_export_utils.log_exception_traceback()
    except:
        scene_export_utils.log_exception_traceback()
    finally:
        global sceneJobHandler
        # do not delete or set sceneJobHandler to None, just disconnect from it.
        # this call is occuring while the scene Job Handler itself is in the callstack, so deleting it here
        # would cause a crash.
        sceneJobHandler.disconnect()

# try to create SceneAPI handler for processing
try:
    import azlmbr.scene

    sceneJobHandler = azlmbr.scene.ScriptBuildingNotificationBusHandler()
    sceneJobHandler.connect()
    sceneJobHandler.add_callback('OnUpdateManifest', on_update_manifest)
except:
    sceneJobHandler = None
