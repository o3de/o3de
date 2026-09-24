/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DebugDrawOctreeComponent.h"

#include <AzCore/Console/Console.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <AzFramework/Visibility/IVisibilitySystem.h>

#include <DebugDraw/DebugDrawBus.h>

AZ_CVAR(
    bool, bg_octreeRuntimeDebugDraw, true, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If true, DebugDrawOctreeComponent draws the visibility octree node bounds (orange) every frame");
AZ_CVAR(
    bool, bg_octreeRuntimeDebugDrawEntries, true, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If true, DebugDrawOctreeComponent also draws the bounding volume of every visibility entry (cyan)");

namespace DebugDraw
{
    static constexpr AZ::u8 BoxEdgeIndices[12][2] = {
        {0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7},
        {7, 6}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };

    void DebugDrawOctreeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DebugDrawOctreeComponent, AZ::Component>()->Version(0);

            if (auto* editContext = serialize->GetEditContext())
            {
                editContext->Class<DebugDrawOctreeComponent>(
                               QT_TRANSLATE_NOOP("DebugDraw", "DebugDraw Octree"),
                               QT_TRANSLATE_NOOP(
                                   "DebugDraw",
                                   "Draws the visibility octree node bounds (orange) and the bounding volume of every visibility "
                                   "entry (cyan) every frame, also while the game is running. Toggle with the "
                                   "bg_octreeRuntimeDebugDraw console variable."))
                           ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                           ->Attribute(AZ::Edit::Attributes::Category, "Debugging");
            }
        }
    }

    void DebugDrawOctreeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DebugDrawOctreeService"));
    }

    void DebugDrawOctreeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("DebugDrawOctreeService"));
    }

    void DebugDrawOctreeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void DebugDrawOctreeComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void DebugDrawOctreeComponent::Init()
    {
    }

    void DebugDrawOctreeComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
    }

    void DebugDrawOctreeComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void DebugDrawOctreeComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        DrawVisibilityOctree();
    }

    void DebugDrawOctreeComponent::DrawVisibilityOctree()
    {
        if (!bg_octreeRuntimeDebugDraw)
        {
            return;
        }

        AzFramework::IVisibilitySystem* const visibilitySystem = AZ::Interface<AzFramework::IVisibilitySystem>::Get();
        if (!visibilitySystem)
        {
            return;
        }

        AzFramework::IVisibilityScene* const visibilityScene = visibilitySystem->GetDefaultVisibilityScene();
        if (!visibilityScene)
        {
            return;
        }

        visibilityScene->EnumerateNoCull(
            [](const AzFramework::IVisibilityScene::NodeData& nodeData)
            {
                DrawWireAabb(nodeData.m_bounds, AZ::Colors::Orange);

                if (bg_octreeRuntimeDebugDrawEntries)
                {
                    for (const auto* visibilityEntry : nodeData.m_entries)
                    {
                        DrawWireAabb(visibilityEntry->m_boundingVolume, AZ::Colors::Cyan);
                    }
                }
            });

        const auto statsText =
            AZStd::string::format("OctreeDebug Visibility Octree Entries: %u",
                                  visibilityScene->GetEntryCount());
        DebugDrawRequestBus::Broadcast(&DebugDrawRequestBus::Events::DrawTextOnScreen, statsText, AZ::Colors::White,
                                       0.0f);
    }

    void DebugDrawOctreeComponent::DrawWireAabb(const AZ::Aabb& aabb, const AZ::Color& color)
    {
        if (!aabb.IsValid())
        {
            return;
        }

        const AZ::Vector3 min = aabb.GetMin();
        const AZ::Vector3 max = aabb.GetMax();
        const AZ::Vector3 corners[8] = {
            min,
            AZ::Vector3(max.GetX(), min.GetY(), min.GetZ()),
            AZ::Vector3(min.GetX(), max.GetY(), min.GetZ()),
            AZ::Vector3(max.GetX(), max.GetY(), min.GetZ()),
            AZ::Vector3(min.GetX(), min.GetY(), max.GetZ()),
            AZ::Vector3(max.GetX(), min.GetY(), max.GetZ()),
            AZ::Vector3(min.GetX(), max.GetY(), max.GetZ()),
            max
        };

        for (const AZ::u8 (&edge)[2] : BoxEdgeIndices)
        {
            DebugDrawRequestBus::Broadcast(
                &DebugDrawRequestBus::Events::DrawLineLocationToLocation,
                corners[edge[0]],
                corners[edge[1]],
                color,
                0.0f);
        }
    }
} // namespace DebugDraw
