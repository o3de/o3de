/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorMiniAudioListenerComponent.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace MiniAudio
{
    AZ::ComponentDescriptor* EditorMiniAudioListenerComponent_CreateDescriptor()
    {
        return EditorMiniAudioListenerComponent::CreateDescriptor();
    }

    void EditorMiniAudioListenerComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorMiniAudioListenerComponent, BaseClass>()->Version(2);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorMiniAudioListenerComponent>(
                    QT_TRANSLATE_NOOP("MiniAudio", "MiniAudio Listener"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "MiniAudio")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                editContext->Class<MiniAudioListenerComponentController>(
                    QT_TRANSLATE_NOOP("MiniAudio", "MiniAudioListenerComponentController"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MiniAudioListenerComponentController::m_config,
                        QT_TRANSLATE_NOOP("MiniAudio", "Configuration"), "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                editContext
                    ->Class<MiniAudioListenerComponentConfig>(
                        QT_TRANSLATE_NOOP("MiniAudio", "MiniAudioListenerComponent Config"),
                        QT_TRANSLATE_NOOP("MiniAudio", "[Configuration for MiniAudioListenerComponent]"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioListenerComponentConfig::m_followEntity,
                        QT_TRANSLATE_NOOP("MiniAudio", "Follow Entity"),
                        QT_TRANSLATE_NOOP("MiniAudio", "The listener will follow the position and orientation of the specified entity."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioListenerComponentConfig::m_listenerIndex,
                        QT_TRANSLATE_NOOP("MiniAudio", "Listener Index"),
                        QT_TRANSLATE_NOOP("MiniAudio", "MiniAudio listener index to control."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &MiniAudioListenerComponentConfig::m_globalVolume,
                        QT_TRANSLATE_NOOP("MiniAudio", "Global Volume"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Sets the global volume of the audio engine, as a percentage."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("MiniAudio", " %"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioListenerComponentConfig::m_innerAngleInDegrees,
                        QT_TRANSLATE_NOOP("MiniAudio", "Inner Cone Angle"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Sets the listener's inner cone angle in Degrees."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("MiniAudio", " degrees"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &MiniAudioListenerComponentConfig::m_outerAngleInDegrees,
                        QT_TRANSLATE_NOOP("MiniAudio", "Outer Cone Angle"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Sets the listener's outer cone angle in Degrees."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("MiniAudio", " degrees"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &MiniAudioListenerComponentConfig::m_outerVolume,
                        QT_TRANSLATE_NOOP("MiniAudio", "Outer Volume"),
                        QT_TRANSLATE_NOOP("MiniAudio", "Sets the volume of the listener outside of the outer cone, as a percentage."))
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Suffix,
                        QT_TRANSLATE_NOOP("MiniAudio", " %"));
            }
        }
    }

    EditorMiniAudioListenerComponent::EditorMiniAudioListenerComponent(const MiniAudioListenerComponentConfig& config)
        : BaseClass(config)
    {
    }

    void EditorMiniAudioListenerComponent::Activate()
    {
        BaseClass::Activate();
    }

    void EditorMiniAudioListenerComponent::Deactivate()
    {
        BaseClass::Deactivate();
    }

    AZ::u32 EditorMiniAudioListenerComponent::OnConfigurationChanged()
    {
        m_controller.OnConfigurationUpdated();
        return AZ::Edit::PropertyRefreshLevels::None;
    }
} // namespace MiniAudio
