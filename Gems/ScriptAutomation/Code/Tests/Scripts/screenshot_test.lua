----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
----------------------------------------------------------------------------------------------------

ExecuteConsoleCommand("LoadLevel levels/defaultlevel/defaultlevel.spawnable")
IdleSeconds(5)

g_screenshotOutputFolder = '@user@/Scripts/Screenshots/'
testEnv = GetRenderApiName()

SetScreenshotFolder(g_screenshotOutputFolder)
SetTestEnvPath(testEnv)

SetOfficialBaselineImageFolder(g_screenshotOutputFolder)
SetLocalBaselineImageFolder(g_screenshotOutputFolder)

CaptureScreenshot("screenshot_test.png")

-- compare to itself as an example
CompareScreenshots(g_screenshotOutputFolder .. testEnv .. "/screenshot_test.png", g_screenshotOutputFolder .. testEnv .. "/screenshot_test.png", 0.01)
