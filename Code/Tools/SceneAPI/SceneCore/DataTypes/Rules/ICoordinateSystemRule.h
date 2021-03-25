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

#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/Utilities/CoordinateSystemConverter.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::SceneAPI::DataTypes
{
    class ICoordinateSystemRule
        : public IRule
    {
    public:
        AZ_RTTI(ICoordinateSystemRule, "{78136748-AC4B-4FC6-B9D8-C5820D67D29F}", IRule);

        enum CoordinateSystem : int
        {
            ZUpPositiveYForward = 0,
            ZUpNegativeYForward = 1
        };

        virtual ~ICoordinateSystemRule() override = default;

        virtual void UpdateCoordinateSystemConverter() = 0;

        virtual void SetTargetCoordinateSystem(CoordinateSystem targetCoordinateSystem) = 0;
        virtual CoordinateSystem GetTargetCoordinateSystem() const = 0;

        virtual const CoordinateSystemConverter& GetCoordinateSystemConverter() const = 0;
    };
} // namespace AZ::SceneAPI::DataTypes
