/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <QGraphicsItem>
#include <QGraphicsLayoutItem>

#include <Components/StylingComponent.h>

namespace GraphCanvas
{
    /////////////////////////////
    // StylingComponentSaveData
    /////////////////////////////

    StylingComponent::StylingComponentSaveData::StylingComponentSaveData(const AZStd::string& subStyle)
        : m_subStyle(subStyle)
    {
    }

    /////////////////////
    // StylingComponent
    /////////////////////
    bool StylingComponentVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 1)
        {
            AZ::Crc32 classId = AZ_CRC_CE("Class");

            StylingComponent::StylingComponentSaveData saveData;

            AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(classId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_subStyle);
            }

            classElement.RemoveElementByName(classId);
            classElement.AddElementWithData(context, "SaveData", saveData);
            classElement.RemoveElementByName(AZ_CRC_CE("Id"));
        }

        return true;
    }

    void StylingComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<StylingComponentSaveData, ComponentSaveData>()
            ->Version(1)
            ->Field("SubStyle", &StylingComponentSaveData::m_subStyle)
        ;

        serializeContext->Class<StylingComponent, AZ::Component>()
            ->Version(2, StylingComponentVersionConverter)
            ->Field("Parent", &StylingComponent::m_parentStyledEntity)
            ->Field("Element", &StylingComponent::m_element)
            ->Field("SaveData", &StylingComponent::m_saveData)
        ;
    }

    AZ::EntityId StylingComponent::CreateStyleEntity(const AZStd::string& element)
    {
        AZ::Entity* entity = aznew AZ::Entity("Style");
        entity->CreateComponent<StylingComponent>(element);

        entity->Init();
        entity->Activate();

        return entity->GetId();
    }

    StylingComponent::StylingComponent(const AZStd::string& element, const AZ::EntityId& parentStyledEntity, const AZStd::string& subStyle)
        : m_parentStyledEntity(parentStyledEntity)
        , m_element(element)
        , m_saveData(subStyle)
    {
    }

    void StylingComponent::Activate()
    {
        m_selectedSelector = Styling::Selector::Get(Styling::States::Selected);
        m_disabledSelector = Styling::Selector::Get(Styling::States::Disabled);
        m_hoveredSelector = Styling::Selector::Get(Styling::States::Hovered);
        m_collapsedSelector = Styling::Selector::Get(Styling::States::Collapsed);
        m_highlightedSelector = Styling::Selector::Get(Styling::States::Highlighted);

        Styling::Selector elementSelector = Styling::Selector::Get(m_element);
        AZ_Assert(elementSelector.IsValid(), "The item's element selector (\"%s\") is not valid", m_element.c_str());
        m_coreSelectors.emplace_back(elementSelector);

        EntitySaveDataRequestBus::Handler::BusConnect(GetEntityId());
        StyledEntityRequestBus::Handler::BusConnect(GetEntityId());
        VisualNotificationBus::Handler::BusConnect(GetEntityId());
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void StylingComponent::Deactivate()
    {
        SceneMemberNotificationBus::Handler::BusDisconnect();
        VisualNotificationBus::Handler::BusDisconnect();
        StyledEntityRequestBus::Handler::BusDisconnect();

        m_selectedSelector = Styling::Selector();
        m_disabledSelector = Styling::Selector();
        m_hoveredSelector = Styling::Selector();
        m_collapsedSelector = Styling::Selector();
        m_highlightedSelector = Styling::Selector();
        m_coreSelectors.clear();
    }

    void StylingComponent::OnItemChange(const AZ::EntityId&, QGraphicsItem::GraphicsItemChange change, const QVariant&)
    {
        if (change == QGraphicsItem::GraphicsItemChange::ItemSelectedHasChanged)
        {
            StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
        }
    }

    AZ::EntityId StylingComponent::GetStyleParent() const
    {
        return m_parentStyledEntity;
    }

    Styling::SelectorVector StylingComponent::GetStyleSelectors() const
    {
        Styling::SelectorVector selectors = m_coreSelectors;

        // Reserve space for all of these selectors added in this function
        selectors.reserve(selectors.size() + m_dynamicSelectors.size() + 3);

        for (auto& mapPair : m_dynamicSelectors)
        {
            selectors.emplace_back(mapPair.second);
        }

        QGraphicsItem* root = nullptr;
        SceneMemberUIRequestBus::EventResult(root, GetEntityId(), &SceneMemberUIRequests::GetRootGraphicsItem);

        if (!root)
        {
            return selectors;
        }

        if (m_hovered)
        {
            selectors.emplace_back(m_hoveredSelector);
        }

        bool isSelected = false;
        SceneMemberUIRequestBus::EventResult(isSelected, GetEntityId(), &SceneMemberUIRequests::IsSelected);

        if (isSelected)
        {
            selectors.emplace_back(m_selectedSelector);
        }

        if (!root->isEnabled())
        {
            selectors.emplace_back(m_disabledSelector);
        }

        // TODO collapsed and highlighted

        return selectors;
    }

    void StylingComponent::AddSelectorState(const char* selectorState)
    {
        auto insertResult = m_dynamicSelectors.insert(AZStd::pair<AZ::Crc32, Styling::Selector>(AZ::Crc32(selectorState), Styling::Selector::Get(selectorState)));

        if (insertResult.second)
        {
            StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
        }
        else
        {
            // Temporary disabling, since with the Node Group, we can add the same selector twice(no reasonable way of resolving this)
            //
            // Will need to port some code to deal with a voting type system of what selectors to add on here rather then just assuming a one on
            // one off system.
            //AZ_Assert(false, "Pushing the same state(%s) onto the selector stack twice. State cannot be correctly removed.", selectorState);
        }
    }

    void StylingComponent::RemoveSelectorState(const char* selectorState)
    {
        AZStd::size_t count = m_dynamicSelectors.erase(AZ::Crc32(selectorState));

        if (count != 0)
        {
            StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
        }
    }

    AZStd::string StylingComponent::GetElement() const
    {
        return m_element;
    }

    AZStd::string StylingComponent::GetClass() const
    {
        return m_saveData.m_subStyle;
    }

    void StylingComponent::OnSceneSet(const AZ::EntityId& scene)
    {
        Styling::Selector classSelector = Styling::Selector::Get(GetClass());
        if (classSelector.IsValid())
        {
            m_coreSelectors.emplace_back(classSelector);
        }

        m_saveData.RegisterIds(scene, GetEntityId());

        SceneNotificationBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusConnect(scene);
        StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
    }

    void StylingComponent::OnRemovedFromScene(const AZ::EntityId& /*scene*/)
    {
        SceneNotificationBus::Handler::BusDisconnect();
    }

    void StylingComponent::OnStylesChanged()
    {
        StyleNotificationBus::Event(GetEntityId(), &StyleNotificationBus::Events::OnStyleChanged);
    }

    void StylingComponent::WriteSaveData(EntitySaveDataContainer& saveDataContainer) const
    {
        StylingComponentSaveData* saveData = saveDataContainer.FindCreateSaveData<StylingComponentSaveData>();

        if (saveData)
        {
            (*saveData) = m_saveData;
        }
    }

    void StylingComponent::ReadSaveData(const EntitySaveDataContainer& saveDataContainer)
    {
        StylingComponentSaveData* saveData = saveDataContainer.FindSaveDataAs<StylingComponentSaveData>();

        if (saveData)
        {
            m_saveData = (*saveData);
        }
    }
}
