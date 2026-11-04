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
#include <AzFramework/Translation/TranslationDef.h>

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
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "HDR Color Grading"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Tune and apply color grading in HDR."))
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/atom-guide/features/#post-processing-effects-postfx")
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "LUT Generation"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->UIElement(AZ::Edit::UIHandlers::Button, QT_TRANSLATE_NOOP("AtomLyIntegration", "Generate LUT"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Generates a LUT from the scene's enabled color grading blend."))
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("AtomLyIntegration", "Generate LUT"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorHDRColorGradingComponent::GenerateLut)
                        ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &EditorHDRColorGradingComponent::m_generatedLutAbsolutePath, QT_TRANSLATE_NOOP("AtomLyIntegration", "Generated LUT Path"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Generated LUT Path"))
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorHDRColorGradingComponent::GetGeneratedLutVisibilitySettings)
                        ->UIElement(AZ::Edit::UIHandlers::Button, QT_TRANSLATE_NOOP("AtomLyIntegration", "Activate LUT"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Use the generated LUT asset in a Look Modification component"))
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("AtomLyIntegration", "Activate LUT"))
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorHDRColorGradingComponent::ActivateLut)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorHDRColorGradingComponent::GetGeneratedLutVisibilitySettings)
                        ;

                    editContext->Class<HDRColorGradingComponentController>(
                        "HDRColorGradingComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &HDRColorGradingComponentController::m_configuration, QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<HDRColorGradingComponentConfig>("HDRColorGradingComponentConfig", "")
                        ->DataElement(Edit::UIHandlers::CheckBox, &HDRColorGradingComponentConfig::m_enabled,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable HDR color grading"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable HDR color grading."))
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Color Adjustment"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorAdjustmentWeight, QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight of color adjustments"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingExposure, QT_TRANSLATE_NOOP("AtomLyIntegration", "Exposure"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Exposure Value"))
                            ->Attribute(Edit::Attributes::Min, AZStd::numeric_limits<float>::lowest())
                            ->Attribute(Edit::Attributes::Max, AZStd::numeric_limits<float>::max())
                            ->Attribute(Edit::Attributes::SoftMin, -20.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 20.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingContrast, QT_TRANSLATE_NOOP("AtomLyIntegration", "Contrast"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Contrast Value"))
                            ->Attribute(Edit::Attributes::Min, -100.0f)
                            ->Attribute(Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingPreSaturation, QT_TRANSLATE_NOOP("AtomLyIntegration", "Pre Saturation"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Pre Saturation Value"))
                            ->Attribute(Edit::Attributes::Min, -100.0f)
                            ->Attribute(Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingFilterIntensity, QT_TRANSLATE_NOOP("AtomLyIntegration", "Filter Intensity"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Filter Intensity Value"))
                            ->Attribute(Edit::Attributes::Min, AZStd::numeric_limits<float>::lowest())
                            ->Attribute(Edit::Attributes::Max, AZStd::numeric_limits<float>::max())
                            ->Attribute(Edit::Attributes::SoftMin, -1.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingFilterMultiply, QT_TRANSLATE_NOOP("AtomLyIntegration", "Filter Multiply"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Filter Multiply Value"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &HDRColorGradingComponentConfig::m_colorFilterSwatch, QT_TRANSLATE_NOOP("AtomLyIntegration", "Filter Swatch"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Color Filter Swatch Value"))

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "White Balance"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_whiteBalanceWeight, QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight of white balance"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_whiteBalanceKelvin, QT_TRANSLATE_NOOP("AtomLyIntegration", "Temperature"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Temperature in Kelvin"))
                            ->Attribute(Edit::Attributes::Min, 1000.0f)
                            ->Attribute(Edit::Attributes::Max, 40000.0f)
                            ->Attribute(AZ::Edit::Attributes::SliderCurveMidpoint, 0.165f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_whiteBalanceTint, QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint Value"))
                            ->Attribute(Edit::Attributes::Min, -100.0f)
                            ->Attribute(Edit::Attributes::Max, 100.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_whiteBalanceLuminancePreservation, QT_TRANSLATE_NOOP("AtomLyIntegration", "Luminance Preservation"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Modulate the preservation of luminance"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Split Toning"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_splitToneWeight, QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Modulates the split toning effect."))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_splitToneBalance, QT_TRANSLATE_NOOP("AtomLyIntegration", "Balance"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Split Tone Balance Value"))
                            ->Attribute(Edit::Attributes::Min, -1.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &HDRColorGradingComponentConfig::m_splitToneShadowsColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadows Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Split Tone Shadows Color"))
                        ->DataElement(AZ::Edit::UIHandlers::Color, &HDRColorGradingComponentConfig::m_splitToneHighlightsColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Highlights Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Split Tone Highlights Color"))

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &HDRColorGradingComponentConfig::m_channelMixingRed, QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing Red"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing Red Value"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &HDRColorGradingComponentConfig::m_channelMixingGreen, QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing Green"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing Green Value"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &HDRColorGradingComponentConfig::m_channelMixingBlue, QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing Blue"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Channel Mixing Blue Value"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadow Midtones Highlights"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_smhWeight, QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Modulates the SMH effect."))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_smhShadowsStart, QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadows Start"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Shadows Start Value"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 16.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 2.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_smhShadowsEnd, QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadows End"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Shadows End Value"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 16.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 2.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_smhHighlightsStart, QT_TRANSLATE_NOOP("AtomLyIntegration", "Highlights Start"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Highlights Start Value"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 16.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 2.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_smhHighlightsEnd, QT_TRANSLATE_NOOP("AtomLyIntegration", "Highlights End"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Highlights End Value"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 16.0f)
                            ->Attribute(Edit::Attributes::SoftMax, 2.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Color, &HDRColorGradingComponentConfig::m_smhShadowsColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Shadows Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Shadows Color"))
                        ->DataElement(AZ::Edit::UIHandlers::Color, &HDRColorGradingComponentConfig::m_smhMidtonesColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Midtones Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Midtones Color"))
                        ->DataElement(AZ::Edit::UIHandlers::Color, &HDRColorGradingComponentConfig::m_smhHighlightsColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Highlights Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "SMH Highlights Color"))

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Final Adjustment"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_finalAdjustmentWeight, QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Weight of final adjustments"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingHueShift, QT_TRANSLATE_NOOP("AtomLyIntegration", "Hue Shift"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Hue Shift Value"))
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &HDRColorGradingComponentConfig::m_colorGradingPostSaturation, QT_TRANSLATE_NOOP("AtomLyIntegration", "Post Saturation"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Post Saturation Value"))
                            ->Attribute(Edit::Attributes::Min, -100.0f)
                            ->Attribute(Edit::Attributes::Max, 100.0f)

                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "LUT Generation"))
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &HDRColorGradingComponentConfig::m_lutResolution, QT_TRANSLATE_NOOP("AtomLyIntegration", "LUT Resolution"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Resolution of generated LUT"))
                            ->EnumAttribute(LutResolution::Lut16x16x16, QT_TRANSLATE_NOOP("AtomLyIntegration", "16x16x16"))
                            ->EnumAttribute(LutResolution::Lut32x32x32, QT_TRANSLATE_NOOP("AtomLyIntegration", "32x32x32"))
                            ->EnumAttribute(LutResolution::Lut64x64x64, QT_TRANSLATE_NOOP("AtomLyIntegration", "64x64x64"))
                        ->DataElement(Edit::UIHandlers::ComboBox, &HDRColorGradingComponentConfig::m_shaperPresetType,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Shaper Type"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Shaper Type."))
                            ->EnumAttribute(ShaperPresetType::None, QT_TRANSLATE_NOOP("AtomLyIntegration", "None"))
                            ->EnumAttribute(ShaperPresetType::LinearCustomRange, QT_TRANSLATE_NOOP("AtomLyIntegration", "Linear Custom Range"))
                            ->EnumAttribute(ShaperPresetType::Log2_48Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 48 nits"))
                            ->EnumAttribute(ShaperPresetType::Log2_1000Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 1000 nits"))
                            ->EnumAttribute(ShaperPresetType::Log2_2000Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 2000 nits"))
                            ->EnumAttribute(ShaperPresetType::Log2_4000Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 4000 nits"))
                            ->EnumAttribute(ShaperPresetType::Log2CustomRange, QT_TRANSLATE_NOOP("AtomLyIntegration", "Log2 Custom Range"))
                            ->EnumAttribute(ShaperPresetType::PqSmpteSt2084, QT_TRANSLATE_NOOP("AtomLyIntegration", "PQ (SMPTE ST 2084)"))
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

            AZ::Render::FrameCaptureOutcome captureOutcome;
            AZ::Render::FrameCaptureRequestBus::BroadcastResult(
                captureOutcome,
                &AZ::Render::FrameCaptureRequestBus::Events::CapturePassAttachment,
                m_currentTiffFilePath,
                LutGenerationPassHierarchy,
                AZStd::string(LutAttachment),
                AZ::RPI::PassAttachmentReadbackOption::Output);

            if (captureOutcome.IsSuccess())
            {
                AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect(captureOutcome.GetValue());
                AZ::TickBus::Handler::BusDisconnect();
            }

            AZ_Error(
                "EditorHDRColorGradingComponent",
                captureOutcome.IsSuccess(),
                "Frame capture initialization failed. %s",
                captureOutcome.GetError().m_errorMessage.c_str());
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
            InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);

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
