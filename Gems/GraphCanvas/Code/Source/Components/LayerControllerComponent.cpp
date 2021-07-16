/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Components/LayerControllerComponent.h>

#include <GraphCanvas/Components/StyleBus.h>

namespace GraphCanvas
{
    // Each layer will consist of a series of slots reserved for various elements
    // These are the number of slots to reserve for each element, and is tracked roughly 
    //
    // Group offsets will occur after selection offsets. Higher number means it will be on top of the previous section.
    // i.e. [GroupOffset2 = 4, GroupOffset1 = 3, SelectionOffset2 = 2, SelectionOffset1 = 1] for Layer 1.
    // i.e. [GroupOffset2 = 8, GroupOffset1 = 7, SelectionOffset2 = 6, SelectionOffset1 = 5] for Layer 2.
    //
    // This will ensure that each layer still holds priority, but internal to the layer we can still shift things around to make it feel more natural.
    //
    // Selection is doubled since I don't have a signal for when the selection is cleared to avoid that. So to keep the same general 'feel', I've just doubled the amount of offsets, and let the deselect downgrade it as well.
    constexpr int s_selectionOffets = 20;
    constexpr int s_groupOffsets = 10;

    ///////////////
    // LayerUtils
    ///////////////

    int LayerUtils::AlwaysOnTopZValue()
    {
        return std::numeric_limits<int>::max() - 1;
    }

    int LayerUtils::AlwaysOnBottomZValue()
    {
        return std::numeric_limits<int>::min() + 1;
    }

    /////////////////////////////
    // LayerControllerComponent
    /////////////////////////////

    void LayerControllerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<LayerControllerComponent, AZ::Component>()
                ->Version(0)
            ;
        }
    }
    
    LayerControllerComponent::LayerControllerComponent(AZStd::string_view layeringElement, LayerOffset layerOffset)
        : m_baseLayering(layeringElement)
        , m_layerOffset(layerOffset)
        , m_baseModifier("")
        , m_externalLayerModifier("")
    {        
    }
    
    LayerControllerComponent::LayerControllerComponent()
        : m_baseLayering("UnknownLayering")
        , m_baseModifier("")
        , m_externalLayerModifier("")
    {
    }

    void LayerControllerComponent::Init()
    {

    }

    void LayerControllerComponent::Activate()
    {
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
        StateController<AZStd::string>::Notifications::Handler::BusConnect(GetLayerModifierController());
    }

    void LayerControllerComponent::Deactivate()
    {
        SceneMemberNotificationBus::Handler::BusDisconnect();
        StateController<AZStd::string>::Notifications::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusDisconnect();
        LayerControllerRequestBus::Handler::BusDisconnect();
        RootGraphicsItemNotificationBus::Handler::BusDisconnect();

        AZ::SystemTickBus::Handler::BusDisconnect();
        GroupableSceneMemberNotificationBus::MultiHandler::BusDisconnect();
    }

    void LayerControllerComponent::OnSystemTick()
    {
        UpdateZValue();
        LayerControllerNotificationBus::Event(GetEntityId(), &LayerControllerNotifications::OnOffsetsChanged, m_layerOffset, m_groupLayerOffset);
        AZ::SystemTickBus::Handler::BusDisconnect();
    }
    
    void LayerControllerComponent::OnSceneSet(const AZ::EntityId& sceneId)
    {
        if (SceneNotificationBus::Handler::BusIsConnected())
        {
            SceneNotificationBus::Handler::BusDisconnect();
        }
        
        if (sceneId.IsValid())
        {
            SceneNotificationBus::Handler::BusConnect(sceneId);
            SceneRequestBus::EventResult(m_editorId, sceneId, &SceneRequests::GetEditorId);
        }

        auto entityId = GetEntityId();

        LayerControllerRequestBus::Handler::BusConnect(entityId);
        RootGraphicsItemNotificationBus::Handler::BusConnect(entityId);

        m_uiRequests = SceneMemberUIRequestBus::FindFirstHandler(entityId);
        m_groupableRequests = GroupableSceneMemberRequestBus::FindFirstHandler(entityId);

        GroupableSceneMemberNotificationBus::MultiHandler::BusConnect(entityId);

        ComputeCurrentLayer();
    }
    
    void LayerControllerComponent::OnStylesChanged()
    {
        StyleManagerRequestBus::EventResult(m_layer, m_editorId, &StyleManagerRequests::FindLayer, m_currentStyle);

        AZ::SystemTickBus::Handler::BusConnect();
    }

    void LayerControllerComponent::OnSelectionChanged()
    {
        if (m_isInspected)
        {
            SetSelectionLayerOffset(s_selectionOffets - 1);
        }
        else if (m_selectionOffset > 0)
        {
            SetSelectionLayerOffset(m_selectionOffset - 1);            
        }
    }
    
    void LayerControllerComponent::OnDisplayStateChanged(RootGraphicsItemDisplayState /*oldState*/, RootGraphicsItemDisplayState newState)
    {
        // Handle selection information
        if (newState == Inspection)
        {
            m_isInspected = true;
        }
        else
        {
            m_isInspected = false;
        }

        if (newState == Deletion
            || newState == InspectionTransparent
            || newState == Inspection)
        {
            m_baseModifier = "interactive";
        }
        else if (newState == RootGraphicsItemDisplayState::GroupHighlight)
        {
            m_baseModifier = "groupHighlight";
        }
        else
        {
            m_baseModifier = "";
        }

        ComputeCurrentLayer();
    }
    
    void LayerControllerComponent::OnStateChanged(const AZStd::string& /*state*/)
    {
        ComputeCurrentLayer();
    }

    StateController<AZStd::string>* LayerControllerComponent::GetLayerModifierController()
    {
        return &m_externalLayerModifier;
    }

    void LayerControllerComponent::OnGroupChanged()
    {
        GroupableSceneMemberNotificationBus::MultiHandler::BusDisconnect();

        GroupableSceneMemberNotificationBus::MultiHandler::BusConnect(GetEntityId());

        if (m_groupableRequests)
        {
            int groupLayerOffset = 0;
            AZ::EntityId groupId = m_groupableRequests->GetGroupId();

            while (groupId.IsValid())
            {
                groupLayerOffset += 1;
                GroupableSceneMemberNotificationBus::MultiHandler::BusConnect(groupId);

                AZ::EntityId newGroupId;
                GroupableSceneMemberRequestBus::EventResult(newGroupId, groupId, &GroupableSceneMemberRequests::GetGroupId);
                if (newGroupId == groupId)
                {
                    break;
                }

                groupId = newGroupId;
            }

            SetGroupLayerOffset(groupLayerOffset);
        }
    }

    int LayerControllerComponent::GetLayerOffset() const
    {
        // Triple indexed array logic
        return m_layerOffset + (m_selectionOffset * OffsetCount) + (m_groupLayerOffset * s_selectionOffets * OffsetCount);
    }

    int LayerControllerComponent::GetSelectionOffset() const
    {
        return m_selectionOffset;
    }

    int LayerControllerComponent::GetGroupLayerOffset() const
    {
        return m_groupLayerOffset;
    }

    void LayerControllerComponent::SetBaseModifier(AZStd::string_view baseModifier)
    {
        m_baseModifier = baseModifier;
    }

    void LayerControllerComponent::SetGroupLayerOffset(int groupOffset)
    {
        m_groupLayerOffset = groupOffset;

        if (m_groupLayerOffset > s_groupOffsets)
        {
            m_groupLayerOffset = s_groupOffsets;
        }

        AZ::SystemTickBus::Handler::BusConnect();
    }

    void LayerControllerComponent::SetSelectionLayerOffset(int selectionOffset)
    {
        m_selectionOffset = selectionOffset;

        if (m_selectionOffset > s_selectionOffets)
        {
            m_selectionOffset = s_selectionOffets;
        }

        AZ::SystemTickBus::Handler::BusConnect();
    }

    int LayerControllerComponent::CalculateZValue(int layer)
    {
        // Z value is a triple indexed array more or less.
        // OffsetCount handles the individual layers which are per election offset.
        // Each seleciton offset is managed per group offset.
        return layer * (OffsetCount + s_selectionOffets * OffsetCount + s_groupOffsets * s_selectionOffets * OffsetCount) + GetLayerOffset();
    }

    void LayerControllerComponent::UpdateZValue()
    {
        m_uiRequests->SetZValue(CalculateZValue(m_layer));
    }

    void LayerControllerComponent::ComputeCurrentLayer()
    {
        if (m_externalLayerModifier.GetState().empty()
            && m_baseModifier.empty())
        {
            m_currentStyle = "default";
        }
        else if (m_externalLayerModifier.HasState())
        {
            m_currentStyle = m_externalLayerModifier.GetState().c_str();
        }
        else
        {
            m_currentStyle = m_baseModifier.c_str();
        }

        OnStylesChanged();
    }
}
