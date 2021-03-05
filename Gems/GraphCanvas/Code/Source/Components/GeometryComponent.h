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
            provided.push_back(AZ_CRC("GraphCanvas_GeometryService", 0x80981600));
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
        ////

        // VisualNotificationBus
        void OnItemChange(const AZ::EntityId& entityId, QGraphicsItem::GraphicsItemChange, const QVariant&) override;
        void OnPositionAnimateBegin(const AZ::Vector2& targetPoint) override;
        void OnPositionAnimateEnd() override;
        ////

        // EntitySaveDataRequestBus
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const override;
        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) override;
        ////

    private:

        GeometrySaveData m_saveData;
        AZ::Vector2 m_oldPosition;

        bool m_animating;
        AZ::Vector2 m_animatingPosition;
    };
}
