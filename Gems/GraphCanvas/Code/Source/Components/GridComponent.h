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
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/SceneBus.h>

namespace GraphCanvas
{
    //! A component that gives a visual coordinates and a size.
    class GridComponent
        : public AZ::Component
        , public GridRequestBus::Handler
        , public SceneMemberRequestBus::Handler
    {
    public:
        AZ_COMPONENT(GridComponent, "{A9EFFA4B-1002-4837-B3EA-C596A14B2172}");
        static void Reflect(AZ::ReflectContext*);

        static AZ::Entity* CreateDefaultEntity();

        GridComponent();
        ~GridComponent() = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("GraphCanvas_GridService", 0x58f7e1d8));
            provided.push_back(AZ_CRC("GraphCanvas_SceneMemberService", 0xe9759a2d));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }

        void Activate() override;
        void Deactivate() override;
        ////

        // GridRequestBus
        void SetMajorPitch(const AZ::Vector2& pitch) override;
        AZ::Vector2 GetMajorPitch() const override;

        void SetMinorPitch(const AZ::Vector2& position) override;
        AZ::Vector2 GetMinorPitch() const override;

        void SetMinimumVisualPitch(int minimum) override;
        int GetMinimumVisualPitch() const override;
        ////

        // SceneMemberRequestBus
        void SetScene(const AZ::EntityId& sceneId) override;
        void ClearScene(const AZ::EntityId& oldSceneId) override;
        void SignalMemberSetupComplete() override;

        AZ::EntityId GetScene() const override;

        bool LockForExternalMovement(const AZ::EntityId& sceneMemberId) override;
        void UnlockForExternalMovement(const AZ::EntityId& sceneMemberId) override;
        ////

    protected:

        AZ::EntityId m_scene;
        AZ::Vector2 m_majorPitch;
        AZ::Vector2 m_minorPitch;
        int m_minimumVisualPitch;
    };
}
