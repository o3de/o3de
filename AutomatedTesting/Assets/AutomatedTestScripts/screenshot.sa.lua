----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

g_screenshotOutputFolder = '@user@/ScriptAutomation/Screenshots/'
Print('Saving screenshots to ' .. ResolvePath(g_screenshotOutputFolder))

ResizeViewport(800, 600)

IdleFrames(100) -- wait for assets to load into the level
ExecuteConsoleCommand("r_displayInfo=0")
IdleFrames(1) -- wait 1 frame for the info text to hide
CaptureScreenshot('ScriptAutomation_CaptureScreenshotTest.png')