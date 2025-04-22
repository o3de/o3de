/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <GraphCanvas/Components/Connections/ConnectionFilters/ConnectionFilterBus.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/SceneBus.h>

namespace GraphCanvas
{
    class SlotConnectionFilterComponent
        : public AZ::Component
        , protected ConnectionFilterRequestBus::Handler
    {
    public:
        AZ_COMPONENT(SlotConnectionFilterComponent, "{6238C5B7-A1B5-442A-92FF-8BC94BB94385}");
        static void Reflect(AZ::ReflectContext* context);

        SlotConnectionFilterComponent();
        ~SlotConnectionFilterComponent() override;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("GraphCanvas_SlotFilterService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("GraphCanvas_SceneMemberService"));
        }

        void Activate() override;
        void Deactivate() override;
        ////

        // ConnectionFilterRequestBus
        void AddFilter(ConnectionFilter* slotFilter) override;
        bool CanConnectWith(const Endpoint& endpoint, const ConnectionMoveType& moveType) const override;
        ////

    private:

        AZStd::vector< ConnectionFilter* > m_filters;
    };
}
