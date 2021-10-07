"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.bus
import azlmbr.atomtools
import azlmbr.materialeditor
import azlmbr.name
import azlmbr.render
import azlmbr.paths
import azlmbr.atom
import sys
import os.path
import filecmp

g_engroot = azlmbr.paths.engroot
sys.path.append(os.path.join(g_engroot, 'Tests', 'Atom', 'Automated'))

g_materialTestFolder = os.path.join(g_engroot,'Gems','Atom','TestData','TestData','Materials','StandardPbrTestCases')

# Change this to True to replace the expected screenshot images
g_replaceExpectedScreenshots = False

# This delay gives the material, model, and textures time to fully load before taking the screenshot
# [GFX TODO][ATOM-4819] Replace this with a callback mechanism that will allow the test to continue as soon as the content is fully loaded
g_defaultDelayFrames = 10

g_screenshotsTaken = 0
# Track the number of material screenshots that are not identical for a report at the end
g_materialsDontMatch = []
# Track screenshots that were not checked for equality because either the actual or expected file didn't exist
g_missingScreenshots = []
# Track screenshots operations that failed
g_failedScreenshots = []

class ScreenshotHelper:
    """
    A helper to capture screenshots and wait for them.
    """

    def __init__(self, idle_wait_frames_callback):
        super().__init__()
        self.done = False
        self.capturedScreenshot = False
        self.max_frames_to_wait = 60
        
        self.idle_wait_frames_callback = idle_wait_frames_callback

    def capture_screenshot_blocking(self, filename):
        """
        Capture a screenshot and block the execution until the screenshot has been written to the disk.
        """
        self.handler = azlmbr.atom.FrameCaptureNotificationBusHandler()
        self.handler.connect()
        self.handler.add_callback('OnCaptureFinished', self.on_screenshot_captured)

        self.done = False
        self.capturedScreenshot = False
        success = azlmbr.atom.FrameCaptureRequestBus(azlmbr.bus.Broadcast, "CaptureScreenshot", filename)
        if success:
            self.wait_until_screenshot()
            print("Screenshot taken.")
        else:
            print("screenshot failed")
        return self.capturedScreenshot

    def on_screenshot_captured(self, parameters):
        # the parameters come in as a tuple
        if parameters[0] == azlmbr.atom.FrameCaptureResult_Success:
            print("screenshot saved: {}".format(parameters[1]))
            self.capturedScreenshot = True
        else:
            print("screenshot failed: {}".format(parameters[1]))
        self.done = True
        self.handler.disconnect();

    def wait_until_screenshot(self):
        frames_waited = 0
        while self.done == False:
            self.idle_wait_frames_callback(1)
            if frames_waited > self.max_frames_to_wait:
                print("timeout while waiting for the screenshot to be written")
                self.handler.disconnect()
                break
            else:
                frames_waited = frames_waited + 1
        print("(waited {} frames)".format(frames_waited))

def ToRadians(degrees):
    return 3.14159 * degrees / 180.0;

def OpenMaterial(filename):
    documentId = azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(azlmbr.bus.Broadcast, 'OpenDocument', os.path.join(g_materialTestFolder, filename))
    return documentId
    
def CloseMaterial(documentId):
    azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(azlmbr.bus.Broadcast, 'CloseDocument', documentId)

def SelectLightingPreset(presetName):
    azlmbr.materialeditor.MaterialViewportRequestBus(azlmbr.bus.Broadcast, 'SelectLightingPresetByName', presetName)
    
def SelectModelPreset(presetName):
    azlmbr.materialeditor.MaterialViewportRequestBus(azlmbr.bus.Broadcast, 'SelectModelPresetByName', presetName)

def SetCameraDistance(distance):
    azlmbr.render.ArcBallControllerRequestBus(azlmbr.bus.Broadcast, 'SetDistance', distance)

def SetCameraHeading(heading):
    azlmbr.render.ArcBallControllerRequestBus(azlmbr.bus.Broadcast, 'SetHeading', heading)

def SetCameraPitch(pitch):
    azlmbr.render.ArcBallControllerRequestBus(azlmbr.bus.Broadcast, 'SetPitch', pitch)

def IdleFrames(numFrames):
    azlmbr.atomtools.general.idle_wait_frames(numFrames)

def CaptureScreenshot(screenshotOutputPath):
    print("Capturing screenshot to " + screenshotOutputPath + " ...")
    return ScreenshotHelper(azlmbr.atomtools.general.idle_wait_frames).capture_screenshot_blocking(screenshotOutputPath)

def ResizeViewport(width, height):
    # This locks the size of the render target to the desired resolution
    azlmbr.atomtools.AtomToolsMainWindowRequestBus(azlmbr.bus.Broadcast, 'LockViewportRenderTargetSize', width, height)
    # This resizes the window to closely match the render target resolution so it doesn't appear stretched while the script is running
    azlmbr.atomtools.AtomToolsMainWindowRequestBus(azlmbr.bus.Broadcast, 'ResizeViewportRenderTarget', width, height)

def ReleaseViewportResolutionLock():
    azlmbr.atomtools.AtomToolsMainWindowRequestBus(azlmbr.bus.Broadcast, 'UnlockViewportRenderTargetSize')

def GenerateMaterialScreenshot(materialName, 
                               uniqueSuffix="",
                               cameraHeading=-30.0,
                               cameraPitch=20.0,
                               cameraDistance=1.25,
                               lighting="Neutral Urban (Alt)",
                               model="Shader Ball",
                               delayFrames=g_defaultDelayFrames):
    """
    Opens a material, takes a screenshot in the material editor viewport, and saves the file to ppm. 
    Also sets the camera position, lighting preset, and model for the screenshot.
    The screenshots will be saved in g_materialTestFolder.
    If g_replaceExpectedScreenshots is true, it will replace the baseline "expected" screenshots. Otherwise,
    the screenshots will be saved alongside the "expected" screenshots for comparison.

    @param materialName    name of the material file to process, not including the path or ".material" extension.
    @param uniqueSuffix    optional name for this particular screenshot configuration. Used to make a unique filename 
                           when taking multiple screenshots of the same material.
    @param cameraHeading   heading of the camera in degrees.
    @param cameraPitch     pitch of the camera in degrees.
    @param cameraDistance  distance of the camera from the center point.
    @param lighting        name of the lighting configuration to use.
    @param model           name of the model configuration to use.
    @param delayFrames     number of frames to delay before taking a screenshot, so the content has time to load.
    """
    
    print("GenerateMaterialScreenshot('{}', '{}')...".format(materialName, uniqueSuffix))
    
    global g_screenshotsTaken
    global g_materialsDontMatch
    global g_missingScreenshots
    global g_failedScreenshots

    documentId = OpenMaterial(materialName + '.material')
    SelectLightingPreset(lighting)
    SelectModelPreset(model)
    SetCameraDistance(cameraDistance)
    SetCameraHeading(ToRadians(cameraHeading))
    SetCameraPitch(ToRadians(-cameraPitch))

    IdleFrames(g_defaultDelayFrames); # The UI needs to time to process the changed scene data
    
    # This delay gives the material, model, and textures time to fully load before taking the screenshot
    # [GFX TODO][ATOM-4819] Replace this with a callback mechanism that will allow the test to continue as soon as the content is fully loaded
    IdleFrames(delayFrames)

    screenshotsFolder = os.path.join(g_materialTestFolder, "Screenshots")
    uniqueFileName = materialName
    if len(uniqueSuffix) > 0:
        uniqueFileName = uniqueFileName + "." + uniqueSuffix
    # Note we use .ppm instead of .dds because more tools support it (especially BeyondCompare and ReviewBoard).
    expectedScreenshotPath = os.path.join(screenshotsFolder, uniqueFileName + ".expected.ppm")
    actualScreenshotPath = os.path.join(screenshotsFolder, uniqueFileName + ".actual.ppm")

    captureScreenshotPath = ""
    if g_replaceExpectedScreenshots:
        captureScreenshotPath = expectedScreenshotPath
    else:
        captureScreenshotPath = actualScreenshotPath

    screenshotSuccess = CaptureScreenshot(captureScreenshotPath)

    if screenshotSuccess:
        g_screenshotsTaken = g_screenshotsTaken + 1

    CloseMaterial(documentId)
    
    if not screenshotSuccess:
        g_failedScreenshots.append(captureScreenshotPath)
    elif not os.path.exists(expectedScreenshotPath):
        g_missingScreenshots.append(expectedScreenshotPath)
    elif not os.path.exists(actualScreenshotPath):
        g_missingScreenshots.append(actualScreenshotPath)
    elif not filecmp.cmp(expectedScreenshotPath, actualScreenshotPath):
        g_materialsDontMatch.append(actualScreenshotPath)


def GenerateAllMaterialScreenshots():
    """
    Takes screenshots of a list of material files and saves them in g_replaceExpectedScreenshots
    """

    # First open any material to ensure the tab bar shows up before we resize the viewport. Otherwise the First
    # screenshot might have a different size from the others.
    OpenMaterial('001_DefaultWhite.material')
    # [GFX TODO][ATOM-4909] We have to use the strange viewport size because of limitations in both the RPI and QT. The RPI doesn't provide
    # ResizeViewportRenderTarget() support on both dx12 and vulkan. And QT can't resize to specific resolutions in device-pixel units (we can
    # achieve 999x999 and 1001x1001 but not 1000x1000 for example).
    ResizeViewport(999, 999)
    IdleFrames(g_defaultDelayFrames); # Allows the UI to refresh before continuing; otherwise the viewport will appear stretched while the user waits a second for the screen capture.
    
    GenerateMaterialScreenshot('001_DefaultWhite')
    GenerateMaterialScreenshot('002_BaseColorLerp')
    GenerateMaterialScreenshot('002_BaseColorLinearLight')
    GenerateMaterialScreenshot('002_BaseColorMultiply')
    GenerateMaterialScreenshot('003_MetalMatte')
    GenerateMaterialScreenshot('003_MetalPolished')
    GenerateMaterialScreenshot('004_MetalMap')
    GenerateMaterialScreenshot('005_RoughnessMap')
    GenerateMaterialScreenshot('006_SpecularF0Map')
    GenerateMaterialScreenshot('007_MultiscatteringCompensationOff')
    GenerateMaterialScreenshot('007_MultiscatteringCompensationOn')
    GenerateMaterialScreenshot('008_NormalMap')
    GenerateMaterialScreenshot('008_NormalMap_Bevels')
    GenerateMaterialScreenshot('009_Opacity_Blended', lighting="Neutral Urban", model="Cube (Beveled)")
    GenerateMaterialScreenshot('009_Opacity_Cutout_PackedAlpha_DoubleSided', lighting="Neutral Urban", model="Cube (Beveled)")
    GenerateMaterialScreenshot('009_Opacity_Cutout_SplitAlpha_DoubleSided', lighting="Neutral Urban", model="Cube (Beveled)")
    GenerateMaterialScreenshot('009_Opacity_Cutout_SplitAlpha_SingleSided', lighting="Neutral Urban", model="Cube (Beveled)")
    GenerateMaterialScreenshot('010_AmbientOcclusion')
    GenerateMaterialScreenshot('011_Emissive')
    GenerateMaterialScreenshot('012_Parallax_POM', model="Cube", cameraHeading=-35.0, cameraPitch=35.0)
    GenerateMaterialScreenshot('013_SpecularAA_Off', lighting="Dark Test Lighting")
    GenerateMaterialScreenshot('013_SpecularAA_On', lighting="Dark Test Lighting")
    GenerateMaterialScreenshot('100_UvTiling_AmbientOcclusion')
    GenerateMaterialScreenshot('100_UvTiling_BaseColor')
    GenerateMaterialScreenshot('100_UvTiling_Emissive')
    GenerateMaterialScreenshot('100_UvTiling_Metallic')
    GenerateMaterialScreenshot('100_UvTiling_Normal')
    GenerateMaterialScreenshot('100_UvTiling_Normal_Dome_Rotate20',      model="Cube", lighting="Dark Test Lighting", cameraHeading=225.0)
    GenerateMaterialScreenshot('100_UvTiling_Normal_Dome_Rotate90',      model="Cube", lighting="Dark Test Lighting", cameraHeading=225.0)
    GenerateMaterialScreenshot('100_UvTiling_Normal_Dome_ScaleOnlyU',    model="Cube", lighting="Dark Test Lighting", cameraHeading=225.0)
    GenerateMaterialScreenshot('100_UvTiling_Normal_Dome_ScaleOnlyV',    model="Cube", lighting="Dark Test Lighting", cameraHeading=225.0)
    GenerateMaterialScreenshot('100_UvTiling_Normal_Dome_ScaleUniform',  model="Cube", lighting="Dark Test Lighting", cameraHeading=225.0)
    GenerateMaterialScreenshot('100_UvTiling_Normal_Dome_TransformAll',  model="Cube", lighting="Dark Test Lighting", cameraHeading=225.0)
    GenerateMaterialScreenshot('100_UvTiling_Opacity', lighting="Neutral Urban")
    GenerateMaterialScreenshot('100_UvTiling_Parallax_A', uniqueSuffix="Angle1", model="Cube", cameraHeading=35.0, cameraPitch=35.0)
    GenerateMaterialScreenshot('100_UvTiling_Parallax_A', uniqueSuffix="Angle2", model="Cube", cameraHeading=125.0, cameraPitch=35.0)
    GenerateMaterialScreenshot('100_UvTiling_Parallax_B', uniqueSuffix="Angle1", model="Cube", cameraHeading=0.0, cameraPitch=45.0, cameraDistance=1.0)
    GenerateMaterialScreenshot('100_UvTiling_Parallax_B', uniqueSuffix="Angle2", model="Cube", cameraHeading=90.0, cameraPitch=45.0, cameraDistance=1.0)
    GenerateMaterialScreenshot('100_UvTiling_Roughness')
    GenerateMaterialScreenshot('100_UvTiling_SpecularF0')

    ReleaseViewportResolutionLock()
    

def main():
    global g_screenshotsTaken
    global g_materialsDontMatch
    global g_missingScreenshots
    global g_failedScreenshots
    g_screenshotsTaken = 0
    g_materialsDontMatch = []
    g_missingScreenshots = []
    g_failedScreenshots = []

    print("==== Begin screenshot script ==========================================================")

    GenerateAllMaterialScreenshots()
    
    print("==== Summary Report ===================================================================")
        
    print("Screenshots taken: {}".format(g_screenshotsTaken))

    print("Screenshots failed: {}".format(len(g_failedScreenshots)))
    print(g_failedScreenshots)

    print("Missing screenshots: {}".format(len(g_missingScreenshots)))
    print(g_missingScreenshots)

    print("\n(The following stats are for informational purposes. A mismatched file doesn't necessarily mean a test failed. Mismatched files will need to be image-diffed in another tool.)")

    print("Mismatched screenshots: {}".format(len(g_materialsDontMatch)))
    print(g_materialsDontMatch)

    print("==== End screenshot script ============================================================")

if __name__ == "__main__":
    main()

