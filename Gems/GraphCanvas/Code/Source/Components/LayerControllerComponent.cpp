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
#include "precompiled.h"

#include <Source/Components/LayerControllerComponent.h>

#include <GraphCanvas/Components/StyleBus.h>

namespace GraphCanvas
{
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
    
    LayerControllerComponent::LayerControllerComponent(AZStd::string_view layeringElement)
        : m_baseLayering(layeringElement)
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
            
            ComputeCurrentLayer();
        }

        LayerControllerRequestBus::Handler::BusConnect(GetEntityId());
        RootGraphicsItemNotificationBus::Handler::BusConnect(GetEntityId());
    }
    
    void LayerControllerComponent::OnStylesChanged()
    {
        int zValue = 0;
        StyleManagerRequestBus::EventResult(zValue, m_editorId, &StyleManagerRequests::FindLayerZValue, m_currentStyle);
        
        SceneMemberUIRequestBus::Event(GetEntityId(), &SceneMemberUIRequests::SetZValue, zValue);
    }
    
    void LayerControllerComponent::OnDisplayStateChanged([[maybe_unused]] RootGraphicsItemDisplayState oldState, RootGraphicsItemDisplayState newState)
    {
        if (newState == Deletion
            || newState == InspectionTransparent
            || newState == Inspection)
        {
            m_baseModifier = "interactive";
        }
        else
        {
            m_baseModifier = "";
        }

        ComputeCurrentLayer();
    }
    
    void LayerControllerComponent::OnStateChanged([[maybe_unused]] const AZStd::string& state)
    {
        ComputeCurrentLayer();
    }

    StateController<AZStd::string>* LayerControllerComponent::GetLayerModifierController()
    {
        return &m_externalLayerModifier;
    }

    void LayerControllerComponent::SetBaseModifier(AZStd::string_view baseModifier)
    {
        m_baseModifier = baseModifier;
    }

    void LayerControllerComponent::ComputeCurrentLayer()
    {
        if (m_externalLayerModifier.GetState().empty()
            && m_baseModifier.empty())
        {
            m_currentStyle = m_baseLayering;
        }
        else if (!m_externalLayerModifier.GetState().empty())
        {
            m_currentStyle = AZStd::string::format("%s_%s", m_baseLayering.c_str(), m_externalLayerModifier.GetState().c_str());
        }
        else
        {
            m_currentStyle = AZStd::string::format("%s_%s", m_baseLayering.c_str(), m_baseModifier.c_str());
        }

        OnStylesChanged();
    }
}