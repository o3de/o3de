/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Debug/RayTracingDebugEditorComponent.h>

namespace AZ::Render
{
    void RayTracingDebugEditorComponent::Reflect(ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext{ azrtti_cast<SerializeContext*>(context) })
        {
            // clang-format off
            serializeContext->Class<RayTracingDebugEditorComponent, BaseClass>()
                ->Version(0)
            ;
            // clang-format on

            if (auto editContext{ serializeContext->GetEditContext() })
            {
                // clang-format off
                editContext->Class<RayTracingDebugEditorComponent>("Debug Ray Tracing", "Controls for debugging ray tracing.")
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
                    ->DataElement(Edit::UIHandlers::Default, &RayTracingDebugComponentController::m_configuration, "Configuration", "")
                        ->Attribute(Edit::Attributes::Visibility, Edit::PropertyVisibility::ShowChildrenOnly)
                ;

                editContext->Class<RayTracingDebugComponentConfig>("RayTracingDebugComponentConfig", "")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                    ->DataElement(Edit::UIHandlers::CheckBox, &RayTracingDebugComponentConfig::m_enabled, "Enable Ray Tracing Debugging", "Enable Ray Tracing Debugging.")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::EntireTree)
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
