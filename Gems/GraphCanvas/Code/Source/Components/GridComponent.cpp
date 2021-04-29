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

#include <AzCore/Serialization/EditContext.h>

#include <Components/GridComponent.h>

#include <Components/GridVisualComponent.h>
#include <Components/StylingComponent.h>

namespace GraphCanvas
{

    //////////////////
    // GridComponent
    //////////////////

    void GridComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<GridComponent, AZ::Component>()
            ->Version(1)
            ->Field("MajorPitch", &GridComponent::m_majorPitch)
            ->Field("MinorPitch", &GridComponent::m_minorPitch)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<GridComponent>("Grid", "The grid's properties")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "Class attributes for the grid")
            ->DataElement(AZ::Edit::UIHandlers::Default, &GridComponent::m_majorPitch, "Tooltip", "The \"major pitch\" of the grid")
            ->DataElement(AZ::Edit::UIHandlers::Default, &GridComponent::m_minorPitch, "Tooltip", "The \"minor pitch\" of the grid")
            ;
    }

    GridComponent::GridComponent()
        : m_majorPitch(100, 100)
        , m_minorPitch(20, 20)
        , m_minimumVisualPitch(5)
    {
    }

    AZ::Entity* GridComponent::CreateDefaultEntity()
    {
        // Create this Node's entity.
        AZ::Entity* entity = aznew AZ::Entity("Scene Grid");
        
        entity->CreateComponent<GridComponent>();
        entity->CreateComponent<GridVisualComponent>();
        entity->CreateComponent<StylingComponent>(Styling::Elements::Graph);

        entity->Init();
        entity->Activate();

        return entity;
    }

    void GridComponent::Activate()
    {
        SceneMemberRequestBus::Handler::BusConnect(GetEntityId());
        GridRequestBus::Handler::BusConnect(GetEntityId());
    }

    void GridComponent::Deactivate()
    {
        GridRequestBus::Handler::BusDisconnect();
        SceneMemberRequestBus::Handler::BusDisconnect();
    }

    void GridComponent::SetMajorPitch(const AZ::Vector2& pitch)
    {
        if (!pitch.IsClose(m_majorPitch))
        {
            m_majorPitch = pitch;
            GridNotificationBus::Event(GetEntityId(), &GridNotifications::OnMajorPitchChanged, m_majorPitch);
        }
    }

    AZ::Vector2 GridComponent::GetMajorPitch() const
    {
        return m_majorPitch;
    }

    void GridComponent::SetMinorPitch(const AZ::Vector2& pitch)
    {
        if (!pitch.IsClose(m_minorPitch))
        {
            m_minorPitch = pitch;
            GridNotificationBus::Event(GetEntityId(), &GridNotifications::OnMinorPitchChanged, m_minorPitch);
        }
    }

    AZ::Vector2 GridComponent::GetMinorPitch() const
    {
        return m_minorPitch;
    }

    void GridComponent::SetMinimumVisualPitch(int minimum)
    {
        if (minimum != m_minimumVisualPitch)
        {
            m_minimumVisualPitch = minimum;
            GridNotificationBus::Event(GetEntityId(), &GridNotifications::OnMinimumVisualPitchChanged, m_minimumVisualPitch);
        }
    }

    int GridComponent::GetMinimumVisualPitch() const
    {
        return m_minimumVisualPitch;
    }

    void GridComponent::SetScene(const AZ::EntityId& scene)
    {
        AZ_Assert(!m_scene.IsValid() && GetEntityId().IsValid(), "This grid is already in a scene (ID: %s)!", m_scene.ToString().data());

        m_scene = scene;
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnSceneSet, m_scene);
    }

    void GridComponent::ClearScene(const AZ::EntityId& /*oldScene*/)
    {
        AZ_Assert(m_scene.IsValid(), "This grid (ID: %s) is not in a scene!", GetEntityId().ToString().data());
        AZ_Assert(GetEntityId().IsValid(), "This grid (ID: %s) doesn't have an Entity!", GetEntityId().ToString().data());

        if (!m_scene.IsValid() || !GetEntityId().IsValid())
        {
            return;
        }

        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnRemovedFromScene, m_scene);
        m_scene.SetInvalid();
    }

    void GridComponent::SignalMemberSetupComplete()
    {
        SceneMemberNotificationBus::Event(GetEntityId(), &SceneMemberNotifications::OnMemberSetupComplete);
    }

    AZ::EntityId GridComponent::GetScene() const
    {
        return m_scene;
    }
}
