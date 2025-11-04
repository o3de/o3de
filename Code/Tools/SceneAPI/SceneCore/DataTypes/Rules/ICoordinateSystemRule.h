/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/Utilities/CoordinateSystemConverter.h>

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

        // advanced coordinate settings
        virtual bool GetUseAdvancedData() const = 0;
        virtual const AZStd::string& GetOriginNodeName() const = 0;
        virtual const Quaternion& GetRotation() const = 0;
        virtual const Vector3& GetTranslation() const = 0;
        virtual float GetScale() const = 0;
    };
} // namespace AZ::SceneAPI::DataTypes
