#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gmock/gmock.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class MockIMeshVertexUVData
                : public IMeshVertexUVData
            {
            public:
                AZ_RTTI(MockIMeshVertexUVData, "{34528E85-2EE0-4999-B838-113BEDB1A258}", IMeshVertexUVData);

                ~MockIMeshVertexUVData() override = default;

                MOCK_CONST_METHOD0(GetCustomName,
                    const AZ::Name& ());

                MOCK_CONST_METHOD0(GetCount,
                    size_t());
                MOCK_CONST_METHOD1(GetUV,
                    const AZ::Vector2&(size_t index));
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
