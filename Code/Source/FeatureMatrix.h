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

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#pragma warning (push, 1)
#pragma warning (disable:4834) // C4834: discarding return value of function with 'nodiscard' attribute
#pragma warning (disable:5031) // #pragma warning(pop): likely mismatch, popping warning state pushed in different file
#pragma warning (disable:4702) // warning C4702: unreachable code
#pragma warning (disable:4723) // warning C4723: potential divide by 0
#include "../../3rdParty/eigen-3.3.9/Eigen/Dense"
#pragma warning (pop)

namespace EMotionFX::MotionMatching
{
    // Features are stored in columns, each row represents a frame
    // ColumnMajor (default in Eigen as well): Row components next to each other in memory for cache-optimized feature access for a given frame
    using FeatureMatrixType = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>;

    class FeatureMatrix
        : public FeatureMatrixType
    {
    public:
        AZ_RTTI(FeatureMatrix, "{E063C9CB-7147-4776-A6E0-98584DD93FEF}");
        AZ_CLASS_ALLOCATOR_DECL

        using Index = Eigen::Index;

        virtual ~FeatureMatrix() = default;

        void Clear();

        void SaveAsCsv(const AZStd::string& filename, const AZStd::vector<AZStd::string>& columnNames = {});

        size_t CalcMemoryUsageInBytes() const;

        AZ::Vector2 GetVector2(Index row, Index startColumn) const;
        void SetVector2(Index row, Index startColumn, const AZ::Vector2& value);

        AZ::Vector3 GetVector3(Index row, Index startColumn) const;
        void SetVector3(Index row, Index startColumn, const AZ::Vector3& value);
    };
} // namespace EMotionFX::MotionMatching
