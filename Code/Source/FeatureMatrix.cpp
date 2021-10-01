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

#include <Allocators.h>
#include <FeatureMatrix.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(FeatureMatrix, MotionMatchAllocator, 0)

    void FeatureMatrix::Clear()
    {
        resize(0, 0);
    }

    AZ::Vector3 FeatureMatrix::GetVector3(Index row, Index startColumn) const
    {
        return AZ::Vector3(
            coeff(row, startColumn + 0),
            coeff(row, startColumn + 1),
            coeff(row, startColumn + 2));
    }

    void FeatureMatrix::SetVector3(Index row, Index startColumn, const AZ::Vector3& value)
    {
        operator()(row, startColumn + 0) = value.GetX();
        operator()(row, startColumn + 1) = value.GetY();
        operator()(row, startColumn + 2) = value.GetZ();
    }

    size_t FeatureMatrix::CalcMemoryUsageInBytes() const
    {
        const size_t bytesPerValue = sizeof(Scalar);
        const size_t numValues = size();
        return numValues * bytesPerValue;
    }
} // namespace EMotionFX::MotionMatching