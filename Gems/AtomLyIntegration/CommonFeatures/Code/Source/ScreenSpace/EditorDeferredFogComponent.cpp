/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <ScreenSpace/EditorDeferredFogComponent.h>

namespace AZ
{
    namespace Render
    {
        void EditorDeferredFogComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorDeferredFogComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorDeferredFogComponent>(
                        "Deferred Fog", "Controls the Deferred Fog")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Atom")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/deferred-fog/") // [TODO][ATOM-13427] Create Wiki for Deferred Fog
                        ;

                    editContext->Class<DeferredFogComponentController>(
                        "DeferredFogComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DeferredFogComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<DeferredFogComponentConfig>(
                        "DeferredFogComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &DeferredFogComponentConfig::m_enabled,
                            "Enable Deferred Fog",
                            "Enable Deferred Fog.")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Color, &DeferredFogComponentConfig::m_fogColor,
                            "Fog Color", "The fog color.")
                           ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &DeferredFogComponentConfig::m_fogStartDistance,
                            "Fog Start Distance", "The distance from the viewer when the fog starts")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 5000.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &DeferredFogComponentConfig::m_fogEndDistance,
                            "Fog End Distance", "At what distance from the viewer does the fog take over and mask the background scene out")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 5000.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // Fog layer properties
                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &DeferredFogComponentConfig::m_enableFogLayerShaderOption,
                            "Enable Fog Layer",
                            "Enable Fog Layer")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &DeferredFogComponentConfig::m_fogMinHeight,
                            "Fog Bottom Height", "The height at which the fog layer starts")
                            ->Attribute(AZ::Edit::Attributes::Min, -5000.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 5000.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -100.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 1000.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &DeferredFogComponentConfig::m_fogMaxHeight,
                            "Fog Max Height", "The height of the fog layer top")
                            ->Attribute(AZ::Edit::Attributes::Min, -5000.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 5000.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -100.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 1000.0f)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // Fog turbulence properties
                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &DeferredFogComponentConfig::m_useNoiseTextureShaderOption,
                            "Enable Turbulence Properties",
                            "Enable Turbulence Properties")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::LineEdit, &DeferredFogComponentConfig::m_noiseTexture,
                            "Noise Texture", "The noise texture used for creating the fog turbulence")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // First noise octave
                        ->DataElement(AZ::Edit::UIHandlers::Vector2, &DeferredFogComponentConfig::m_noiseScaleUV,
                            "Noise Texture First Octave Scale", "The scale of the first noise octave - higher indicates higher frequency / repetition")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Vector2, &DeferredFogComponentConfig::m_noiseVelocityUV,
                            "Noise Texture First Octave Velocity", "The velocity of the first noise octave UV coordinates")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // Second noise octave
                        ->DataElement(AZ::Edit::UIHandlers::Vector2, &DeferredFogComponentConfig::m_noiseScaleUV2,
                            "Noise Texture Second Octave Scale", "The scale of the second noise octave - higher indicates higher frequency / repetition")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Vector2, &DeferredFogComponentConfig::m_noiseVelocityUV2,
                            "Noise Texture Second Octave Velocity", "The velocity of the second noise octave UV coordinates")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &DeferredFogComponentConfig::m_octavesBlendFactor,
                            "Octaves Blend Factor", "The blend factor between the noise octaves")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.2f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 0.8f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorDeferredFogComponent>()->RequestBus("DeferredFogRequestsBus");

                behaviorContext->ConstantProperty("EditorDeferredFogComponentTypeId", BehaviorConstant(Uuid(DeferredFog::EditorDeferredFogComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorDeferredFogComponent::EditorDeferredFogComponent(const DeferredFogComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorDeferredFogComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
