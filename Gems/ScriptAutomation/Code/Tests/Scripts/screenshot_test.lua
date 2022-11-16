----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
----------------------------------------------------------------------------------------------------

ExecuteConsoleCommand("LoadLevel levels/defaultlevel/defaultlevel.spawnable")
IdleSeconds(5) -- wait for the level to load

g_screenshotOutputFolder = '@user@/Scripts/Screenshots/'
testEnv = GetRenderApiName()

SetScreenshotFolder(g_screenshotOutputFolder)
SetTestEnvPath(testEnv)

Print("Saving screenshots to " .. ResolvePath(g_screenshotOutputFolder .. testEnv))

SetOfficialBaselineImageFolder(g_screenshotOutputFolder)
SetLocalBaselineImageFolder(g_screenshotOutputFolder)

ExecuteConsoleCommand("r_displayInfo=0")
IdleFrames(1) -- wait 1 frame for the info text to hide

CaptureScreenshot("screenshot_test.png")

-- compare to itself as an example
CompareScreenshots(g_screenshotOutputFolder .. testEnv .. "/screenshot_test.png", g_screenshotOutputFolder .. testEnv .. "/screenshot_test.png", 0.01)
