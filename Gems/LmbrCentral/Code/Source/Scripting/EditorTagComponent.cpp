/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include "EditorTagComponent.h"
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    //=========================================================================
    // Component Descriptor
    //=========================================================================
    void EditorTagComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EditorTagComponent, AZ::Component>()
                ->Version(1)
                ->Field("Tags", &EditorTagComponent::m_tags);


            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorTagComponent>("Tag", "The Tag component allows you to apply one or more labels to an entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                        ->Attribute(AZ::Edit::Attributes::Category, "Gameplay")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Tag.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Tag.svg")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/gameplay/tag/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorTagComponent::m_tags, "Tags", "The tags that will be on this entity by default")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorTagComponent::OnTagChanged);
            }
        }
    }

    //=========================================================================
    // EditorTagComponentRequestBus::Handler
    //=========================================================================
    bool EditorTagComponent::HasTag(const char* tag)
    {
        return AZStd::find(m_tags.begin(), m_tags.end(), tag) != m_tags.end();
    }

    void EditorTagComponent::AddTag(const char* tag)
    {
        if (!HasTag(tag))
        {
            m_tags.push_back(tag);
            ActivateTag(tag);
        }
    }

    void EditorTagComponent::RemoveTag(const char* tag)
    {
        AZStd::remove_if(m_tags.begin(), m_tags.end(), [&tag](const AZStd::string& target) { return target == tag; });
        if (AZStd::find(m_activeTags.begin(), m_activeTags.end(), tag) != m_activeTags.end())
        {
            DeactivateTag(tag);
        }
    }
    //////////////////////////////////////////////////////////////////////////

    //=========================================================================
    // AZ::Component
    //=========================================================================
    void EditorTagComponent::Activate()
    {
        EditorComponentBase::Activate();
        ActivateTags();
        EditorTagComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void EditorTagComponent::Deactivate()
    {
        EditorTagComponentRequestBus::Handler::BusDisconnect();
        DeactivateTags();
        EditorComponentBase::Deactivate();
    }

    //=========================================================================
    // AzToolsFramework::Components::EditorComponentBase
    //=========================================================================
    void EditorTagComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (TagComponent* tagComponent = gameEntity->CreateComponent<TagComponent>())
        {
            Tags newTagList;
            for (const AZStd::string& tag : m_tags)
            {
                newTagList.insert(Tag(tag.c_str()));
            }
            tagComponent->EditorSetTags(AZStd::move(newTagList));
        }
    }

    void EditorTagComponent::ActivateTag(const char* tagName)
    { 
        Tag tag(tagName);
        const AZ::EntityId entityId = GetEntityId();
        m_activeTags.push_back(tagName);

        TagComponentNotificationsBus::Event(entityId, &TagComponentNotificationsBus::Events::OnTagAdded, tag);
        TagGlobalNotificationBus::Event(tag, &TagGlobalNotificationBus::Events::OnEntityTagAdded, entityId);
        // Intentionally don't connect to the TagGlobalRequestBus for the editor component because its bus Id and params are
        // not tied to any entity Id. This can collide with the runtime tag component which will have the same tag name
    }

    void EditorTagComponent::DeactivateTag(const char* tagName)
    {
        Tag tag(tagName);
        const AZ::EntityId entityId = GetEntityId();

        TagGlobalNotificationBus::Event(tag, &TagGlobalNotificationBus::Events::OnEntityTagRemoved, entityId);
        TagComponentNotificationsBus::Event(entityId, &TagComponentNotificationsBus::Events::OnTagRemoved, tag);

        AZStd::remove_if(m_activeTags.begin(), m_activeTags.end(), [&tagName](const AZStd::string& target) { return target == tagName; });
    }

    void EditorTagComponent::ActivateTags()
    {
        for (const AZStd::string& tag : m_tags)
        {
            ActivateTag(tag.c_str());
        }
    }

    void EditorTagComponent::DeactivateTags()
    {
        EditorTags tagsToDeactivate(AZStd::move(m_activeTags));
        for (const AZStd::string& tag : tagsToDeactivate)
        {
            DeactivateTag(tag.c_str());
        }
    }

    void EditorTagComponent::OnTagChanged()
    {
        DeactivateTags();
        ActivateTags();
    }

} // namespace LmbrCentral
