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

#include <AzTest/AzTest.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class MockIMeshVertexColorData
                : public IMeshVertexColorData
            {
            public:
                AZ_RTTI(MockIMeshVertexColorData, "{15AD4D4A-BFCC-41C3-B9BB-F7EFBA0AE0CC}", IMeshVertexColorData)

                virtual ~MockIMeshVertexColorData() = default;

                MOCK_CONST_METHOD0(GetCustomName,
                    const AZ::Name& ());

                MOCK_CONST_METHOD0(GetCount, 
                    size_t());
                MOCK_CONST_METHOD1(GetColor,
                    const Color&(size_t index));
            };
        }  // namespace DataTypes
    }  //namespace SceneAPI
}  // namespace AZ
