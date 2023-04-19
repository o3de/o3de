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
g_renderApiName = GetRenderApiName()
g_platformName = GetPlatformName()
g_renderPipelineName = GetRenderPipelineName();

g_screenshotOutputFolder = '@user@/AutomationScripts/CapturedScreenshots/'
g_testEnv = g_renderPipelineName .. "/" .. g_platformName .. "/" .. g_renderApiName

g_imperceptibleImageDiffLevel = 0.01

SetScreenshotFolder(g_screenshotOutputFolder)
SetTestEnvPath(g_testEnv)
SetOfficialBaselineImageFolder('@projectroot@/AutomationScripts/ExpectedScreenshots/')
