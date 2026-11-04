/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/Bloom/EditorBloomComponent.h>

#include <AzFramework/Translation/TranslationDef.h>

namespace AZ
{
    namespace Render
    {
        void EditorBloomComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorBloomComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorBloomComponent>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Bloom"),
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls the Bloom"))
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/bloom/")
                        ;

                    editContext->Class<BloomComponentController>(
                        "ExposureControlComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &BloomComponentController::m_configuration,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<BloomComponentConfig>("BloomComponentConfig", "")
                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &BloomComponentConfig::m_enabled,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Bloom"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Bloom."))
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &BloomComponentConfig::m_threshold,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Threshold"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "How bright is the light source bloom applied to "))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &BloomComponentConfig::m_knee,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Knee"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Soft knee to smoothen edge of threshold"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &BloomComponentConfig::m_intensity,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Intensity"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Brightness of bloom"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 25.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &BloomComponentConfig::m_enableBicubic,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Bicubic"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable bicubic filter for upsampling"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        // Kernel sizes
                        ->ClassElement(AZ::Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Kernel Size"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &BloomComponentConfig::m_kernelSizeScale,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Kernel Size Scale"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Global scaling factor of kernel size"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify,
                            Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &BloomComponentConfig::m_kernelSizeStage0,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Kernel Size 0"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Kernel size for blur stage 0 in percent of screen width"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &BloomComponentConfig::m_kernelSizeStage1,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Kernel Size 1"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Kernel size for blur stage 1 in percent of screen width"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &BloomComponentConfig::m_kernelSizeStage2,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Kernel Size 2"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Kernel size for blur stage 2 in percent of screen width"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &BloomComponentConfig::m_kernelSizeStage3,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Kernel Size 3"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Kernel size for blur stage 3 in percent of screen width"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &BloomComponentConfig::m_kernelSizeStage4,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Kernel Size 4"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Kernel size for blur stage 4 in percent of screen width"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        // Tints
                        ->ClassElement(AZ::Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                        ->DataElement(AZ::Edit::UIHandlers::Color, &BloomComponentConfig::m_tintStage0,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint 0"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint for blur stage 0"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Color, &BloomComponentConfig::m_tintStage1,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint 1"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint for blur stage 1"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Color, &BloomComponentConfig::m_tintStage2,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint 2"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint for blur stage 2"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Color, &BloomComponentConfig::m_tintStage3,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint 3"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint for blur stage 3"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Color, &BloomComponentConfig::m_tintStage4,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint 4"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Tint for blur stage 4"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &BloomComponentConfig::ArePropertiesReadOnly)

                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Overrides"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        // Auto-gen editor context settings for overrides
#define EDITOR_CLASS BloomComponentConfig
#include <Atom/Feature/ParamMacros/StartOverrideEditorContext.inl>
#include <Atom/Feature/PostProcess/Bloom/BloomParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                            ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorBloomComponent>()->RequestBus("BloomRequestBus");

                behaviorContext->ConstantProperty("EditorBloomComponentTypeId", BehaviorConstant(Uuid(Bloom::EditorBloomComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorBloomComponent::EditorBloomComponent(const BloomComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorBloomComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
