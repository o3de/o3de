/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace Physics
{
    namespace ReflectionUtils
    {
        void ReflectPhysicsApi(AZ::ReflectContext* context);
    }

    namespace Utils
    {
        //! Reusable unordered set of string names.
        using NameSet = AZStd::unordered_set<AZStd::string>;

        //! Helper routine for transforming a diagonal matrix from local space to world space.
        AZ_INLINE AZ::Matrix3x3 DiagonalMatrixLocalToWorld(const AZ::Vector3& matrixDiagonal, const AZ::Matrix3x3& rotationToWorld)
        {
            return rotationToWorld * AZ::Matrix3x3::CreateDiagonal(matrixDiagonal) * rotationToWorld.GetTranspose();
        }

        //! Makes the input string unique for the input set
        void MakeUniqueString(const AZStd::unordered_set<AZStd::string>& stringSet
            , AZStd::string& stringInOut
            , AZ::u64 maxStringLength);

        //! Returns true if the tag matches the filter tag, or the filter tag is empty
        bool FilterTag(AZ::Crc32 tag, AZ::Crc32 filter);

        //! Returns true if the scale is uniform.
        bool HasUniformScale(const AZ::Vector3& scale);
    }
}
