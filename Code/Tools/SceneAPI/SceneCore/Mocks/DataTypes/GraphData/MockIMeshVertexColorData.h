#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
