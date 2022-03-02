/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <Debug/RenderDebugEditorComponent.h>

namespace AZ {
    namespace Render {

        void RenderDebugEditorComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<RenderDebugEditorComponent, BaseClass>()
                    ->Version(0);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<RenderDebugEditorComponent>(
                        "Render Debug", "Controls for debugging rendering.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                            ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                        ;

                    editContext->Class<RenderDebugComponentController>(
                        "RenderDebugComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderDebugComponentController::m_configuration, "Configuration", "")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<RenderDebugComponentConfig>("RenderDebugComponentConfig", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &RenderDebugComponentConfig::m_enabled,
                            "Enable Render Debugging",
                            "Enable Render Debugging.")

                        // Auto-gen editor context settings for overrides
// #define EDITOR_CLASS RenderDebugComponentConfig
// #include <Atom/Feature/ParamMacros/StartOverrideEditorContext.inl>
// #include <Atom/Feature/Debug/RenderDebugParams.inl>
// #include <Atom/Feature/ParamMacros/EndParams.inl>
// #undef EDITOR_CLASS
                            ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<RenderDebugEditorComponent>()->RequestBus("RenderDebugRequestBus");

                behaviorContext->ConstantProperty("RenderDebugEditorComponentTypeId", BehaviorConstant(Uuid(RenderDebug::RenderDebugEditorComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        RenderDebugEditorComponent::RenderDebugEditorComponent(const RenderDebugComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 RenderDebugEditorComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
