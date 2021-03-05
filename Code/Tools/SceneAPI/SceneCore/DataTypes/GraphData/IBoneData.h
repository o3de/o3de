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
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IBoneData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IBoneData, "{9DC2BC55-BF0A-4849-9367-2138340768DE}", IGraphObject);

                virtual ~IBoneData() override = default;

                virtual const MatrixType& GetWorldTransform() const = 0;

                void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override
                {
                    output.Write("WorldTransform", GetWorldTransform());
                }
            };
        } // DataTypes
    } // SceneAPI
} // AZ
