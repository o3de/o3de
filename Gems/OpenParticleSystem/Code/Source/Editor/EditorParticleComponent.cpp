/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorParticleComponent.h"
#include "EditorParticleSystemComponentRequestBus.h"
#include <OpenParticleSystem/ParticleFeatureProcessorInterface.h>

#include <API/EditorAssetSystemAPI.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

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
                        ->Attribute("BrowseIcon", ":/stylesheet/img/UI20/browse-edit-select-files.svg")
                        ->Attribute("EditButton", "")
                        ->Attribute("EditDescription", "Open in Particle Editor")
                        ->Attribute("EditCallback", &EditorParticleComponent::OpenParticleEditor)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ParticleComponentConfig::m_followActiveCamera, "FollowActiveCamera",
                            "Particles always generated around active camera and absolute position of particle system will be ignored, global space "
                            "used forcibly.");
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("EditorParticleComponentTypeId", BehaviorConstant(AZ::Uuid(EditorParticleComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

            behaviorContext->EBus<EditorParticleRequestBus>("EditorParticleRequestBus")
                ->Attribute(AZ::Script::Attributes::Module, "OpenParticleSystem")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Event("SetMaterialDiffuseMap", &EditorParticleRequestBus::Events::SetMaterialDiffuseMap)
                    ->Attribute(AZ::Script::Attributes::ToolTip,  "Set new diffuse map for current particle system")
                ;

            behaviorContext->Class<EditorParticleComponent>()->RequestBus("EditorParticleRequestBus");
        }
    }

    void EditorParticleComponent::Activate()
    {
        const AZ::EntityId entityId = GetEntityId();
        const AZ::EntityComponentIdPair entityComponentIdPair{ entityId, GetId() };
        BaseClass::Activate();
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(entityId);
        EditorParticleRequestBus::Handler::BusConnect(entityId);
    }

    void EditorParticleComponent::Deactivate()
    {
        const AZ::EntityId entityId = GetEntityId();
        EditorParticleRequestBus::Handler::BusDisconnect(entityId);
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        BaseClass::Deactivate();
    }

    void EditorParticleComponent::SetParticleAsset(AZ::Data::Asset<ParticleAsset> particleAsset, bool inParticleEditor)
    {
        m_controller.SetParticleAsset(particleAsset, inParticleEditor);
    }

    void EditorParticleComponent::SetMaterialDiffuseMap(AZ::u32 emitterIndex, AZStd::string mapPath)
    {
        m_controller.SetMaterialDiffuseMap(emitterIndex, mapPath);
    }

    AZ::u32 EditorParticleComponent::OnConfigurationChanged()
    {
        return BaseClass::OnConfigurationChanged();
    }

    void EditorParticleComponent::OnEntityVisibilityChanged(bool visibility)
    {
        m_controller.SetVisible(visibility);
    }

    void EditorParticleComponent::OpenParticleEditor(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType&)
    {
        if (assetId.IsValid())
        {
            bool foundSourceInfo = false;
            AZStd::string folderFoundIn;
            AZ::Data::AssetInfo assetInfo;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundSourceInfo, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourceUUID, assetId.m_guid, assetInfo, folderFoundIn);
            if (foundSourceInfo)
            {
                const AZ::IO::Path assetFullPath = AZ::IO::Path(folderFoundIn) / AZ::IO::Path(assetInfo.m_relativePath);
                EditorParticleSystemComponentRequestBus::Broadcast(&EditorParticleSystemComponentRequestBus::Events::OpenParticleEditor, assetFullPath.c_str());
            }
            else
            {
                AZ_Warning("EditorParticleComponent", false, "Could not find particle editor asset");
            }
        }
    }
} // namespace OpenParticle
