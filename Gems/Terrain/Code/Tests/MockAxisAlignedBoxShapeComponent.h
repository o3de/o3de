/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <gmock/gmock.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/MockShapes.h>

namespace UnitTest
{
    class MockAxisAlignedBoxShapeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockAxisAlignedBoxShapeComponent, "{77CBEED3-FAA3-4BC7-85A9-1A2BFC37BC2A}");

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
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ShapeService"));
            provided.push_back(AZ_CRC_CE("BoxShapeService"));
            provided.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
        }
    };
}
