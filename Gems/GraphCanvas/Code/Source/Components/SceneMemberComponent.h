
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
#include <AzCore/Component/EntityBus.h>

#include <GraphCanvas/Components/SceneBus.h>

namespace GraphCanvas
{
    //! Manages all of the state required by scene members
    class SceneMemberComponent
        : public AZ::Component
        , public SceneMemberRequestBus::Handler
        , public AZ::EntityBus::Handler
    {
    public:
        AZ_COMPONENT(SceneMemberComponent, "{C431F18F-22FB-4D3E-8E1A-2F8E4E30F7FB}");

        static void Reflect(AZ::ReflectContext* reflectContext);        

        SceneMemberComponent();
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

        bool LockForExternalMovement(const AZ::EntityId& sceneMemberId) override;
        void UnlockForExternalMovement(const AZ::EntityId& sceneMemberId) override;
        ////

        // EntityBus
        void OnEntityExists(const AZ::EntityId&) override;
        ////

    private:

        AZ::EntityId m_lockedSceneMember;
        AZ::EntityId m_sceneId;
    };
}