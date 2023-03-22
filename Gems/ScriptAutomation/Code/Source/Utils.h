/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Windowing/WindowBus.h>

#include <Atom/RHI/Device.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>

// Some versions of Visual Studio have a bug that prevents debugging static member.
// https://developercommunity.visualstudio.com/content/problem/25756/cannot-view-static-member-variables-in-debugger.html
#define AZ_DEBUG_STATIC_MEMEBER(localVariable, staticMember) AZStd::add_lvalue_reference<decltype(staticMember)>::type localVariable = staticMember; AZ_UNUSED(localVariable);

namespace ScriptAutomation
{
    namespace Utils
    {
        AZ::RHI::Ptr<AZ::RHI::Device> GetRHIDevice();

        struct AssetEntry
        {
            AZStd::string m_path;
            AZ::Data::AssetId m_assetId;
            AZStd::string m_name;
        };

        /**
         * Helper for ImGui
         * Used to help display a ListBox of AssetEntries
         */
        bool AssetEntryNameGetter(void* data, int index, const char** outName);

        void ToggleRadTMCapture();
        
        class DefaultIBL
        {
        public:
            DefaultIBL() = default;
            ~DefaultIBL();
            void PreloadAssets();
            void Init(AZ::RPI::Scene* scene);
            void SetExposure(float exposure);
            void Reset();
        private:
            AZ::Render::ImageBasedLightFeatureProcessorInterface* m_featureProcessor = nullptr;

            AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_diffuseImageAsset;
            AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_specularImageAsset;
        };

        bool SupportsResizeClientAreaOfDefaultWindow();
        void ResizeClientArea(uint32_t width, uint32_t height, const AzFramework::WindowPosOptions& options);

        bool SupportsToggleFullScreenOfDefaultWindow();
        void ToggleFullScreenOfDefaultWindow();

        AZ::IO::FixedMaxPath GetProfilingPath(bool resolvePath);
        AZStd::string ResolvePath(const AZStd::string& path);

        bool RunDiffTool(const AZStd::string& filePathA, const AZStd::string& filePathB);

        AZ::Data::Instance<AZ::RPI::StreamingImage> GetSolidColorCubemap(uint32_t color);

        //! Returns true if the file resides within a folder
        bool IsFileUnderFolder(AZStd::string filePath, AZStd::string folder);

    } // namespace Utils
} // namespace ScriptAutomation
