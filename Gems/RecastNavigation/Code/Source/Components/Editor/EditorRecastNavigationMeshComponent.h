/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EditorRecastNavigationMeshConfig.h"
#include "Components/RecastNavigationDebugDraw.h"
#include "Components/RecastNavigationMeshConfig.h"

#include <DetourNavMesh.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <Components/RecastNavigationMeshCommon.h>
#include <ToolsComponents/EditorComponentBase.h>

namespace RecastNavigation
{
    //! Calculates a navigation mesh with the triangle data provided by @RecastNavigationSurveyorComponent.
    //! Provides APIs to find a path between two entities or two world positions.
    class EditorRecastNavigationMeshComponent final
        : public AzToolsFramework::Components::EditorComponentBase
        , public RecastNavigationMeshCommon
    {
    public:
        AZ_EDITOR_COMPONENT(EditorRecastNavigationMeshComponent, "{22D516D4-C98D-4783-85A4-1ABE23CAB4D4}", AzToolsFramework::Components::EditorComponentBase);

        EditorRecastNavigationMeshComponent();
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        // EditorComponentBase interface implementation
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        //! Flag used for button placement
        bool m_updateNavigationMeshComponentFlag = false;

        RecastNavigationMeshConfig m_meshConfig;

        EditorRecastNavigationMeshConfig m_meshEditorConfig;
        AZ::Event<int>::Handler m_updateFrequencyHandler;
        void OnUpdatedPeriod(int newUpdatePeriodInSeconds);

        AZStd::mutex m_navigationMeshMutex;
        bool m_updatingNavMeshInProgress = false;
        AZ::TaskDescriptor m_taskDescriptor{ "UpdatingNavMesh", "RecastNavigation" };
        AZStd::unique_ptr<AZ::TaskGraphEvent> m_taskGraphEvent;
        AZStd::unique_ptr<AZ::TaskExecutor> m_navigationTaskExecutor;

        AZ::Crc32 UpdatedNavigationMeshInEditor();

        AZ::ScheduledEvent m_tickEvent;
        void OnTick();

        AZ::ScheduledEvent m_updateNavMeshEvent;
        void OnUpdateNavMeshEvent();
    };
} // namespace RecastNavigation
