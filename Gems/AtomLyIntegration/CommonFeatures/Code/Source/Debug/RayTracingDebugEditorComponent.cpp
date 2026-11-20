/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Debug/RayTracingDebugEditorComponent.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace AZ::Render
{
    void RayTracingDebugEditorComponent::Reflect(ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto* serializeContext{ azrtti_cast<SerializeContext*>(context) })
        {
            // clang-format off
            serializeContext->Class<RayTracingDebugEditorComponent, BaseClass>()
                ->Version(0)
            ;
            // clang-format on

            if (auto* editContext{ serializeContext->GetEditContext() })
            {
                // clang-format off
                editContext->Class<RayTracingDebugEditorComponent>(QT_TRANSLATE_NOOP("AtomLyIntegration", "Debug Ray Tracing"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls for debugging ray tracing."))
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Graphics/Debugging")
                        ->Attribute(Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Level"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                ;

                editContext->Class<RayTracingDebugComponentController>("RayTracingDebugComponentController", "")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ->DataElement(Edit::UIHandlers::Default, &RayTracingDebugComponentController::m_configuration, QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                        ->Attribute(Edit::Attributes::Visibility, &RayTracingDebugComponentController::IsRayTracingSupported)
                    ->UIElement(Edit::UIHandlers::LineEdit, QT_TRANSLATE_NOOP("AtomLyIntegration", "Ray Tracing is not supported on this device."))
                        ->Attribute(Edit::Attributes::Visibility, &RayTracingDebugComponentController::IsRayTracingNotSupported)
                ;

                editContext->Class<RayTracingDebugComponentConfig>("RayTracingDebugComponentConfig", "")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                    ->DataElement(Edit::UIHandlers::CheckBox, &RayTracingDebugComponentConfig::m_enabled, QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Ray Tracing Debugging"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Ray Tracing Debugging."))
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(Edit::UIHandlers::ComboBox, &RayTracingDebugComponentConfig::m_debugViewMode, QT_TRANSLATE_NOOP("AtomLyIntegration", "View mode"), QT_TRANSLATE_NOOP("AtomLyIntegration", "What property to output to the view"))
                        ->EnumAttribute(RayTracingDebugViewMode::InstanceIndex, QT_TRANSLATE_NOOP("AtomLyIntegration", "Instance Index"))
                        ->EnumAttribute(RayTracingDebugViewMode::InstanceID, QT_TRANSLATE_NOOP("AtomLyIntegration", "Instance ID"))
                        ->EnumAttribute(RayTracingDebugViewMode::ClusterID, QT_TRANSLATE_NOOP("AtomLyIntegration", "Cluster ID"))
                        ->EnumAttribute(RayTracingDebugViewMode::PrimitiveIndex, QT_TRANSLATE_NOOP("AtomLyIntegration", "Primitive Index"))
                        ->EnumAttribute(RayTracingDebugViewMode::Barycentrics, QT_TRANSLATE_NOOP("AtomLyIntegration", "Barycentric Coordinates"))
                        ->EnumAttribute(RayTracingDebugViewMode::Normals, QT_TRANSLATE_NOOP("AtomLyIntegration", "Normals"))
                        ->EnumAttribute(RayTracingDebugViewMode::UVs, QT_TRANSLATE_NOOP("AtomLyIntegration", "UV Coordinates"))
                        ->EnumAttribute(RayTracingDebugViewMode::BaseColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Material Base Color"))
                        ->EnumAttribute(RayTracingDebugViewMode::EmissiveColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Material Emissive Color"))
                        ->EnumAttribute(RayTracingDebugViewMode::IrradianceColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Material Irradiance Color"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RayTracingDebugComponentConfig::GetEnabled)
                ;
                // clang-format on
            }
        }
    }

    u32 RayTracingDebugEditorComponent::OnConfigurationChanged()
    {
        m_controller.OnConfigurationChanged();
        return Edit::PropertyRefreshLevels::AttributesAndValues;
    }
} // namespace AZ::Render
