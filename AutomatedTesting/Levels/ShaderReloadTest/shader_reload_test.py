#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import os
import numpy as np
import tempfile
from PIL import Image

import azlmbr.bus as azbus
import azlmbr.legacy.general as azgeneral
import azlmbr.editor as azeditor
import azlmbr.atom as azatom
import azlmbr.entity as azentity
import azlmbr.camera as azcamera

# See README.md for details.

# Some global constants
EXPECTED_LEVEL_NAME = "ShaderReloadTest"
SUPPORTED_IMAGE_FILE_EXTENSIONS = {".ppm", ".bmp", ".tiff", ".tif", ".png"}

def OverwriteFile(azslFilePath: str, begin: list, middle: list, end: list):
    fileObj = open(azslFilePath, "w")
    fileObj.writelines(begin)
    fileObj.writelines(middle)
    fileObj.writelines(end)
    fileObj.close()


def FlipTestColorLine(line: str) -> tuple[tuple[int, int, int], str]:
    """
    Flips/toggles "BLUE_COLOR" for "GREEN_COLOR" in @line
    and viceversa. Also retuns the expected pixel color in addition
    to the modified line.
    """
    if "BLUE_COLOR" in line:
        expectedColor = "GREEN_COLOR"
        newLine = line.replace("BLUE_COLOR", expectedColor)
        return (0, 152, 15), newLine
    elif "GREEN_COLOR" in line:
        expectedColor = "BLUE_COLOR"
        newLine = line.replace("GREEN_COLOR", expectedColor)
        return (0, 1, 155), newLine
    raise Exception("Can't find color")


def FlipShaderColor(azslFilePath: str) -> tuple[int, int, int]:
    """
    Modifies the Shader code in @azslFilePath by flipping the line
    that outputs the expected color.
    If the line is found as:
        const float3 TEST_COLOR = BLUE_COLOR;
    It gets flipped to:
        const float3 TEST_COLOR = GREEN_COLOR;
    and viceversa.
    """
    begin = []
    middle = []
    end = []
    fileObj = open(azslFilePath, 'rt')
    sectionStart = False
    sectionEnd = False
    expectedColor = ""
    for line in fileObj:
        if ("ShaderReloadTest" in line):
            if ("START" in line):
                middle.append(line)
                sectionStart = True
                continue
            elif ("END" in line):
                middle.append(line)
                sectionEnd = True
                continue
        if (not sectionStart) and (not sectionEnd):
            begin.append(line)
            continue
        if sectionEnd:
            end.append(line)
            continue
        if "TEST_COLOR" in line:
            expectedColor, newLine = FlipTestColorLine(line)
            middle.append(newLine)

    fileObj.close()
    OverwriteFile(azslFilePath, begin, middle, end)
    return expectedColor
    

def UpdateShaderAndTestPixelResult(azslFilePath: str, screenUpdateWaitTime: float, captureCountOnFailure: int, screenshotImagePath: str, quickShaderOverwriteCount: int, quickShaderOverwriteWait: float) -> bool:
    """
    This function represents the work done for a single iteration of the shader modification test.
    Modifies the content of the Shader (@azslFilePath), waits @screenUpdateWaitTime, and captures
    the pixels of the Editor Viewport.  
    Some leniency was added with the variable @captureCountOnFailure because sometimes @@screenUpdateWaitTime is not
    enought time for the screen to update. This function retries @captureCountOnFailure times before
    considering it a failure.
    """
    expectedPixelColor = FlipShaderColor(azslFilePath)
    print(f"Expecting color {expectedPixelColor}")
    for overwriteCount in range(quickShaderOverwriteCount):
        azgeneral.idle_wait(quickShaderOverwriteWait)
        expectedPixelColor = FlipShaderColor(azslFilePath)
        print(f"Shader quickly overwritten {overwriteCount + 1} of {quickShaderOverwriteCount}")
        print(f"Expecting color {expectedPixelColor}")
    azgeneral.idle_wait(screenUpdateWaitTime)
    # Capture the screenshot
    captureCount = -1
    success = False
    while captureCount < captureCountOnFailure:
        captureCount += 1
        outcome = azatom.FrameCaptureRequestBus(
                    azbus.Broadcast, "CaptureScreenshot", screenshotImagePath
                )
        if not outcome.IsSuccess():
            frameCaptureError = outcome.GetError()
            errorMsg = frameCaptureError.error_message
            print(f"Failed to capture screenshot at outputImagePath='{screenshotImagePath}'\nError:\n{errorMsg}")
            return False
        azgeneral.idle_wait(screenUpdateWaitTime)
        img = Image.open(screenshotImagePath)
        width, height = img.size
        r = int(height/2)
        c = int(width/2)
        image_array = np.array(img)
        color = image_array[r][c]
        print(f"captureCount {captureCount}: Center Pixel[{r},{c}] Color={color}, type:{type(color)}, r={color[0]}, g={color[1]}, b={color[2]}")
        success = (color[0] == expectedPixelColor[0]) and (color[1] == expectedPixelColor[1]) and (color[2] == expectedPixelColor[2])
        if success:
            return success
    return success 


def ShaderReloadTest(iterationCountMax: int, screenUpdateWaitTime: float, captureCountOnFailure: int, screenshotImagePath: str, quickShaderOverwriteCount: int, quickShaderOverwriteWait: float) -> tuple[bool, int]:
    """
    This function is the main loop. Runs @iterationCountMax iterations and all iterations must PASS
    to consider the test a success.
    A single iteration modifies the Shader file, waits @screenUpdateWaitTime, captures the pixel content
    of the Editor Viewport, and reads the center pixel of the image for an expected color.
    """    
    levelPath = azeditor.EditorToolsApplicationRequestBus(azbus.Broadcast, "GetCurrentLevelPath")
    levelName = os.path.basename(levelPath)
    if levelName != EXPECTED_LEVEL_NAME:
        print(f"ERROR: This test suite expects a level named '{EXPECTED_LEVEL_NAME}', instead got '{levelName}'")
        return False, 0
    azslFilePath = os.path.join(levelPath, "SimpleMesh.azsl") 

    iterationCount = 0
    success = False
    while iterationCount < iterationCountMax:
        iterationCount += 1
        print(f"Starting Retry {iterationCount} of {iterationCountMax}...")
        success = UpdateShaderAndTestPixelResult(azslFilePath, screenUpdateWaitTime, captureCountOnFailure, screenshotImagePath, quickShaderOverwriteCount, quickShaderOverwriteWait)
        if not success:
            break
    return success, iterationCount


def ValidateImageExtension(screenshotImagePath: str) -> bool:
    _, file_extension = os.path.splitext(screenshotImagePath)
    if file_extension in SUPPORTED_IMAGE_FILE_EXTENSIONS:
        return True
    print(f"ERROR: Image path '{screenshotImagePath}' has an unsupported file extension.\nSupported extensions: {SUPPORTED_IMAGE_FILE_EXTENSIONS}")
    return False


def AdjustEditorCameraPosition(cameraEntityName: str) -> azentity.EntityId:
    """
    Searches for an entity named @cameraEntityName, assumes the entity has a Camera Component,
    and forces the Editor Viewport to make it the Active Camera. This helps center the `Billboard`
    entity because this test Samples the middle the of the screen for the correct Pixel color.
    """
    if not cameraEntityName:
        return None
    # Find the first entity with such name.
    searchFilter = azentity.SearchFilter()
    searchFilter.names = [cameraEntityName,]
    entityList = azentity.SearchBus(azbus.Broadcast, "SearchEntities", searchFilter)
    print(f"Found {len(entityList)} entities named {cameraEntityName}. Will use the first.")
    if len(entityList) < 1:
        print(f"No camera entity with name {cameraEntityName} was found. Viewport camera won't be adjusted.")
        return None
    cameraEntityId = entityList[0]
    isActiveCamera = azcamera.EditorCameraViewRequestBus(azbus.Event, "IsActiveCamera", cameraEntityId)
    if isActiveCamera:
        print(f"Entity '{cameraEntityName}' is already the active camera")
        return cameraEntityId
    azcamera.EditorCameraViewRequestBus(azbus.Event, "ToggleCameraAsActiveView", cameraEntityId)
    print(f"Entity '{cameraEntityName}' is now the active camera. Will wait 2 seconds for the screen to settle.")
    print(f"REMARK: It is expected that the camera is located at [0, -1, 2] with all euler angles at 0.")
    azgeneral.idle_wait(2.0)
    return cameraEntityId


def ClearViewportOfHelpers():
    """
    Makes sure all helpers and artifacts that add unwanted pixels
    are hidden.
    """
    # Make sure no entity is selected when the test runs because entity selection adds unwanted colored pixels
    azeditor.ToolsApplicationRequestBus(azbus.Broadcast, "SetSelectedEntities", [])
    # Hide helpers
    if azgeneral.is_helpers_shown():
        azgeneral.toggle_helpers()
    # Hide icons
    if azgeneral.is_icons_shown():
        azgeneral.toggle_icons()
    #Hide FPS, etc
    azgeneral.set_cvar_integer("r_displayInfo", 0)
    # Wait a little for the screen to update.
    azgeneral.idle_wait(0.25)


# Quick Example on how to run this test from the Editor Console (See README.md for more details):
# Runs 10 iterations:
# pyRunFile C:\GIT\o3de\AutomatedTesting\Levels\ShaderReloadTest\shader_reload_test.py -i 10
def MainFunc():
    import argparse

    parser = argparse.ArgumentParser(
        description="Records several frames of pass attachments as image files."
    )

    parser.add_argument(
        "-i",
        "--iterations",
        type=int,
        default=1,
        help="How many times the Shader should be modified and the screen pixel validated.",
    )

    parser.add_argument(
        "--screen_update_wait_time",
        type=float,
        default=3.0,
        help="Minimum time to wait after modifying the shader and taking the screen snapshot to validate color output.",
    )

    parser.add_argument(
        "--capture_count_on_failure",
        type=int,
        default=2,
        help="How many times the screen should be recaptured if the pixel output failes.",
    )

    parser.add_argument(
        "-p",
        "--screenshot_image_path",
        default="",
        help="Absolute path of the file where the screenshot will be written to. Must include image extensions 'ppm', 'png', 'bmp', 'tif'. By default a temporary png path will be created",
    )

    parser.add_argument(
        "-q",
        "--quick_shader_overwrite_count",
        type=int,
        default=0,
        help="How many times the shader should be overwritten before capturing the screenshot. This simulates real life cases where a shader file is updated and saved to the file system several times consecutively",
    )

    parser.add_argument(
        "-w",
        "--quick_shader_overwrite_wait",
        type=float,
        default=1.0,
        help="Minimum time to wait in between quick shader overwrites.",
    )

    parser.add_argument(
        "-c",
        "--camera_entity_name",
        default="Camera",
        help="Name of the entity that contains a Camera Component. If found, the Editor camera will be set to it before starting the test.",
    )

    args = parser.parse_args()
    iterationCountMax = args.iterations
    screenUpdateWaitTime = args.screen_update_wait_time
    captureCountOnFailure = args.capture_count_on_failure
    screenshotImagePath = args.screenshot_image_path
    quickShaderOverwriteCount = args.quick_shader_overwrite_count
    quickShaderOverwriteWait = args.quick_shader_overwrite_wait
    cameraEntityName = args.camera_entity_name
    tmpDir = None
    if not screenshotImagePath:
        tmpDir = tempfile.TemporaryDirectory()
        screenshotImagePath = os.path.join(tmpDir.name, "shader_reload.png")
        print(f"The temporary file '{screenshotImagePath}' will be used to capture screenshots")
    else:
        if not ValidateImageExtension(screenshotImagePath):
            return # Exit test suite.
    
    cameraEntityId = AdjustEditorCameraPosition(cameraEntityName)
    ClearViewportOfHelpers()

    result, iterationCount = ShaderReloadTest(iterationCountMax, screenUpdateWaitTime, captureCountOnFailure, screenshotImagePath, quickShaderOverwriteCount, quickShaderOverwriteWait)
    if result:
        print(f"ShaderReloadTest PASSED after retrying {iterationCount}/{iterationCountMax} times.")
    else:
        print(f"ShaderReloadTest FAILED after retrying {iterationCount}/{iterationCountMax} times.")
    
    if cameraEntityId is not None:
        azcamera.EditorCameraViewRequestBus(azbus.Event, "ToggleCameraAsActiveView", cameraEntityId)
    
    if tmpDir:
        tmpDir.cleanup()
if __name__ == "__main__":
    MainFunc()
