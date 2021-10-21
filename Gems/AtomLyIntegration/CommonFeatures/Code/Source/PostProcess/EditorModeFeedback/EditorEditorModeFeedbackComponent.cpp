/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/EditorModeFeedback/EditorEditorModeFeedbackComponent.h>

namespace AZ
{
    namespace Render
    {
        void EditorEditorModeFeedbackComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorEditorModeFeedbackComponent, BaseClass>()->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorEditorModeFeedbackComponent>(
                        "Editor Mode Feedback", "Tune the visual feedback for the different editor modes.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Atom")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://") // [TODO ATOM-2672][PostFX] need to create page for PostProcessing.
                        ;

                    editContext->Class<EditorModeFeedbackComponentController>(
                        "EditorModeFeedbackComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorModeFeedbackComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<EditorModeFeedbackComponentConfig>("EditorModeFeedbackComponentConfig", "")
                        ->DataElement(Edit::UIHandlers::CheckBox, &EditorModeFeedbackComponentConfig::m_enabled,
                            "Enable editor mode feedback",
                            "Enable editor mode feedback.")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Focus Mode")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorModeFeedbackComponentConfig::m_desaturationAmount, "Desaturation", "Exposure Value")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                        ;
                }
            }
        }

        EditorEditorModeFeedbackComponent::EditorEditorModeFeedbackComponent(const EditorModeFeedbackComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorEditorModeFeedbackComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    } // namespace Render
} // namespace AZ
