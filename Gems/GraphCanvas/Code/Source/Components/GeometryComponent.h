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

#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Types/EntitySaveData.h>

namespace GraphCanvas
{
    //! A component that gives a visual coordinates.
    class GeometryComponent 
        : public AZ::Component
        , public GeometryRequestBus::Handler
        , public VisualNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public EntitySaveDataRequestBus::Handler
    {
    public:
        static const float IS_CLOSE_TOLERANCE;

        AZ_COMPONENT(GeometryComponent, "{DFD3FDE1-9856-41C9-AEF1-DD5B647A2B92}");
        static void Reflect(AZ::ReflectContext*);

        GeometryComponent();
        virtual ~GeometryComponent();

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("GraphCanvas_GeometryService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }
        ////

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& scene) override;
        ////
        
        // GeometryRequestBus
        AZ::Vector2 GetPosition() const override;
        void SetPosition(const AZ::Vector2& position) override;

        void SignalBoundsChanged() override;

        void SetIsPositionAnimating(bool animating) override;

        void SetAnimationTarget(const AZ::Vector2& targetPoint) override;
        ////

        // VisualNotificationBus
        void OnItemChange(const AZ::EntityId& entityId, QGraphicsItem::GraphicsItemChange, const QVariant&) override;
        ////

        // EntitySaveDataRequestBus
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const override;
        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) override;
        ////

    private:

        void ForceSetPosition(const AZ::Vector2& forcedPosition);

        bool IsAnimating() const;

        GeometrySaveData m_saveData;

        bool m_animating;

        AZ::Vector2 m_animatingPosition;
    };
}
