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
            provided.push_back(AZ_CRC("GraphCanvas_SlotFilterService", 0x62bc9d76));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_SceneMemberService", 0xe9759a2d));
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