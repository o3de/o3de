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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/std/parallel/mutex.h>

#include <DebugDraw/DebugDrawBus.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

// DebugDraw components
#include "DebugDrawLineComponent.h"
#include "DebugDrawRayComponent.h"
#include "DebugDrawSphereComponent.h"
#include "DebugDrawObbComponent.h"
#include "DebugDrawTextComponent.h"

#ifdef DEBUGDRAW_GEM_EDITOR
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#endif // DEBUGDRAW_GEM_EDITOR

namespace DebugDraw
{
    // DebugDraw elements that don't have corresponding component representations yet
    class DebugDrawAabbElement
    {
    public:
        AZ_CLASS_ALLOCATOR(DebugDrawAabbElement, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(DebugDrawAabbElement, "{3B3E45AC-95B5-477F-BC34-58765A031BF1}");

        AZ::EntityId            m_targetEntityId;
        AZ::Aabb                m_aabb;
        float                   m_duration;
        AZ::ScriptTimePoint     m_activateTime;
        AZ::Color               m_color;
        AZ::Vector3             m_worldLocation;
        AZ::ComponentId         m_owningEditorComponent;

        DebugDrawAabbElement()
            : m_duration(0.f)
            , m_color(1.0f, 1.0f, 1.0f, 1.0f)
            , m_worldLocation(AZ::Vector3::CreateZero())
            , m_owningEditorComponent(AZ::InvalidComponentId)
        {
        }
    };


    class DebugDrawSystemComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AZ::EntityBus::MultiHandler
        , protected DebugDrawRequestBus::Handler
        , protected DebugDrawInternalRequestBus::Handler

#ifdef DEBUGDRAW_GEM_EDITOR
        , protected AzToolsFramework::EditorEntityContextNotificationBus::Handler
#endif // DEBUGDRAW_GEM_EDITOR
    {
    public:
        AZ_COMPONENT(DebugDrawSystemComponent, "{48D54C3C-F284-43A5-B070-106F2CEB7154}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

#ifdef DEBUGDRAW_GEM_EDITOR
        // EditorEntityContextNotificationBus interface implementation
        void OnStopPlayInEditor() override;
#endif // DEBUGDRAW_GEM_EDITOR

    protected:
        // DebugDrawRequestBus interface implementation
        void DrawAabb(const AZ::Aabb& aabb, const AZ::Color& color, float duration) override;
        void DrawAabbOnEntity(const AZ::EntityId& targetEntity, const AZ::Aabb& aabb, const AZ::Color& color, float duration) override;
        void DrawLineBatchLocationToLocation(const AZStd::vector<DebugDraw::DebugDrawLineElement>& lineBatch) override;
        void DrawLineLocationToLocation(const AZ::Vector3& startLocation, const AZ::Vector3& endLocation, const AZ::Color& color, float duration) override;
        void DrawLineEntityToLocation(const AZ::EntityId& startEntity, const AZ::Vector3& endLocation, const AZ::Color& color, float duration) override;
        void DrawLineEntityToEntity(const AZ::EntityId& startEntity, const AZ::EntityId& endEntity, const AZ::Color& color, float duration) override;
        void DrawObb(const AZ::Obb& obb, const AZ::Color& color, float duration) override;
        void DrawObbOnEntity(const AZ::EntityId& targetEntity, const AZ::Obb& obb, const AZ::Color& color, float duration) override;
        void DrawRayLocationToDirection(const AZ::Vector3& worldLocation, const AZ::Vector3& worldDirection, const AZ::Color& color, float duration) override;
        void DrawRayEntityToDirection(const AZ::EntityId& startEntity, const AZ::Vector3& worldDirection, const AZ::Color& color, float duration) override;
        void DrawRayEntityToEntity(const AZ::EntityId& startEntity, const AZ::EntityId& endEntity, const AZ::Color& color, float duration) override;
        void DrawSphereAtLocation(const AZ::Vector3& worldLocation, float radius, const AZ::Color& color, float duration) override;
        void DrawSphereOnEntity(const AZ::EntityId& targetEntity, float radius, const AZ::Color& color, float duration) override;
        void DrawTextAtLocation(const AZ::Vector3& worldLocation, const AZStd::string& text, const AZ::Color& color, float duration) override;
        void DrawTextOnEntity(const AZ::EntityId& targetEntity, const AZStd::string& text, const AZ::Color& color, float duration) override;
        void DrawTextOnScreen(const AZStd::string& text, const AZ::Color& color, float duration) override;

        // DebugDrawInternalRequestBus interface implementation
        void RegisterDebugDrawComponent(AZ::Component* component) override;
        void UnregisterDebugDrawComponent(AZ::Component* component) override;

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override { return AZ::ComponentTickBus::TICK_DEFAULT; }

        // EntityBus
        void OnEntityDeactivated(const AZ::EntityId& entityId) override;

        // Ticking functions for drawing debug elements
        void OnTickAabbs();
        void OnTickLines();
        void OnTickObbs();
        void OnTickRays();
        void OnTickSpheres();
        void OnTickText();

        // Element creation functions, used when DebugDraw components register themselves
        void CreateAabbEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawAabbElement& element);
        void CreateLineEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawLineElement& element);
        void CreateObbEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawObbElement& element);
        void CreateRayEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawRayElement& element);
        void CreateSphereEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawSphereElement& element);
        void CreateTextEntryForComponent(const AZ::EntityId& componentEntityId, const DebugDrawTextElement& element);

        template <typename F>
        void removeExpiredDebugElementsFromVector(AZStd::vector<F>& vectorToExpire);

        AZStd::vector<DebugDrawAabbElement> m_activeAabbs;
        AZStd::mutex m_activeAabbsMutex;
        AZStd::vector<DebugDrawLineElement> m_activeLines;
        AZStd::mutex m_activeLinesMutex;
        AZStd::vector<DebugDrawObbElement> m_activeObbs;
        AZStd::mutex m_activeObbsMutex;
        AZStd::vector<DebugDrawRayElement> m_activeRays;
        AZStd::mutex m_activeRaysMutex;
        AZStd::vector<DebugDrawSphereElement> m_activeSpheres;
        AZStd::mutex m_activeSpheresMutex;
        AZStd::vector<DebugDrawTextElement> m_activeTexts;
        AZStd::mutex m_activeTextsMutex;

        double m_currentTime;

        AZStd::vector<Vec3> m_batchPoints;
        AZStd::vector<ColorB> m_batchColors;
    };
}