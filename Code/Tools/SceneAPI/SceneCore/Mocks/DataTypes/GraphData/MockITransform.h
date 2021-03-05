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

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>

namespace AZ
{
    class Transform;
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class MockITransform
                : public ITransform
            {
            public:
                AZ_RTTI(MockITransform, "{1D4A832F-823C-4A80-97E1-A95D1B2BF18F}", ITransform);

                virtual ~MockITransform() override = default;

                MOCK_METHOD0(GetMatrix,
                    AZ::SceneAPI::DataTypes::MatrixType&());
                MOCK_CONST_METHOD0(GetMatrix,
                    const AZ::SceneAPI::DataTypes::MatrixType&());
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
