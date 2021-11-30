
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>

namespace GraphCanvas
{
    //! Manages all of the state required by scene members
    class SceneMemberComponent
        : public AZ::Component
        , public SceneMemberRequestBus::Handler
        , public GroupableSceneMemberRequestBus::Handler
        , public AZ::EntityBus::Handler
    {
    public:
        AZ_COMPONENT(SceneMemberComponent, "{C431F18F-22FB-4D3E-8E1A-2F8E4E30F7FB}");

        static void Reflect(AZ::ReflectContext* reflectContext);

        SceneMemberComponent();
        explicit SceneMemberComponent(bool isGroupable);
        ~SceneMemberComponent() = default;

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // SceneMemberRequestBus
        void SetScene(const AZ::EntityId& sceneId) override;
        void ClearScene(const AZ::EntityId& sceneId) override;
        void SignalMemberSetupComplete() override;

        AZ::EntityId GetScene() const override;

        bool IsGrouped() const override;
        const AZ::EntityId& GetGroupId() const override;

        void RegisterToGroup(const AZ::EntityId& groupId) override;
        void UnregisterFromGroup(const AZ::EntityId& groupId) override;
        void RemoveFromGroup() override;
        ////

        // EntityBus
        void OnEntityExists(const AZ::EntityId&) override;
        ////

    private:

        bool m_isGroupable;
        
        AZ::EntityId m_sceneId;

        AZ::EntityId m_groupId;
    };
}
