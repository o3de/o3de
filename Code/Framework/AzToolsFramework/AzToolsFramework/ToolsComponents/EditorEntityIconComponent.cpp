/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AzToolsFramework_precompiled.h"

#include "EditorEntityIconComponent.h"

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EditorViewportIconDisplayInterface.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorEntityIconComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorEntityIconComponent, EditorComponentBase>()
                    ->Field("EntityIconAssetId", &EditorEntityIconComponent::m_entityIconAssetId)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorEntityIconComponent>("Entity Icon", "Edit-time entity icon in entity-inspector and viewport")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                            ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::HideOnAdd | AZ::Edit::SliceFlags::PushWhenHidden)
                        ;
                }
            }
        }

        void EditorEntityIconComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorEntityIconService", 0x94dff5d7));
        }

        void EditorEntityIconComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorEntityIconService", 0x94dff5d7));
        }

        EditorEntityIconComponent::~EditorEntityIconComponent()
        {
        }

        void EditorEntityIconComponent::Init()
        {
            EditorComponentBase::Init();
        }

        void EditorEntityIconComponent::Activate()
        {
            EditorComponentBase::Activate();
            AZ::EntityBus::Handler::BusConnect(GetEntityId());
            EditorEntityIconComponentRequestBus::Handler::BusConnect(GetEntityId());
            EditorInspectorComponentNotificationBus::Handler::BusConnect(GetEntityId());
        }

        void EditorEntityIconComponent::Deactivate()
        {
            EditorInspectorComponentNotificationBus::Handler::BusDisconnect();
            EditorEntityIconComponentRequestBus::Handler::BusDisconnect();
            AZ::EntityBus::Handler::BusDisconnect();
            EditorComponentBase::Deactivate();
        }

        void EditorEntityIconComponent::SetEntityIconAsset(const AZ::Data::AssetId& assetId)
        {
            if (m_entityIconAssetId != assetId)
            {
                m_entityIconAssetId = assetId;
                m_entityIconCache.SetEntityIconPath(CalculateEntityIconPath(m_firstComponentIdCache));
                EditorEntityIconComponentNotificationBus::Event(GetEntityId(), &EditorEntityIconComponentNotificationBus::Events::OnEntityIconChanged, m_entityIconAssetId);
                SetDirty();
            }
        }

        AZ::Data::AssetId EditorEntityIconComponent::GetEntityIconAssetId()
        {
            return m_entityIconAssetId;
        }

        AZStd::string EditorEntityIconComponent::GetEntityIconPath()
        {
            if (m_entityIconCache.Empty())
            {
                UpdateFirstComponentIdCache();
                m_entityIconCache.SetEntityIconPath(CalculateEntityIconPath(m_firstComponentIdCache));
            }

            return m_entityIconCache.GetEntityIconPath();
        }

        int EditorEntityIconComponent::GetEntityIconTextureId()
        {
            return m_entityIconCache.GetEntityIconTextureId();
        }

        bool EditorEntityIconComponent::IsEntityIconHiddenInViewport()
        {
            return (!m_entityIconAssetId.IsValid() && m_preferNoViewportIcon);
        }

        void EditorEntityIconComponent::OnEntityActivated(const AZ::EntityId&)
        {
            if (m_entityIconCache.Empty())
            {
                /* The case where the entity is activated the first time. */

                UpdatePreferNoViewportIconFlag();
                UpdateFirstComponentIdCache();
                m_entityIconCache.SetEntityIconPath(CalculateEntityIconPath(m_firstComponentIdCache));
                EditorEntityIconComponentNotificationBus::Event(GetEntityId(), &EditorEntityIconComponentNotificationBus::Events::OnEntityIconChanged, m_entityIconAssetId);
            }
        }

        void EditorEntityIconComponent::OnComponentOrderChanged()
        {
            if (!m_entityIconAssetId.IsValid())
            {
                const bool preferNoViewportIconFlagChanged = UpdatePreferNoViewportIconFlag();
                const bool firstComponentIdChanged = UpdateFirstComponentIdCache();

                if (firstComponentIdChanged)
                {
                    m_entityIconCache.SetEntityIconPath(GetDefaultEntityIconPath(m_firstComponentIdCache));
                    EditorEntityIconComponentNotificationBus::Event(GetEntityId(), &EditorEntityIconComponentNotificationBus::Events::OnEntityIconChanged, m_entityIconAssetId);
                }
                else if (preferNoViewportIconFlagChanged)
                {
                    EditorEntityIconComponentNotificationBus::Event(GetEntityId(), &EditorEntityIconComponentNotificationBus::Events::OnEntityIconChanged, m_entityIconAssetId);
                }
            }
        }

        AZStd::string EditorEntityIconComponent::CalculateEntityIconPath(AZ::ComponentId firstComponentId)
        {
            AZStd::string entityIconPath = GetEntityIconAssetPath();
            if (entityIconPath.empty())
            {
                entityIconPath = GetDefaultEntityIconPath(firstComponentId);
            }
            return entityIconPath;
        }

        AZStd::string EditorEntityIconComponent::GetEntityIconAssetPath()
        {
            bool foundIcon = false;
            AZStd::string entityIconPath;
            if (m_entityIconAssetId.IsValid())
            {
                AZ::Data::AssetInfo iconAssetInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(iconAssetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, m_entityIconAssetId);
                if (iconAssetInfo.m_assetType != AZ::Data::s_invalidAssetType)
                {
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundIcon, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, iconAssetInfo.m_relativePath, entityIconPath);
                }
            }

            if (foundIcon)
            {
                return entityIconPath;
            }
            else
            {
                return AZStd::string();
            }
        }

        AZStd::string EditorEntityIconComponent::GetDefaultEntityIconPath(AZ::ComponentId firstComponentId)
        {
            AZStd::string entityIconPath;

            if (firstComponentId != AZ::InvalidComponentId)
            {
                AZ::Entity* entity = GetEntity();
                if (entity)
                {
                    AZ::Component* component = entity->FindComponent(firstComponentId);
                    if (component)
                    {
                        AZ::Uuid componentType = AzToolsFramework::GetUnderlyingComponentType(*component);
                        AzToolsFramework::EditorRequestBus::BroadcastResult(entityIconPath, &AzToolsFramework::EditorRequestBus::Events::GetComponentIconPath, componentType, AZ::Edit::Attributes::ViewportIcon, component);
                    }
                }
            }

            if (entityIconPath.empty())
            {
                AzToolsFramework::EditorRequestBus::BroadcastResult(entityIconPath, &AzToolsFramework::EditorRequestBus::Events::GetDefaultEntityIcon);
            }

            return entityIconPath;
        }

        bool EditorEntityIconComponent::UpdateFirstComponentIdCache()
        {
            bool firstComponentIdChanged = false;
            ComponentOrderArray componentOrderArray;
            EditorInspectorComponentRequestBus::EventResult(componentOrderArray, GetEntityId(), &EditorInspectorComponentRequests::GetComponentOrderArray);
            if (componentOrderArray.empty())
            {
                if (m_firstComponentIdCache != AZ::InvalidComponentId)
                {
                    m_firstComponentIdCache = AZ::InvalidComponentId;
                    firstComponentIdChanged = true;
                }
            }
            else
            {
                if (componentOrderArray.size() > 1)
                {
                    if (m_firstComponentIdCache != componentOrderArray[1])
                    {
                        m_firstComponentIdCache = componentOrderArray[1];
                        firstComponentIdChanged = true;
                    }
                }
                else if(m_firstComponentIdCache != componentOrderArray.front())
                {
                    m_firstComponentIdCache = componentOrderArray.front();
                    firstComponentIdChanged = true;
                }
            }

            return firstComponentIdChanged;
        }

        bool EditorEntityIconComponent::UpdatePreferNoViewportIconFlag()
        {
            bool flagChanged = false;
            ComponentOrderArray componentOrderArray;
            EditorInspectorComponentRequestBus::EventResult(componentOrderArray, GetEntityId(), &EditorInspectorComponentRequests::GetComponentOrderArray);
            if (componentOrderArray.empty())
            {
                if (m_preferNoViewportIcon == true)
                {
                    m_preferNoViewportIcon = false;
                    flagChanged = true;
                }
            }
            else
            {
                bool preferNoViewportIcon = false;

                AZ::SerializeContext* serializeContext = nullptr;
                EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
                AZ_Assert(serializeContext, "No serialize context");

                for (AZ::ComponentId componentId : componentOrderArray)
                {
                    AZ::Entity* entity = GetEntity();
                    AZ::Component* component = entity->FindComponent(componentId);
                    if (component == nullptr)
                    {
                        continue;
                    }

                    AZ::Uuid componentType = AzToolsFramework::GetUnderlyingComponentType(*component);
                    auto classData = serializeContext->FindClassData(componentType);
                    if (classData && classData->m_editData)
                    {
                        auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                        if (editorElementData)
                        {
                            if (auto preferNoViewportIconAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::PreferNoViewportIcon))
                            {
                                auto preferNoViewportIconAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<bool>*>(preferNoViewportIconAttribute);
                                if (preferNoViewportIconAttributeData)
                                {
                                    if (preferNoViewportIconAttributeData->Get(nullptr))
                                    {
                                        preferNoViewportIcon = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                if (m_preferNoViewportIcon != preferNoViewportIcon)
                {
                    m_preferNoViewportIcon = preferNoViewportIcon;
                    flagChanged = true;
                }
            }

            return flagChanged;
        }

        int EditorEntityIconComponent::EntityIcon::GetEntityIconTextureId()
        {
            // if we do not yet have a valid texture id, request it using the entity icon path
            if (m_entityIconTextureId == 0)
            {
                m_entityIconTextureId = EditorViewportIconDisplay::Get()->GetOrLoadIconForPath(m_entityIconPath);
            }

            return m_entityIconTextureId;
        }

    } // namespace Components
} // namespace AzToolsFramework
