/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

//#define O3DE_USE_EIGEN
#define O3DE_MM_FLOATTYPE float

#ifdef O3DE_USE_EIGEN
#pragma warning (push, 1)
#pragma warning (disable:4834) // C4834: discarding return value of function with 'nodiscard' attribute
#pragma warning (disable:5031) // #pragma warning(pop): likely mismatch, popping warning state pushed in different file
#pragma warning (disable:4702) // warning C4702: unreachable code
#pragma warning (disable:4723) // warning C4723: potential divide by 0
#include "../../3rdParty/eigen-3.3.9/Eigen/Dense"
#pragma warning (pop)
#endif

namespace EMotionFX::MotionMatching
{
    class FeatureSchema;

#ifdef O3DE_USE_EIGEN
    // Features are stored in columns, each row represents a frame
    // RowMajor: Store row components next to each other in memory for cache-optimized feature access for a given frame.
    using FeatureMatrixType = Eigen::Matrix<O3DE_MM_FLOATTYPE, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
#else
    /**
     * Small wrapper for a 2D matrix similar to the Eigen::Matrix.
     */
    class FeatureMatrixType
    {
    public:
        size_t size() const
        {
            return m_data.size();
        }

        size_t rows() const
        {
            return m_rowCount;
        }

        size_t cols() const
        {
            return m_columnCount;
        }

        void resize(size_t rowCount, size_t columnCount)
        {
            m_rowCount = rowCount;
            m_columnCount = columnCount;
            m_data.resize(m_rowCount * m_columnCount);
        }

        float& operator()(size_t row, size_t column)
        {
            return m_data[row * m_columnCount + column];
        }

        const float& operator()(size_t row, size_t column) const
        {
            return m_data[row * m_columnCount + column];
        }

        float coeff(size_t row, size_t column) const
        {
            return m_data[row * m_columnCount + column];
        }

    private:
        AZStd::vector<float> m_data;
        size_t m_rowCount = 0;
        size_t m_columnCount = 0;
    };
#endif

    class FeatureMatrix
        : public FeatureMatrixType
    {
    public:
        AZ_RTTI(FeatureMatrix, "{E063C9CB-7147-4776-A6E0-98584DD93FEF}");
        AZ_CLASS_ALLOCATOR_DECL

#ifdef O3DE_USE_EIGEN
        using Index = Eigen::Index;
#else
        using Index = size_t;
#endif

        virtual ~FeatureMatrix() = default;

        void Clear();

        void SaveAsCsv(const AZStd::string& filename, const AZStd::vector<AZStd::string>& columnNames = {});
        void SaveAsCsv(const AZStd::string& filename, const FeatureSchema* featureSchema);

        size_t CalcMemoryUsageInBytes() const;

        AZ::Vector2 GetVector2(Index row, Index startColumn) const;
        void SetVector2(Index row, Index startColumn, const AZ::Vector2& value);

        AZ::Vector3 GetVector3(Index row, Index startColumn) const;
        void SetVector3(Index row, Index startColumn, const AZ::Vector3& value);
    };
} // namespace EMotionFX::MotionMatching
