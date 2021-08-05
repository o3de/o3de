#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMeshAdvancedRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class MockIMeshAdvancedRule
                : public IMeshAdvancedRule
            {
            public:
                AZ_RTTI(MockIMeshAdvancedRule, "{6F67873D-778C-4D11-8818-B1906E60B67D}", IMeshAdvancedRule);

                virtual ~MockIMeshAdvancedRule() override = default;

                MOCK_CONST_METHOD0(Use32bitVertices,
                    bool());
                MOCK_CONST_METHOD0(MergeMeshes, 
                    bool());
                MOCK_CONST_METHOD0(GetVertexColorStreamName,
                    const AZStd::string&());
                MOCK_CONST_METHOD0(IsVertexColorStreamDisabled,
                    bool());
                MOCK_CONST_METHOD1(GetUVStreamName, 
                    AZStd::string&(size_t index));
                MOCK_CONST_METHOD1(IsUVStreamDisabled,
                    bool(size_t index));
                MOCK_CONST_METHOD0(GetUVStreamCount,
                    size_t());
                MOCK_CONST_METHOD0(UseCustomNormals,
                    bool());
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
