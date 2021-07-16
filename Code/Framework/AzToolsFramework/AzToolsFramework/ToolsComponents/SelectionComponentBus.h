/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef SELECTION_COMPONENT_BUS_H_INC
#define SELECTION_COMPONENT_BUS_H_INC

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#pragma once

namespace AzToolsFramework
{
    namespace Components
    {
        class BoundCollector
        {
        public:
            BoundCollector()
            {
                m_finalAabb = AZ::Aabb::CreateNull();
            }

            void operator=(const AZ::Aabb& other)
            {
                if (other.IsValid())
                {
                    m_finalAabb.AddAabb(other);
                }
            }

            AZ::Aabb m_finalAabb;
        };

        enum SelectionFlags
        {
            SF_Unselected = 0,
            SF_Selected = 1,
            SF_Primary = 2,
        };

        //////////////////////////////////////////////////////////////////////////
        // messages FOR transform change notification.
        // selection components connect to this bus by Entity ID.
        //////////////////////////////////////////////////////////////////////////
        class SelectionComponentMessages
            : public AZ::EBusTraits
        {
        public:

            using Bus = AZ::EBus<SelectionComponentMessages>;

            //////////////////////////////////////////////////////////////////////////
            // Bus configuration
            typedef AZ::EntityId BusIdType;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById; // components have an actual ID that they report back on
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // every listener registers unordered on this bus

            virtual AZ::Aabb GetSelectionBound() { return AZ::Aabb::CreateNull(); }
        };
    }
} // namespace AzToolsFramework

#endif
