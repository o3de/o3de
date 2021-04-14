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
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    class Vector2;
    class Name;
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMeshVertexUVData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IMeshVertexUVData, "{C45B2027-5D0A-400A-9689-88C9A27EFE57}", IGraphObject);

                virtual ~IMeshVertexUVData() override = default;

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

                virtual const AZ::Name& GetCustomName() const = 0;

                virtual size_t GetCount() const = 0;
                virtual const AZ::Vector2& GetUV(size_t index) const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ
