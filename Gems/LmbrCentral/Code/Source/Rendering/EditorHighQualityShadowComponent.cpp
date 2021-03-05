/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "LmbrCentral_precompiled.h"

#include "EditorHighQualityShadowComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>



namespace LmbrCentral
{
    void EditorHighQualityShadowConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorHighQualityShadowConfig, HighQualityShadowConfig>()->
                Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<HighQualityShadowConfig>(
                    "Shadow Map Settings", "Settings for the entity's shadow map")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &HighQualityShadowConfig::m_enabled, "Enabled", "Enable the shadow map")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &HighQualityShadowConfig::EditorRefresh)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &HighQualityShadowConfig::m_constBias, "Const Bias", "Constant bias")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.001f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &HighQualityShadowConfig::EditorRefresh)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &HighQualityShadowConfig::m_slopeBias, "Slope Bias", "Slope bias")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &HighQualityShadowConfig::EditorRefresh)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &HighQualityShadowConfig::m_jitter, "Jitter", "Jitter")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &HighQualityShadowConfig::EditorRefresh)
                    ->DataElement(AZ::Edit::UIHandlers::Vector3, &HighQualityShadowConfig::m_bboxScale, "Bounding Box Scale", "Scale applied to the shadow frustum")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &HighQualityShadowConfig::EditorRefresh)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &HighQualityShadowConfig::m_shadowMapSize, "Shadow Map Size", "Shadow map size")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, AZStd::vector<AZ::Edit::EnumConstant<int> > { 
                            AZ::Edit::EnumConstant<int>(256, "256"),
                            AZ::Edit::EnumConstant<int>(512, "512"),
                            AZ::Edit::EnumConstant<int>(1024, "1024"),
                            AZ::Edit::EnumConstant<int>(2048, "2048"),
                            AZ::Edit::EnumConstant<int>(8192, "8192")
                            })
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &HighQualityShadowConfig::EditorRefresh)
                    ;
            }
        }
    }

    void EditorHighQualityShadowConfig::EditorRefresh()
    {
        if (m_entityId.IsValid())
        {
            EditorHighQualityShadowComponentRequestBus::Event(m_entityId, &EditorHighQualityShadowComponentRequests::RefreshProperties);
        }
    }

    void EditorHighQualityShadowComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorHighQualityShadowConfig::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorHighQualityShadowComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("EditorHighQualityShadowConfig", &EditorHighQualityShadowComponent::m_config);

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<EditorHighQualityShadowComponent>("High Quality Shadow", "Assigns a unique shadow map to the entity to provide higher quality shadows. Has performance and memory impact so use sparingly.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Shadow.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Shadow.png")
                        ->Attribute(AZ::Edit::Attributes::PreferNoViewportIcon, true)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-high-quality-shadow.html")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorHighQualityShadowComponent::m_config, "Shadow Map Settings", "Settings for the entity's unique shadow map")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void EditorHighQualityShadowComponent::Activate()
    {
        m_config.m_entityId = GetEntityId();
        EditorHighQualityShadowComponentRequestBus::Handler::BusConnect(GetEntityId());
        MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void EditorHighQualityShadowComponent::Deactivate()
    {
        HighQualityShadowComponentUtils::RemoveShadow(GetEntityId());
        EditorHighQualityShadowComponentRequestBus::Handler::BusDisconnect();
        MeshComponentNotificationBus::Handler::BusDisconnect(GetEntityId());
    }

    void EditorHighQualityShadowComponent::RefreshProperties()
    {
        HighQualityShadowComponentUtils::ApplyShadowSettings(GetEntityId(), m_config);
    }

    void EditorHighQualityShadowComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (HighQualityShadowComponent* runtimeComponent = gameEntity->CreateComponent<HighQualityShadowComponent>())
        {
            runtimeComponent->m_config = m_config;
        }
    }

    void EditorHighQualityShadowComponent::OnMeshCreated([[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        HighQualityShadowComponentUtils::ApplyShadowSettings(GetEntityId(), m_config);
    }

    void EditorHighQualityShadowComponent::OnMeshDestroyed()
    {
        HighQualityShadowComponentUtils::RemoveShadow(GetEntityId());
    }


} // namespace LmbrCentral


