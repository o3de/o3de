/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <PostProcess/ColorGrading/HDRColorGradingComponent.h>

namespace AZ
{
    namespace Render
    {
        static constexpr const char* const TempTiffFilePath{ "@usercache@/LutGeneration/SavedLut_%s.tiff" };
        static constexpr const char* const GeneratedLutRelativePath = { "LutGeneration/SavedLut_%s" };
        static constexpr const char* const TiffToAzassetPythonScriptPath{ "@engroot@/Gems/Atom/Feature/Common/Editor/Scripts/ColorGrading/tiff_to_3dl_azasset.py" };
        static constexpr const char* const ActivateLutAssetPythonScriptPath{ "@engroot@/Gems/Atom/Feature/Common/Editor/Scripts/ColorGrading/activate_lut_asset.py" };

        class EditorHDRColorGradingComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<HDRColorGradingComponentController, HDRColorGradingComponent, HDRColorGradingComponentConfig>
            , private TickBus::Handler
            , private FrameCaptureNotificationBus::Handler
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<HDRColorGradingComponentController, HDRColorGradingComponent, HDRColorGradingComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorHDRColorGradingComponent, "{C1FAB0B1-5847-4533-B08E-7314AC807B8E}", BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorHDRColorGradingComponent() = default;
            EditorHDRColorGradingComponent(const HDRColorGradingComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;

        private:
            // AZ::TickBus overrides ...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // FrameCaptureNotificationBus overrides ...
            void OnCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string& info) override;

            void GenerateLut();
            AZ::u32 ActivateLut();
            bool GetGeneratedLutVisibilitySettings();

            bool m_waitOneFrame = false;
            AZStd::string m_currentTiffFilePath;
            AZStd::string m_currentLutFilePath;

            AZStd::string m_generatedLutAbsolutePath;
        };
    } // namespace Render
} // namespace AZ
