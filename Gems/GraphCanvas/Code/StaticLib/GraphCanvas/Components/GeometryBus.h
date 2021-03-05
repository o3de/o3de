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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Types/EntitySaveData.h>

namespace GraphCanvas
{
    //! GeometryRequests
    //! Informational requests serviced by the Geometry component.
    class GeometryRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        // Multiple handlers. Events received in defined order.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Get the position of the entity in scene space.
        virtual AZ::Vector2 GetPosition() const = 0;

        //! Set the entity's position in scene space.
        virtual void SetPosition(const AZ::Vector2&) = 0;

        virtual void SignalBoundsChanged() = 0;
    };

    using GeometryRequestBus = AZ::EBus<GeometryRequests>;

    //! GeometryNorifications
    //! Notifications regarding changes to an entity's geometry.
    class GeometryNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Emitted when the position of the entity changes in the scene.
        virtual void OnPositionChanged(const AZ::EntityId& /*targetEntity*/, const AZ::Vector2& /*position*/) {}

        virtual void OnBoundsChanged() {}
    };

    using GeometryNotificationBus = AZ::EBus<GeometryNotifications>;

    class GeometrySaveData
        : public ComponentSaveData
    {
    public:
        AZ_RTTI(GeometrySaveData, "{7CC444B1-F9B3-41B5-841B-0C4F2179F111}", ComponentSaveData);
        AZ_CLASS_ALLOCATOR(GeometrySaveData, AZ::SystemAllocator, 0);

        GeometrySaveData()
            : m_position(0,0)
        {

        }

        ~GeometrySaveData() = default;

        AZ::Vector2 m_position;
    };
}