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
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
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
    static constexpr AZStd::array<AZStd::array<AZ::u8, 2>, 12> BoxEdgeIndices = {{
        {0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7},
        {7, 6}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
    }};

    static void AppendWireAabb(const AZ::Aabb& aabb, const AZ::Color& color, AZStd::vector<DebugDrawLineElement>& lines)
    {
        if (!aabb.IsValid())
        {
            return;
        }

        const AZ::Vector3 min = aabb.GetMin();
        const AZ::Vector3 max = aabb.GetMax();
        const AZStd::array<AZ::Vector3, 8> corners = {
            min,
            AZ::Vector3(max.GetX(), min.GetY(), min.GetZ()),
            AZ::Vector3(min.GetX(), max.GetY(), min.GetZ()),
            AZ::Vector3(max.GetX(), max.GetY(), min.GetZ()),
            AZ::Vector3(min.GetX(), min.GetY(), max.GetZ()),
            AZ::Vector3(max.GetX(), min.GetY(), max.GetZ()),
            AZ::Vector3(min.GetX(), max.GetY(), max.GetZ()),
            max,
        };

        for (const auto& edge : BoxEdgeIndices)
        {
            DebugDrawLineElement& line = lines.emplace_back();
            line.m_startWorldLocation = corners[edge[0]];
            line.m_endWorldLocation = corners[edge[1]];
            line.m_color = color;
        }
    }

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

        AZStd::vector<AZ::Aabb> nodeBounds;
        AZStd::vector<AZ::Aabb> entryBounds;
        AZ::u32 entryCount = 0;
        const bool drawEntries = bg_octreeRuntimeDebugDrawEntries;
        const auto appendNode = [&](const AzFramework::IVisibilityScene::NodeData& nodeData)
        {
            nodeBounds.push_back(nodeData.m_bounds);
            entryCount += static_cast<AZ::u32>(nodeData.m_entries.size());

            if (drawEntries)
            {
                for (const auto* visibilityEntry : nodeData.m_entries)
                {
                    entryBounds.push_back(visibilityEntry->m_boundingVolume);
                }
            }
        };

        visibilityScene->Enumerate(appendNode);

        AZStd::vector<DebugDrawLineElement> lines;
        lines.reserve(BoxEdgeIndices.size() * (nodeBounds.size() + entryBounds.size()));
        for (const AZ::Aabb& bounds : nodeBounds)
        {
            AppendWireAabb(bounds, AZ::Colors::Orange, lines);
        }
        for (const AZ::Aabb& bounds : entryBounds)
        {
            AppendWireAabb(bounds, AZ::Colors::Cyan, lines);
        }
        DebugDrawRequestBus::Broadcast(&DebugDrawRequestBus::Events::DrawLineBatchLocationToLocation, lines);

        const auto statsText =
            AZStd::string::format("OctreeDebug Visibility Octree Entries: %u", entryCount);
        DebugDrawRequestBus::Broadcast(&DebugDrawRequestBus::Events::DrawTextOnScreen, statsText, AZ::Colors::White,
                                       0.0f);
    }
} // namespace DebugDraw
