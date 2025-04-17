/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CubeMapCapture/EditorCubeMapCaptureComponent.h>

namespace AZ
{
    namespace Render
    {
        void EditorCubeMapCaptureComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorCubeMapCaptureComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorCubeMapCaptureComponent>(
                        "CubeMap Capture", "The CubeMap Capture component captures a specular or diffuse cubemap at a specific position in the level")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Lighting")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->UIElement(AZ::Edit::UIHandlers::Button, "Capture CubeMap", "Capture CubeMap")
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Capture CubeMap")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCubeMapCaptureComponent::CaptureCubeMap)
                        ;

                    editContext->Class<CubeMapCaptureComponentController>(
                        "CubeMapCaptureComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &CubeMapCaptureComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<CubeMapCaptureComponentConfig>(
                        "CubeMapCaptureComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &CubeMapCaptureComponentConfig::m_exposure, "Exposure", "Exposure to use when capturing the cubemap")
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -16.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 16.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &CubeMapCaptureComponentConfig::m_captureType, "Capture Type", "The type of cubemap to capture")
                            ->EnumAttribute(CubeMapCaptureType::Specular, "Specular IBL")
                            ->EnumAttribute(CubeMapCaptureType::Diffuse, "Diffuse IBL")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CubeMapCaptureComponentConfig::OnCaptureTypeChanged)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &CubeMapCaptureComponentConfig::m_specularQualityLevel, "Specular IBL CubeMap Quality", "Resolution of the Specular IBL cubemap")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &CubeMapCaptureComponentConfig::GetSpecularQualityVisibilitySetting)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CubeMapCaptureComponentConfig::OnSpecularQualityChanged)
                            ->EnumAttribute(CubeMapSpecularQualityLevel::VeryLow, "Very Low")
                            ->EnumAttribute(CubeMapSpecularQualityLevel::Low, "Low")
                            ->EnumAttribute(CubeMapSpecularQualityLevel::Medium, "Medium")
                            ->EnumAttribute(CubeMapSpecularQualityLevel::High, "High")
                            ->EnumAttribute(CubeMapSpecularQualityLevel::VeryHigh, "Very High")
                        ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &CubeMapCaptureComponentConfig::m_relativePath, "CubeMap Path", "CubeMap Path")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->EBus<EditorCubeMapCaptureBus>("EditorCubeMapCaptureBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Event("CaptureCubeMap", &EditorCubeMapCaptureInterface::CaptureCubeMap)
                    ;
                
                behaviorContext->ConstantProperty("EditorCubeMapCaptureComponentTypeId", BehaviorConstant(Uuid(EditorCubeMapCaptureComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorCubeMapCaptureComponent::EditorCubeMapCaptureComponent(const CubeMapCaptureComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorCubeMapCaptureComponent::Activate()
        {
            BaseClass::Activate();
            EditorCubeMapCaptureBus::Handler::BusConnect(GetEntityId());
        }

        void EditorCubeMapCaptureComponent::Deactivate()
        {
            EditorCubeMapCaptureBus::Handler::BusDisconnect(GetEntityId());
            BaseClass::Deactivate();
        }

        AZ::u32 EditorCubeMapCaptureComponent::CaptureCubeMap()
        {
            CubeMapCaptureComponentConfig& configuration = m_controller.m_configuration;

            AzToolsFramework::ScopedUndoBatch undoBatch("CubeMap Render");
            SetDirty();

            return RenderCubeMap(
                [&](RenderCubeMapCallback callback, AZStd::string& relativePath) { m_controller.RenderCubeMap(callback, relativePath); },
                "Capturing Cubemap...",
                GetEntity(),
                "CubeMapCaptures",
                configuration.m_relativePath,
                configuration.m_captureType,
                configuration.m_specularQualityLevel);
        }
    } // namespace Render
} // namespace AZ
