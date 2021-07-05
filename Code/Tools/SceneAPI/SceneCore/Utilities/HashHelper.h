/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Matrix3x4.h>

namespace AZStd
{
    template<>
    struct hash<AZ::Vector2>
    {
        using result_type = AZStd::size_t;

        result_type operator()(const AZ::Vector2& value) const
        {
            result_type hash = 0;

            hash_combine(hash, value.GetX());
            hash_combine(hash, value.GetY());

            return hash;
        }
    };

    template<>
    struct hash<AZ::Vector3>
    {
        using result_type = AZStd::size_t;

        result_type operator()(const AZ::Vector3& value) const
        {
            result_type hash = 0;

            hash_combine(hash, value.GetX());
            hash_combine(hash, value.GetY());
            hash_combine(hash, value.GetZ());

            return hash;
        }
    };

    template<>
    struct hash<AZ::Vector4>
    {
        using result_type = AZStd::size_t;

        result_type operator()(const AZ::Vector4& value) const
        {
            result_type hash = 0;

            hash_combine(hash, value.GetX());
            hash_combine(hash, value.GetY());
            hash_combine(hash, value.GetZ());
            hash_combine(hash, value.GetW());

            return hash;
        }
    };

    template<>
    struct hash<AZ::Matrix3x4>
    {
        using result_type = AZStd::size_t;

        result_type operator()(const AZ::Matrix3x4& value) const
        {
            result_type hash = 0;

            AZ::Vector3 basisX;
            AZ::Vector3 basisY;
            AZ::Vector3 basisZ;
            AZ::Vector3 translation;

            value.GetBasisAndTranslation(&basisX, &basisY, &basisZ, &translation);

            hash_combine(hash, basisX);
            hash_combine(hash, basisY);
            hash_combine(hash, basisZ);
            hash_combine(hash, translation);

            return hash;
        }
    };
}
