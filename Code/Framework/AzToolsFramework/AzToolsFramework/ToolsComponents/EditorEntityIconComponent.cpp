/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorEntityIconComponent.h"

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EditorViewportIconDisplayInterface.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
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
            services.push_back(AZ_CRC_CE("EditorEntityIconService"));
        }

        void EditorEntityIconComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorEntityIconService"));
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
            m_needsInitialUpdate = true;
            EditorComponentBase::Activate();
            EditorEntityIconComponentRequestBus::Handler::BusConnect(GetEntityId());
            EditorInspectorComponentNotificationBus::Handler::BusConnect(GetEntityId());
        }

        void EditorEntityIconComponent::Deactivate()
        {
            EditorInspectorComponentNotificationBus::Handler::BusDisconnect();
            EditorEntityIconComponentRequestBus::Handler::BusDisconnect();
            EditorComponentBase::Deactivate();
        }

        void EditorEntityIconComponent::SetEntityIconAsset(const AZ::Data::AssetId& assetId)
        {
            if (m_entityIconAssetId != assetId)
            {
                // Note that we could be going from a situation where we had an icon, to one where we don't so the cache needs refreshing.
                if (!assetId.IsValid())
                {
                    m_needsInitialUpdate = true;
                }
                m_entityIconAssetId = assetId;
                m_entityIconCache.SetEntityIconPath(CalculateEntityIconPath());
                                
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
            RefreshCachesIfNecessary();
            return m_entityIconCache.GetEntityIconPath();
        }

        int EditorEntityIconComponent::GetEntityIconTextureId()
        {
            RefreshCachesIfNecessary();
            return m_entityIconCache.GetEntityIconTextureId();
        }

        void EditorEntityIconComponent::RefreshCachesIfNecessary()
        {
            if (!m_needsInitialUpdate)
            {
                return;
            }

            m_needsInitialUpdate = false;

            if (m_firstComponentIdCache == AZ::InvalidComponentId)
            {
                UpdatePreferNoViewportIconFlag();
                UpdateFirstComponentIdCache();
            }

            m_entityIconCache.SetEntityIconPath(CalculateEntityIconPath());
        }

        bool EditorEntityIconComponent::IsEntityIconHiddenInViewport()
        {
            RefreshCachesIfNecessary();
            return (!m_entityIconAssetId.IsValid() && m_preferNoViewportIcon);
        }

        void EditorEntityIconComponent::OnComponentOrderChanged()
        {
            if (!m_entityIconAssetId.IsValid())
            {
                const bool preferNoViewportIconFlagChanged = UpdatePreferNoViewportIconFlag();
                const bool firstComponentIdChanged = UpdateFirstComponentIdCache();

                if (firstComponentIdChanged)
                {
                    m_entityIconCache.SetEntityIconPath(GetDefaultEntityIconPath());
                    EditorEntityIconComponentNotificationBus::Event(GetEntityId(), &EditorEntityIconComponentNotificationBus::Events::OnEntityIconChanged, m_entityIconAssetId);
                }
                else if (preferNoViewportIconFlagChanged)
                {
                    EditorEntityIconComponentNotificationBus::Event(GetEntityId(), &EditorEntityIconComponentNotificationBus::Events::OnEntityIconChanged, m_entityIconAssetId);
                }
                m_needsInitialUpdate = false; // avoid doing the work twice.
            }
        }

        AZStd::string EditorEntityIconComponent::CalculateEntityIconPath()
        {
            AZStd::string entityIconPath = GetEntityIconAssetPath();
            if (entityIconPath.empty())
            {
                entityIconPath = GetDefaultEntityIconPath();
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

        AZStd::string EditorEntityIconComponent::GetDefaultEntityIconPath()
        {
            AZStd::string entityIconPath;

            RefreshCachesIfNecessary();
            AZ::ComponentId firstComponentId = m_firstComponentIdCache;

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
            // The idea here is to find the first component that is not a fixed component (like TransformComponent, UniformScaleComponent,
            // etc.) on the entity and use that as the default icon.
            // The entity only stores its component order if the component order is different from the default order, which is only the case
            // if the user has personally modified the order of the components, which is not the case for the vast majority of entities in real levels.
            // So to figure out the icon, we need to essentially do the same work the inspector would do, which is to take the components on the entity
            // and filter out the invisible ones, then sort them by order if order is present, and then by priority.
            bool firstComponentIdChanged = false;
            ComponentOrderArray componentOrderArray;
            EditorInspectorComponentRequestBus::EventResult(componentOrderArray, GetEntityId(), &EditorInspectorComponentRequests::GetComponentOrderArray);
            AZ::Entity::ComponentArrayType components;
            components = GetEntity()->GetComponents();
            RemoveHiddenComponents(components);
            SortComponentsByOrder(GetEntity()->GetId(), components);
            SortComponentsByPriority(components);

            AZ::ComponentId componentIdToSet = AZ::InvalidComponentId;
            // we want to use the first "non-fixed" component on the entity (ie, not the uniform scale, or transform) but we'll settle for
            // the first visible component if we can't find one.
            if (!components.empty())
            {
                for (const AZ::Component* component : components)
                {
                    if (componentIdToSet == AZ::InvalidComponentId)
                    {
                        // initialize it to the first one as a fallback
                        componentIdToSet = component->GetId();
                    }
                    else
                    {
                        if (IsComponentDraggable(component))
                        {
                            componentIdToSet = component->GetId();
                            break;
                        }
                    }
                }
            }

            if (m_firstComponentIdCache != componentIdToSet)
            {
                m_firstComponentIdCache = componentIdToSet;
                firstComponentIdChanged = true;
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
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
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
