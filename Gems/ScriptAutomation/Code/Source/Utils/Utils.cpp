/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Utils/Utils.h>

#include <AtomCore/Instance/InstanceDatabase.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/Utils/DdsFile.h>

#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>


namespace ScriptAutomation
{
    namespace Utils
    {
        bool SupportsResizeClientAreaOfDefaultWindow()
        {
            return AzFramework::NativeWindow::SupportsClientAreaResizeOfDefaultWindow();
        }

        void ResizeClientArea(uint32_t width, uint32_t height)
        {
            AzFramework::NativeWindowHandle windowHandle = nullptr;
            AzFramework::WindowSystemRequestBus::BroadcastResult(
                windowHandle,
                &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);

            AzFramework::WindowSize clientAreaSize = {width, height};
            AzFramework::WindowRequestBus::Event(windowHandle, &AzFramework::WindowRequestBus::Events::ResizeClientArea, clientAreaSize);
            AzFramework::WindowSize newWindowSize;
            AzFramework::WindowRequestBus::EventResult(newWindowSize, windowHandle, &AzFramework::WindowRequests::GetClientAreaSize);
            AZ_Error("ResizeClientArea", newWindowSize.m_width == width && newWindowSize.m_height == height,
                "Requested window resize to %ux%u but got %ux%u. This display resolution is too low or desktop scaling is too high.",
                width, height, newWindowSize.m_width, newWindowSize.m_height);
        }

        bool SupportsToggleFullScreenOfDefaultWindow()
        {
            return AzFramework::NativeWindow::CanToggleFullScreenStateOfDefaultWindow();
        }

        void ToggleFullScreenOfDefaultWindow()
        {
            AzFramework::NativeWindow::ToggleFullScreenStateOfDefaultWindow();
        }

        AZStd::string GetScreenshotsPath(bool resolvePath)
        {
            AZStd::string path = "@user@/scriptautomation/screenshots/";

            if (resolvePath)
            {
                path = Utils::ResolvePath(path);
            }

            return path;
        }

        AZStd::string GetProfilingPath(bool resolvePath)
        {
            AZStd::string path = "@user@/scriptautomation/profiling/";

            if (resolvePath)
            {
                path = Utils::ResolvePath(path);
            }

            return path;
        }


        AZStd::string ResolvePath(const AZStd::string& path)
        {
            char resolvedPath[AZ_MAX_PATH_LEN] = {0};
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(path.c_str(), resolvedPath, AZ_MAX_PATH_LEN);
            return resolvedPath;
        }

        bool IsFileUnderFolder(AZStd::string filePath, AZStd::string folder)
        {
            AzFramework::StringFunc::Path::Normalize(filePath);
            AzFramework::StringFunc::Path::Normalize(folder);

            AZStd::to_lower(filePath.begin(), filePath.end());
            AZStd::to_lower(folder.begin(), folder.end());
            auto relativePath = AZ::IO::FixedMaxPath(filePath.c_str()).LexicallyRelative(folder.c_str());
            if (!relativePath.empty() && !relativePath.Native().starts_with(".."))
            {
                return true;
            }
            return false;
        }

    } // namespace Utils
} // namespace ScriptAutomation
