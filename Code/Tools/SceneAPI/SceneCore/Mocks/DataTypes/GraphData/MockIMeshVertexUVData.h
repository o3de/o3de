#pragma once

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
