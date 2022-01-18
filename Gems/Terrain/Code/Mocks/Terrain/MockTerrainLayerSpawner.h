/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

namespace UnitTest
{
    class MockTerrainLayerSpawnerComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockTerrainLayerSpawnerComponent, "{9F27C980-9826-4063-86D8-E981C1E842A3}");

        static void Reflect([[maybe_unused]] AZ::ReflectContext* context)
        {
        }

        void Activate() override
        {
        }

        void Deactivate() override
        {
        }
      
    private: 
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("TerrainAreaService"));
        }
    };
    
} //namespace UnitTest
