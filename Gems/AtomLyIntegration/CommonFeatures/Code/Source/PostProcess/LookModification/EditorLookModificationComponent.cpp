/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/LookModification/EditorLookModificationComponent.h>

namespace AZ
{
    namespace Render
    {
        void EditorLookModificationComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorLookModificationComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorLookModificationComponent>(
                        "Look Modification", "The look modification process.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Atom")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.png") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://") // [TODO ATOM-2672][PostFX] need to create page for PostProcessing.
                        ;

                    editContext->Class<LookModificationComponentController>(
                        "LookModificationComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LookModificationComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<LookModificationComponentConfig>("LookModificationComponentConfig", "")
                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &LookModificationComponentConfig::m_enabled,
                            "Enable look modification",
                            "Enable look modification.")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &LookModificationComponentConfig::m_colorGradingLut, "Color Grading LUT", "Color grading LUT")

                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->DataElement(Edit::UIHandlers::ComboBox,
                            &LookModificationComponentConfig::m_shaperPresetType,
                            "Shaper Type",
                            "Shaper Type.")
                        ->EnumAttribute(ShaperPresetType::None, "None")
                        ->EnumAttribute(ShaperPresetType::Log2_48_nits, "Log2_48_nits")
                        ->EnumAttribute(ShaperPresetType::Log2_1000_nits, "Log2_1000_nits")
                        ->EnumAttribute(ShaperPresetType::Log2_2000_nits, "Log2_2000_nits")
                        ->EnumAttribute(ShaperPresetType::Log2_4000_nits, "Log2_4000_nits")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &LookModificationComponentConfig::m_colorGradingLutIntensity, "LUT Intensity", "Blend intensity of this LUT.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &LookModificationComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &LookModificationComponentConfig::m_colorGradingLutOverride, "LUT Override", "Blend intensity of this LUT.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &LookModificationComponentConfig::ArePropertiesReadOnly)

                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Overrides")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                        // Auto-gen editor context settings for overrides
#define EDITOR_CLASS LookModificationComponentConfig
#include <Atom/Feature/ParamMacros/StartOverrideEditorContext.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                            ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorLookModificationComponent>()->RequestBus("LookModificationRequestBus");

                behaviorContext->ConstantProperty("EditorLookModificationComponentTypeId", BehaviorConstant(Uuid(EditorLookModificationComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorLookModificationComponent::EditorLookModificationComponent(const LookModificationComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorLookModificationComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
