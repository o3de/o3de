#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
