/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OpenParticleSystem/EditorParticleComponent.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <OpenParticleSystem/ParticleFeatureProcessorInterface.h>

namespace OpenParticle
{
    void EditorParticleComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext != nullptr)
        {
            serializeContext->Class<EditorParticleComponent, BaseClass>()->Version(0);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext != nullptr)
            {
                editContext->Class<EditorParticleComponent>("Particle", "Particle System")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Particle System")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<ParticleAsset>::Uuid());

                editContext->Class<ParticleComponentController>("ParticleComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticleComponentController::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                editContext->Class<ParticleComponentConfig>("ParticleComponentConfig", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleComponentConfig::m_enable, "Enable",
                            "Control whether this particle effect is enabled, can only be changed in edit mode")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ParticleComponentConfig::m_autoPlay, "AutoPlay",
                            "Control whether this particle effect auto played after loaded, (e.g. loaded in editor or game beginning)")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticleComponentConfig::m_particleAsset, "Asset", "Particle Asset")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticleComponentConfig::m_followActiveCamera, "Follow camera",
                            "Particles always generated around active camera and absolute position of particle system will be ignored, global space "
                            "used forcibly.");
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("EditorParticleComponentTypeId", BehaviorConstant(AZ::Uuid(EditorParticleComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

            behaviorContext->Class<EditorParticleComponent>()->RequestBus("ParticleRequestBus");
        }
    }

    void EditorParticleComponent::Activate()
    {
        const AZ::EntityId entityId = GetEntityId();
        const AZ::EntityComponentIdPair entityComponentIdPair{ entityId, GetId() };
        BaseClass::Activate();
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(entityId);
    }

    void EditorParticleComponent::Deactivate()
    {
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        BaseClass::Deactivate();
    }

    AZ::u32 EditorParticleComponent::OnConfigurationChanged()
    {
        return BaseClass::OnConfigurationChanged();
    }

    void EditorParticleComponent::OnEntityVisibilityChanged(bool visibility)
    {
        m_controller.SetVisible(visibility);
    }
} // namespace OpenParticle
