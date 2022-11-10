/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/ColorGrading/EditorHDRColorGradingComponent.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <Atom/RPI.Public/ViewportContextManager.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Base.h>

namespace AZ
{
    namespace Render
    {
        namespace Internal
        {
            struct EditorHDRColorGradingNotificationBusHandler final
                : public EditorHDRColorGradingNotificationBus::Handler
                , public AZ::BehaviorEBusHandler
            {
                AZ_EBUS_BEHAVIOR_BINDER(
                    EditorHDRColorGradingNotificationBusHandler, "{61FFB210-C2F9-4A82-9088-4C974C3E0EE7}", AZ::SystemAllocator
                        , OnGenerateLutCompleted, OnActivateLutCompleted);

                void OnGenerateLutCompleted(const AZStd::string& lutAssetAbsolutePath) override
                {
                    Call(FN_OnGenerateLutCompleted, lutAssetAbsolutePath);
                }

                void OnActivateLutCompleted() override
                {
                    Call(FN_OnActivateLutCompleted);
                }
            };
        }

        void EditorHDRColorGradingComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorHDRColorGradingComponent, BaseClass>()
                    ->Version(2)
                    ->Field("generatedLut", &EditorHDRColorGradingComponent::m_generatedLutAbsolutePath)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorHDRColorGradingComponent>(
                        "HDR Color Grading", "Tune and apply color grading in HDR.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/atom-guide/features/#post-processing-effects-postfx")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "LUT Generation")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->UIElement(AZ::Edit::UIHandlers::Button, "Generate LUT", "Generates a LUT from the scene's enabled color grading blend.")
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Generate LUT")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorHDRColorGradingComponent::GenerateLut)
                        ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &EditorHDRColorGradingComponent::m_generatedLutAbsolutePath, "Generated LUT Path", "Generated LUT Path")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorHDRColorGradingComponent::GetGeneratedLutVisibilitySettings)
                        ->UIElement(AZ::Edit::UIHandlers::Button, "Activate LUT", "Use the generated LUT asset in a Look Modification component")
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Activate LUT")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorHDRColorGradingComponent::ActivateLut)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorHDRColorGradingComponent::GetGeneratedLutVisibilitySettings)
                        ;

                    editContext->Class<HDRColorGradingComponentController>(
                        "HDRColorGradingComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &HDRColorGradingComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<HDRColorGradingComponentConfig>("HDRColorGradingComponentConfig", "")
                        ->DataElement(Edit::UIHandlers::CheckBox, &HDRColorGradingComponentConfig::m_enabled,
                            "Enable HDR color grading",
                            "Enable HDR color grading.")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Color Adjustment")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorAdjustmentWeight, "Weight", "Weight of color adjustments")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingExposure, "Exposure", "Exposure Value")
                            ->Attribute(Edit::Attributes::Min, AZStd::numeric_limits<float>::lowest())
                            ->Attribute(Edit::Attributes::Max, AZStd::numeric_limits<float>::max())
                            ->Attribute(Edit::Attributes::SoftMin, -20.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 20.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingContrast, "Contrast", "Contrast Value")
                            ->Attribute(Edit::Attributes::Min, -100.0f)
                            ->Attribute(Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingPreSaturation, "Pre Saturation", "Pre Saturation Value")
                            ->Attribute(Edit::Attributes::Min, -100.0f)
                            ->Attribute(Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingFilterIntensity, "Filter Intensity", "Filter Intensity Value")
                            ->Attribute(Edit::Attributes::Min, AZStd::numeric_limits<float>::lowest())
                            ->Attribute(Edit::Attributes::Max, AZStd::numeric_limits<float>::max())
                            ->Attribute(Edit::Attributes::SoftMin, -1.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingFilterMultiply, "Filter Multiply", "Filter Multiply Value")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &HDRColorGradingComponentConfig::m_colorFilterSwatch, "Filter Swatch", "Color Filter Swatch Value")

                        ->ClassElement(AZ::Edit::ClassElements::Group, "White Balance")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_whiteBalanceWeight, "Weight", "Weight of white balance")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_whiteBalanceKelvin, "Temperature", "Temperature in Kelvin")
                            ->Attribute(Edit::Attributes::Min, 1000.0f)
                            ->Attribute(Edit::Attributes::Max, 40000.0f)
                            ->Attribute(AZ::Edit::Attributes::SliderCurveMidpoint, 0.165f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_whiteBalanceTint, "Tint", "Tint Value")
                            ->Attribute(Edit::Attributes::Min, -100.0f)
                            ->Attribute(Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_whiteBalanceLuminancePreservation, "Luminance Preservation", "Modulate the preservation of luminance")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Split Toning")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_splitToneWeight, "Weight", "Modulates the split toning effect.")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_splitToneBalance, "Balance", "Split Tone Balance Value")
                            ->Attribute(Edit::Attributes::Min, -1.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &HDRColorGradingComponentConfig::m_splitToneShadowsColor, "Shadows Color", "Split Tone Shadows Color")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &HDRColorGradingComponentConfig::m_splitToneHighlightsColor, "Highlights Color", "Split Tone Highlights Color")

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Channel Mixing")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_channelMixingRed, "Channel Mixing Red", "Channel Mixing Red Value")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_channelMixingGreen, "Channel Mixing Green", "Channel Mixing Green Value")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_channelMixingBlue, "Channel Mixing Blue", "Channel Mixing Blue Value")
                            ->Attribute(Edit::Attributes::Min, 0.0f)

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Shadow Midtones Highlights")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_smhWeight, "Weight", "Modulates the SMH effect.")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_smhShadowsStart, "Shadows Start", "SMH Shadows Start Value")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 16.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 2.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_smhShadowsEnd, "Shadows End", "SMH Shadows End Value")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 16.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 2.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_smhHighlightsStart, "Highlights Start", "SMH Highlights Start Value")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 16.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 2.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_smhHighlightsEnd, "Highlights End", "SMH Highlights End Value")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 16.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 2.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &HDRColorGradingComponentConfig::m_smhShadowsColor, "Shadows Color", "SMH Shadows Color")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &HDRColorGradingComponentConfig::m_smhMidtonesColor, "Midtones Color", "SMH Midtones Color")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &HDRColorGradingComponentConfig::m_smhHighlightsColor, "Highlights Color", "SMH Highlights Color")

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Final Adjustment")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_finalAdjustmentWeight, "Weight", "Weight of final adjustments")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingHueShift, "Hue Shift", "Hue Shift Value")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingPostSaturation, "Post Saturation", "Post Saturation Value")
                            ->Attribute(Edit::Attributes::Min, -100.0f)
                            ->Attribute(Edit::Attributes::Max, 100.0f)

                        ->ClassElement(AZ::Edit::ClassElements::Group, "LUT Generation")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &HDRColorGradingComponentConfig::m_lutResolution, "LUT Resolution", "Resolution of generated LUT")
                            ->EnumAttribute(LutResolution::Lut16x16x16, "16x16x16")
                            ->EnumAttribute(LutResolution::Lut32x32x32, "32x32x32")
                            ->EnumAttribute(LutResolution::Lut64x64x64, "64x64x64")
                        ->DataElement(Edit::UIHandlers::ComboBox, &HDRColorGradingComponentConfig::m_shaperPresetType,
                            "Shaper Type", "Shaper Type.")
                            ->EnumAttribute(ShaperPresetType::None, "None")
                            ->EnumAttribute(ShaperPresetType::LinearCustomRange, "Linear Custom Range")
                            ->EnumAttribute(ShaperPresetType::Log2_48Nits, "Log2 48 nits")
                            ->EnumAttribute(ShaperPresetType::Log2_1000Nits, "Log2 1000 nits")
                            ->EnumAttribute(ShaperPresetType::Log2_2000Nits, "Log2 2000 nits")
                            ->EnumAttribute(ShaperPresetType::Log2_4000Nits, "Log2 4000 nits")
                            ->EnumAttribute(ShaperPresetType::Log2CustomRange, "Log2 Custom Range")
                            ->EnumAttribute(ShaperPresetType::PqSmpteSt2084, "PQ (SMPTE ST 2084)")
                        ;
                }
            }

            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<EditorHDRColorGradingRequestBus>("EditorHDRColorGradingRequestBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Event("GenerateLutAsync", &EditorHDRColorGradingRequests::GenerateLutAsync)
                    ->Event("ActivateLutAsync", &EditorHDRColorGradingRequests::ActivateLutAsync)
                    ;

                behaviorContext->EBus<EditorHDRColorGradingNotificationBus>("EditorHDRColorGradingNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Handler<Internal::EditorHDRColorGradingNotificationBusHandler>()
                    ->Event("OnGenerateLutCompleted", &EditorHDRColorGradingNotificationBus::Events::OnGenerateLutCompleted)
                    ->Event("OnActivateLutCompleted", &EditorHDRColorGradingNotificationBus::Events::OnActivateLutCompleted)
                    ;
            }
        }

        EditorHDRColorGradingComponent::EditorHDRColorGradingComponent(const HDRColorGradingComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorHDRColorGradingComponent::Activate()
        {
            BaseClass::Activate();
            EditorHDRColorGradingRequestBus::Handler::BusConnect(GetEntityId());
        }

        void EditorHDRColorGradingComponent::Deactivate()
        {
            BaseClass::Deactivate();
            EditorHDRColorGradingRequestBus::Handler::BusDisconnect();
        }

        void EditorHDRColorGradingComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            if (m_waitOneFrame)
            {
                m_waitOneFrame = false;
                return;
            }

            const char* LutAttachment = "LutOutput";
            auto currentPipeline =
                AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get()->GetDefaultViewportContext()->GetCurrentPipeline();
            if (!currentPipeline)
            {
                return;
            }
            auto renderPipelineName = currentPipeline->GetId();
            const AZStd::vector<AZStd::string> LutGenerationPassHierarchy{
                renderPipelineName.GetCStr(),
                "LutGenerationPass"
            };

            char resolvedOutputFilePath[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(m_currentTiffFilePath.c_str(), resolvedOutputFilePath, AZ_MAX_PATH_LEN);

            AZStd::string lutGenerationCacheFolder;
            AzFramework::StringFunc::Path::GetFolderPath(resolvedOutputFilePath, lutGenerationCacheFolder);
            AZ::IO::SystemFile::CreateDir(lutGenerationCacheFolder.c_str());

            AZ::Render::FrameCaptureId frameCaptureId = AZ::Render::InvalidFrameCaptureId;
            AZ::Render::FrameCaptureRequestBus::BroadcastResult(
                frameCaptureId,
                &AZ::Render::FrameCaptureRequestBus::Events::CapturePassAttachment,
                LutGenerationPassHierarchy,
                AZStd::string(LutAttachment),
                m_currentTiffFilePath,
                AZ::RPI::PassAttachmentReadbackOption::Output);

            if (frameCaptureId != AZ::Render::InvalidFrameCaptureId)
            {
                AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect(frameCaptureId);
                AZ::TickBus::Handler::BusDisconnect();
            }
        }

        void EditorHDRColorGradingComponent::OnFrameCaptureFinished([[maybe_unused]] AZ::Render::FrameCaptureResult result, [[maybe_unused]]const AZStd::string& info)
        {
            AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();

            char resolvedInputFilePath[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(m_currentTiffFilePath.c_str(), resolvedInputFilePath, AZ_MAX_PATH_LEN);
            char resolvedOutputFilePath[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(m_currentLutFilePath.c_str(), resolvedOutputFilePath, AZ_MAX_PATH_LEN);

            AZStd::string lutGenerationFolder;
            AzFramework::StringFunc::Path::GetFolderPath(resolvedOutputFilePath, lutGenerationFolder);
            AZ::IO::SystemFile::CreateDir(lutGenerationFolder.c_str());

            AZStd::vector<AZStd::string_view> pythonArgs
            {
                "--i", resolvedInputFilePath,
                "--o", resolvedOutputFilePath
            };

            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs,
                TiffToAzassetPythonScriptPath,
                pythonArgs);

            m_controller.m_configuration.m_generateLut = false;
            m_controller.OnConfigChanged();

            m_generatedLutAbsolutePath = resolvedOutputFilePath + AZStd::string(".azasset");
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
                AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);

            EditorHDRColorGradingNotificationBus::Event(GetEntityId(), &EditorHDRColorGradingNotificationBus::Handler::OnGenerateLutCompleted, m_generatedLutAbsolutePath);
        }

        void EditorHDRColorGradingComponent::GenerateLut()
        {
            // turn on lut generation pass
            AZ::Uuid uuid = AZ::Uuid::CreateRandom();
            AZStd::string uuidString;
            uuid.ToString(uuidString);

            m_currentTiffFilePath = AZStd::string::format(TempTiffFilePath, uuidString.c_str());
            m_currentLutFilePath = "@projectroot@/" + AZStd::string::format(GeneratedLutRelativePath, uuidString.c_str());

            m_controller.SetGenerateLut(true);
            m_controller.OnConfigChanged();

            m_waitOneFrame = true;

            AZ::TickBus::Handler::BusConnect();
        }

        AZ::u32 EditorHDRColorGradingComponent::ActivateLut()
        {
            using namespace AzFramework::StringFunc::Path;

            AZStd::string entityName;
            AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, GetEntityId());

            AZStd::string filename;
            GetFileName(m_generatedLutAbsolutePath.c_str(), filename);
            AZStd::string assetRelativePath = "LutGeneration/" + filename + ".azasset";
            AZStd::vector<AZStd::string_view> pythonArgs
            {
                "--entityName", entityName,
                "--assetRelativePath", assetRelativePath
            };

            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs,
                ActivateLutAssetPythonScriptPath,
                pythonArgs);

            // Remark, when LUT activation is complete, a notification should be sent
            // via EditorHDRColorGradingNotificationBus::OnActivateLutCompleted, but the completion will occur
            // inside the python script @ActivateLutAssetPythonScriptPath, so the
            // responsibility to send this notification is on @ActivateLutAssetPythonScriptPath.

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        bool EditorHDRColorGradingComponent::GetGeneratedLutVisibilitySettings()
        {
            return !m_generatedLutAbsolutePath.empty();
        }

        u32 EditorHDRColorGradingComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        //! EditorHDRColorGradingRequestBus overrides...
        void EditorHDRColorGradingComponent::GenerateLutAsync()
        {
            GenerateLut();
        }

        void EditorHDRColorGradingComponent::ActivateLutAsync()
        {
            ActivateLut();
        }
        ////////////////////////////////////////////////

    } // namespace Render
} // namespace AZ
