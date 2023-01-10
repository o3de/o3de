/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/PaintBrush/PaintBrushSystemComponent.h>

namespace AzFramework
{
    void PaintBrushSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PaintBrushSystemComponent, AZ::Component>()
                ->Version(1)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PaintBrushSessionBus>("PaintBrushSessionBus")
                ->Attribute(AZ::Script::Attributes::Category, "PaintBrush")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "paintbrush")
                ->Event("StartPaintSession", &PaintBrushSessionBus::Events::StartPaintSession)
                ->Event("EndPaintSession", &PaintBrushSessionBus::Events::EndPaintSession)
                ->Event("BeginBrushStroke", &PaintBrushSessionBus::Events::BeginBrushStroke)
                ->Event("EndBrushStroke", &PaintBrushSessionBus::Events::EndBrushStroke)
                ->Event("IsInBrushStroke", &PaintBrushSessionBus::Events::IsInBrushStroke)
                ->Event("ResetBrushStrokeTracking", &PaintBrushSessionBus::Events::ResetBrushStrokeTracking)
                ->Event("PaintToLocation", &PaintBrushSessionBus::Events::PaintToLocation)
                ->Event("SmoothToLocation", &PaintBrushSessionBus::Events::SmoothToLocation);
        }
    }

    void PaintBrushSystemComponent::Activate()
    {
        PaintBrushSessionBus::Handler::BusConnect();
    }

    void PaintBrushSystemComponent::Deactivate()
    {
        PaintBrushSessionBus::Handler::BusDisconnect();
    }

    AZ::Uuid PaintBrushSystemComponent::StartPaintSession(const AZ::EntityId& entityId)
    {
        AZ::EntityComponentIdPair entityComponentPair(AZ::EntityId(AZ::EntityId::InvalidEntityId), AZ::InvalidComponentId);

        // To find the paintable component associated with this entity id, visit every paintbrush notification bus handler
        // and look for the first one that matches the entity id. 
        AzFramework::PaintBrushNotificationBus::EnumerateHandlers(
            [entityId, &entityComponentPair]([[maybe_unused]]AzFramework::PaintBrushNotifications* handler) -> bool
            {
                auto handlerEntityComponentPair = *(AzFramework::PaintBrushNotificationBus::GetCurrentBusId());

                if (handlerEntityComponentPair.GetEntityId() == entityId)
                {
                    entityComponentPair = handlerEntityComponentPair;
                    return false;
                }

                // Haven't found a match yet, so keep enumerating.
                return true;
            });

        // If we found a match, create a new paint session for this entity/component pair.
        if (entityComponentPair.GetEntityId().IsValid())
        {
            auto paintBrush = AZStd::make_shared<AzFramework::PaintBrush>(entityComponentPair);
            auto sessionId = AZ::Uuid::CreateRandom();

            paintBrush->BeginPaintMode();

            m_brushSessions.emplace(sessionId, AZStd::move(paintBrush));
            return sessionId;
        }

        AZ_Error("PaintBrushSystemComponent", false, "Entity doesn't have a paintable component.");
        return AZ::Uuid::CreateNull();
    }

    void PaintBrushSystemComponent::EndPaintSession(const AZ::Uuid& sessionId)
    {
        auto paintBrushItr = m_brushSessions.find(sessionId);
        AZ_Error("PaintBrushSystemComponent", paintBrushItr != m_brushSessions.end(), "Paint Brush Session ID isn't valid.");

        if (paintBrushItr != m_brushSessions.end())
        {
            paintBrushItr->second->EndPaintMode();
            m_brushSessions.erase(sessionId);
        }
    }

    void PaintBrushSystemComponent::BeginBrushStroke(const AZ::Uuid& sessionId, const AzFramework::PaintBrushSettings& brushSettings)
    {
        auto paintBrushItr = m_brushSessions.find(sessionId);
        AZ_Error("PaintBrushSystemComponent", paintBrushItr != m_brushSessions.end(), "Paint Brush Session ID isn't valid.");

        if (paintBrushItr != m_brushSessions.end())
        {
            paintBrushItr->second->BeginBrushStroke(brushSettings);
        }
    }

    void PaintBrushSystemComponent::EndBrushStroke(const AZ::Uuid& sessionId)
    {
        auto paintBrushItr = m_brushSessions.find(sessionId);
        AZ_Error("PaintBrushSystemComponent", paintBrushItr != m_brushSessions.end(), "Paint Brush Session ID isn't valid.");

        if (paintBrushItr != m_brushSessions.end())
        {
            paintBrushItr->second->EndBrushStroke();
        }
    }

    bool PaintBrushSystemComponent::IsInBrushStroke(const AZ::Uuid& sessionId) const
    {
        auto paintBrushItr = m_brushSessions.find(sessionId);
        AZ_Error("PaintBrushSystemComponent", paintBrushItr != m_brushSessions.end(), "Paint Brush Session ID isn't valid.");

        if (paintBrushItr != m_brushSessions.end())
        {
            return paintBrushItr->second->IsInBrushStroke();
        }

        return false;
    }

    void PaintBrushSystemComponent::ResetBrushStrokeTracking(const AZ::Uuid& sessionId)
    {
        auto paintBrushItr = m_brushSessions.find(sessionId);
        AZ_Error("PaintBrushSystemComponent", paintBrushItr != m_brushSessions.end(), "Paint Brush Session ID isn't valid.");

        if (paintBrushItr != m_brushSessions.end())
        {
            paintBrushItr->second->ResetBrushStrokeTracking();
        }
    }

    void PaintBrushSystemComponent::PaintToLocation(
        const AZ::Uuid& sessionId, const AZ::Vector3& brushCenter, const AzFramework::PaintBrushSettings& brushSettings)
    {
        auto paintBrushItr = m_brushSessions.find(sessionId);
        AZ_Error("PaintBrushSystemComponent", paintBrushItr != m_brushSessions.end(), "Paint Brush Session ID isn't valid.");

        if (paintBrushItr != m_brushSessions.end())
        {
            paintBrushItr->second->PaintToLocation(brushCenter, brushSettings);
        }
    }

    void PaintBrushSystemComponent::SmoothToLocation(
        const AZ::Uuid& sessionId, const AZ::Vector3& brushCenter, const AzFramework::PaintBrushSettings& brushSettings)
    {
        auto paintBrushItr = m_brushSessions.find(sessionId);
        AZ_Error("PaintBrushSystemComponent", paintBrushItr != m_brushSessions.end(), "Paint Brush Session ID isn't valid.");

        if (paintBrushItr != m_brushSessions.end())
        {
            paintBrushItr->second->SmoothToLocation(brushCenter, brushSettings);
        }
    }

} // namespace AzFramework
