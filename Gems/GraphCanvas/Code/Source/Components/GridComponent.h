/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            provided.push_back(AZ_CRC_CE("GraphCanvas_GridService"));
            provided.push_back(AZ_CRC_CE("GraphCanvas_SceneMemberService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& /*dependent*/)
        {
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& /*required*/)
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
        ////

    protected:

        AZ::EntityId m_scene;
        AZ::Vector2 m_majorPitch;
        AZ::Vector2 m_minorPitch;
        int m_minimumVisualPitch;
    };
}
